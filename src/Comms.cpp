//TODO:  process "<Error:ZProbe triggered before move, aborting command."
//TODO:    $RST=#   Erases G54 - G59 WCS offsets and G28 / 30 positions stored in EEPROM.

#include "Platforms/Platforms.h"

#include <imgui.h>

#include <chrono>
using namespace std::chrono;

#include <vector>

#include "Clout.h"
#include "Comms.h"
#include "Console.h"

//Winsock stuff
	CloutSocket sckCarvera;
	sockaddr_in addrCarvera;

	CloutSocket sckBroadcast;
	sockaddr_in addrBroadcast;

	struct timeval tv;

//Status info
	bool bCommsConnected = false;
	char sConnectedIP[16];
	WORD wConnectedPort;

	BYTE bDetectedDevices = 0;
	char sDetectedDevices[MAX_DEVICES][3][20];

//Message Queue
	std::vector<CarveraMessage> XmitMessageQueue;
	std::vector<CarveraMessage> RecvMessageQueue;
	void DetermineMsgType(CarveraMessage& msg);

//Inter-thread comms
	CloutThreadHandle hCommsThread;
	bool bOperationRunning = false;		//Set by other modules when a complex operation is running, and we should limit what we show in the console (like "ok"s)

	CloutThreadHandle hSocketsThread;

	CloutMutex hBufferMutex;  //Mutex for accessing send and recv buffers


//Make sure the string is null terminated, sometimes they aren't
void TerminateString(char *s, int iBytes)
{
	if (s[iBytes] != 0x0)
		s[iBytes] = 0x0;
}

//Sockets thread.  This handles direct socket comms stuff
THREADPROC_DEC SocketThreadProc(THREADPROC_ARG lpParameter)
{
	int n;

	steady_clock::time_point LastStatusRqst = steady_clock::now();
	XmitMessageQueue.clear();
	RecvMessageQueue.clear();

	//Start getting some info
		//SendCommandAndWait("?", false);
		//SendCommandAndWait("version", true);

	while (bCommsConnected)
	{
		//Check for incoming messages
			fd_set stReadFDS;
			FD_ZERO(&stReadFDS);
			FD_SET(sckCarvera, &stReadFDS);

			//Set the timeout time.  This has to be done every time because of linux
				tv.tv_sec = 0;
				tv.tv_usec = 10000; //10ms

			n = select(sckCarvera+1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read
			if (n > 0) //Something is available to read
			{
				char buf[MAX_DATA_LENGTH];
				n = recv(sckCarvera, buf, MAX_DATA_LENGTH, 0); //Read it
				if (n > 0)
				{
					//Place this message on the receive queue
						TerminateString(buf, n);

						CarveraMessage msg;
						msg.iProcessed = 0;
						memcpy(msg.cData, buf, n);
						//msg.Time = steady_clock::now();						

						DetermineMsgType(msg);

						if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
						{
							//TODO: Error message
							continue;
						}

						RecvMessageQueue.push_back(msg);

						ReleaseMutex(&hBufferMutex);
				}
				else if (n < 0) //Error
				{
					DisplaySocketError();
					break;
				}
				else //(n == 0)  Connection has been closed
					break;
				
			}
			else if (n < 0) //Error
			{
				DisplaySocketError();
				break;
			}


		//Send any messages in the queue
			if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
			{
				//TODO: Error message
				continue;
			}

			while (XmitMessageQueue.size() > 0)
			{
				n = send(sckCarvera, XmitMessageQueue.at(0).cData, strlen(XmitMessageQueue.at(0).cData)+1, 0); //Send the first message in the queue
				if (n < 0)
				{
					DisplaySocketError();
					break;
				}

				XmitMessageQueue.erase(XmitMessageQueue.begin()); //Delete the message we just sent
			}

			ReleaseMutex(&hBufferMutex);

		//Send status request if it's time
			if (TimeSince_ms(LastStatusRqst) > 500)
			{
				n = send(sckCarvera, "?", 2, 0);
				if (n < 0)
				{
					DisplaySocketError();
					break;
				}

				n = send(sckCarvera, "$G", 3, 0);
				if (n < 0)
				{
					DisplaySocketError();
					break;
				}

				LastStatusRqst = steady_clock::now();
			}

			Sleep(25);
	}

	Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection Closed");
	bCommsConnected = false;

	return 0;
}

//Main communications thread
THREADPROC_DEC CommsThreadProc(THREADPROC_ARG lpParameter)
{
	//Receive buffer
		const int iRecvBufferLen = 5000;
		char sRecv[iRecvBufferLen];
		int iBytes;
	
	int x;
	//int iLoopCounter = 0;

	while (1)
	{
		//Process incoming messages
			CloutThreadMessage msg;
			while (GetThreadMessage(&hCommsThread , &msg))
			{
				switch (msg.iType)
				{
					case MSG_SEND_STRING:	//Send a string to Carvera
						if (bCommsConnected)
						{
							const char *str = (const char*)msg.Param1;

							//Some message we don't want to wait for a response at all.  Homing doesn't give us any response until it's finished
							bool bWait = true;
								if (_strnicmp(str, "$H", 2) == 0)
									bWait = false;

									bWait = false;
							if (bWait)
								SendCommandAndWait(str, true);
							else
								SendCommand(str, true);
						}
					break;
				}
			}

			if (!bCommsConnected)
			{
				//Listen for broadcast messages from Carvera
				fd_set stReadFDS;
				FD_ZERO(&stReadFDS);
				FD_SET(sckBroadcast, &stReadFDS);

				//Set the timeout time.  This has to be done every time because of linux
				tv.tv_sec = 0;
				tv.tv_usec = 10000; //10ms

				int t = select(sckBroadcast+1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read

				if (t > 0)
				{
					iBytes = recv(sckBroadcast, sRecv, iRecvBufferLen, 0);
					if (iBytes > 0)
					{
						TerminateString(sRecv, iBytes);

						//Get the Name, IP, and Port from the message
							char sName[20]="";
							char sIP[20]="";
							char sPort[20]="";

							char *token=0;
							char* next_token = 0;
							token = strtok_r(sRecv, ",", &next_token);
							if (token != 0)
								strcpy(sName, token);
							token = strtok_r(0, ",", &next_token);
							if (token != 0)
								strcpy(sIP, token);
							token = strtok_r(0, ",", &next_token);
							if (token != 0)
								strcpy(sPort, token);

						//Check if it's already in our list
						BYTE bSearch1 = 0;
						BYTE bSearch2 = 0;
						for (x = 0; x < bDetectedDevices; x++)
						{
							bSearch1 = strcmp(sName, sDetectedDevices[x][0]);
							bSearch2 = strcmp(sIP, sDetectedDevices[x][1]);
							bSearch2 += strcmp(sPort, sDetectedDevices[x][2]);

							if (bSearch1 == 0 && bSearch2 == 0) //Perfect match
								break;
						}
						if (x == bDetectedDevices) //We went through the list and it's a new one, so add it
						{
							if (bDetectedDevices < MAX_DEVICES)
							{				
								strcpy(sDetectedDevices[x][0], sName);
								strcpy(sDetectedDevices[x][1], sIP);
								strcpy(sDetectedDevices[x][2], sPort);
								bDetectedDevices++;
							}
						}
						else if (bSearch2 != 0) //We found a duplicate, but the IP or Port has changed, so update that one
						{
							strcpy(sDetectedDevices[x][1], sIP);
							strcpy(sDetectedDevices[x][2], sPort);
						}
					}
				}

				continue;
			}
		
		ThreadSleep(25);
	}

	return 0;
}

//Setup communications
int CommsInit()
{
	int iResult;

	bCommsConnected = false;
	bDetectedDevices = 0;

	//Startup Winsock
		if (!StartupSockets())
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error starting Winsock!");
			return 0;
		}
	
	//Create the socket for finding Carvera devices on the network
		sckBroadcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sckBroadcast < 0)
		{
			//Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error creating DGRAM socket: %d", WSAGetLastError());
			DisplaySocketError();
			return 0;
		}
		
		addrBroadcast.sin_family = AF_INET;
		addrBroadcast.sin_port = htons(3333);
		addrBroadcast.sin_addr.s_addr = INADDR_ANY;

		//Bind the socket for receiving
		if (bind(sckBroadcast, (sockaddr*)&addrBroadcast, sizeof(addrBroadcast)) < 0)
		{
			//Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error binding DGRAM socket: %d", WSAGetLastError());
			DisplaySocketError();
			return 0;
		}

	//Set the recv timeout.  This is how long select() will wait to see if anything is available
		tv.tv_sec = 0;
		tv.tv_usec = 10000; //10ms

	//Events and mutexes
		//NewEvent(&hProbeResponseEvent);
		NewMutex(&hBufferMutex);

	//Create the communications thread
		CloutCreateThread(&hCommsThread, CommsThreadProc);

		return 1;
}

//Called from the main loop.  Do some processing of incoming messages
void Comms_Update()
{
	char *c;
	char *data;

	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return;
	}

	//Loop through all messages in the recv queue
		for (int x = 0; x < RecvMessageQueue.size(); x++)
		{
			data = RecvMessageQueue[x].cData;

			if (RecvMessageQueue[x].iType == CARVERA_MSG_STATUS)
			{
				//https://smoothieware.github.io/Webif-pack/documentation/web/html/configuring-grbl-v0.html
				if (strncmp(data, "<Idle|MPos", 10) == 0)
					MachineStatus.Status = Carvera::Status::Idle;
				else if (strncmp(data, "<Run|MPos", 9) == 0)
					MachineStatus.Status = Carvera::Status::Busy;
				else if (strncmp(data, "<Home|MPos", 10) == 0)
					MachineStatus.Status = Carvera::Status::Homing;
				else if (strncmp(data, "<Alarm|MPos", 11) == 0)
					MachineStatus.Status = Carvera::Status::Alarm;

				//Read WCS coords
					c = strstr(data, "WPos:");
					if (c != 0)
					{
						c += 5; //The start of X pos
						CommaStringTo3Doubles(c, &MachineStatus.Coord.Working.x, &MachineStatus.Coord.Working.y, &MachineStatus.Coord.Working.z);
					}

				//Read machine coords
					c = strstr(data, "MPos:");
					if (c != 0)
					{
						c += 5; //The start of X pos
						CommaStringTo3Doubles(c, &MachineStatus.Coord.Machine.x, &MachineStatus.Coord.Machine.y, &MachineStatus.Coord.Machine.z);
					}

				//Read feed rates
					c = strstr(data, "F:");
					if (c != 0)
					{
						c += 2; //The start of X feed rate
						CommaStringTo3Doubles(c, &MachineStatus.FeedRates.x, &MachineStatus.FeedRates.y, &MachineStatus.FeedRates.z);

						//Actually the first one isn't X feedrate.  Y feedrate is shared with X.  The first value always seems to be 0?
						MachineStatus.FeedRates.x = MachineStatus.FeedRates.y;
					}
			}
			else if (RecvMessageQueue[x].iType == CARVERA_MSG_PARSER)
			{
				if (strstr(data, "G90") != 0)
					MachineStatus.Positioning = Carvera::Positioning::Absolute;
				else if (strstr(data, "G91") != 0)
					MachineStatus.Positioning = Carvera::Positioning::Relative;


				//Current WCS
				Carvera::CoordSystem::eCoordSystem NewWCS = Carvera::CoordSystem::Unknown;

				if (strstr(data, "G53") != 0)
					NewWCS = Carvera::CoordSystem::G53;
				else if (strstr(data, "G54") != 0)
					NewWCS = Carvera::CoordSystem::G54;
				else if (strstr(data, "G55") != 0)
					NewWCS = Carvera::CoordSystem::G55;
				else if (strstr(data, "G56") != 0)
					NewWCS = Carvera::CoordSystem::G56;
				else if (strstr(data, "G57") != 0)
					NewWCS = Carvera::CoordSystem::G57;
				else if (strstr(data, "G58") != 0)
					NewWCS = Carvera::CoordSystem::G58;
				else if (strstr(data, "G59") != 0)
					NewWCS = Carvera::CoordSystem::G59;

				if (MachineStatus.WCS != NewWCS) //It's changed
				{
					MachineStatus.WCS = NewWCS;

					strcpy(szWCSChoices[1], "G53");
					strcpy(szWCSChoices[2], "G54");
					strcpy(szWCSChoices[3], "G55");
					strcpy(szWCSChoices[4], "G56");
					strcpy(szWCSChoices[5], "G57");
					strcpy(szWCSChoices[6], "G58");
					strcpy(szWCSChoices[7], "G59");

					if (NewWCS > Carvera::CoordSystem::G53)
						strcat(szWCSChoices[NewWCS], " (Active)");
				}

				if (strstr(data, "G20") != 0)
					MachineStatus.Units = Carvera::Units::inch;
				else if (strstr(data, "G21") != 0)
					MachineStatus.Units = Carvera::Units::mm;

				if (strstr(data, "G17") != 0)
					MachineStatus.Plane = Carvera::Plane::XYZ;
				else if (strstr(data, "G18") != 0)
					MachineStatus.Plane = Carvera::Plane::XZY;
				else if (strstr(data, "G19") != 0)
					MachineStatus.Plane = Carvera::Plane::YZX;
			}

			//Show it on the log
			//	if (bShowOnLog)
			//		Console.AddLog(CommsConsole::ITEM_TYPE_RECV, "<%s", sRecv);

			RecvMessageQueue[x].iProcessed++;

			//If this message has been here through 2 loops and nobody's removed it, ignore it
				if (RecvMessageQueue[x].iProcessed > 2)
				{
					RecvMessageQueue.erase(RecvMessageQueue.begin() + x); //TODO: Change this whole loop to use the iterator
					x--;
				}		
		}

	ReleaseMutex(&hBufferMutex);
}

void CommsDisconnect()
{
	//Console.AddLog("Disconnecting");
	CloseSocket(&sckCarvera); //This alone will alert the sockets thread to shutdown and clean things up
}


void DetermineMsgType(CarveraMessage &msg)
{
	msg.iType = CARVERA_MSG_UNKNOWN;

	if (strncmp(msg.cData, "ok", 2) == 0)
		msg.iType = CARVERA_MSG_OK;
	else if (_strnicmp(msg.cData, "G28 means goto", 14) == 0) //Annoying messages in response to a G28 command
		msg.iType = CARVERA_MSG_G28;
	else if (strncmp(msg.cData, "<Idle|MPos", 10) == 0 || strncmp(msg.cData, "<Run|MPos", 9) == 0 || strncmp(msg.cData, "<Home|MPos", 10 || strncmp(msg.cData, "<Alarm|MPos", 11)) == 0)
		msg.iType = CARVERA_MSG_STATUS;
	else if (strncmp(msg.cData, "[PRB:", 5) == 0) //Response to a probing operation	eg: [PRB:-227.990, -1.000, -1.000 : 1]
		msg.iType = CARVERA_MSG_PROBE;
	else if (strncmp(msg.cData, "[G", 2) == 0)  //Response to G Code parser status request
		msg.iType = CARVERA_MSG_PARSER;
}

void Comms_ConnectDevice(BYTE bDeviceIdx)
{
	//Create the socket for communicating with Carvera
		sckCarvera = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sckCarvera < 0)
		{
			DisplaySocketError();
			return;
		}

	//Get the address
		BuildAddress(&addrCarvera, sDetectedDevices[bDeviceIdx][1], sDetectedDevices[bDeviceIdx][2]);

	//Connect
		if (connect(sckCarvera, (sockaddr*)&addrCarvera, sizeof(addrCarvera)) != 0)
		{
			if (!DisplaySocketError())
				return;
		}

	//Connection established

		bCommsConnected = true;

		send(sckCarvera, "?", 2, 0);
		send(sckCarvera, "version", 8, 0);

	//Create the communications thread
		CloutCreateThread(&hSocketsThread, SocketThreadProc);

	//Save the info to display on the screen
		strcpy(sConnectedIP, sDetectedDevices[bDeviceIdx][1]);
		wConnectedPort = atoi(sDetectedDevices[bDeviceIdx][2]);

		Console.AddLog("Connected to %s at %s:%d", sDetectedDevices[bDeviceIdx][0], sConnectedIP, wConnectedPort);
}
void Comms_Disconnect()
{
	CommsDisconnect();
}
void Comms_SendString(const char* sString)
{
	CarveraMessage msg;

	//msg.Time = steady_clock::now();
	strcpy(msg.cData, sString);
	
	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return;
	}

	XmitMessageQueue.push_back(msg);

	ReleaseMutex(&hBufferMutex);
}

int Comms_PopMessageOfType(CarveraMessage* msg, int iType) //Removes the first message of the desired type from the queue (FIFO)
{
	int iRes = 0; //Not found

	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return -1; //Error
	}

	//Loop through all messages in the recv queue looking for a certain reply, ie a probe result
		for (int x = 0; x < RecvMessageQueue.size(); x++)
		{
			if (RecvMessageQueue[x].iType == iType) //Found this message
			{
				*msg = RecvMessageQueue[x];
				RecvMessageQueue.erase(RecvMessageQueue.begin() + x); //TODO: Change this whole loop to use the iterator
				iRes = 1; //Success
				break;
			}
		}

	ReleaseMutex(&hBufferMutex);

	return iRes;
}