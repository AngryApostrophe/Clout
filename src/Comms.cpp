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

	struct sHiddenReplies
	{
		int iType;
		std::chrono::steady_clock::time_point Time;		//Time this one was ignore.  It'll time out after a bit
	};
	std::vector<sHiddenReplies> HiddenReplies; //List of replies from Carvera that we will ignore and not show on console.  Ie, response to regular ? and $G updates

//Inter-thread comms
	CloutThreadHandle hCommsThread;
	bool bOperationRunning = false;		//Set by other modules when a complex operation is running, and we should limit what we show in the console (like "ok"s).  TODO: This may no longer be necessary, revisit later

	CloutMutex hBufferMutex;  //Mutex for accessing send and recv buffers


//Make sure the string is null terminated, sometimes they aren't
void TerminateString(char *s, int iBytes)
{
	if (s[iBytes] != 0x0)
		s[iBytes] = 0x0;
}

bool IsMessageIgnored(int iType)
{
	std::vector<sHiddenReplies>::iterator iter;

	//First remove any old ones
		for (iter = HiddenReplies.begin(); iter != HiddenReplies.end();)
		{
			if (TimeSince_ms(iter->Time) > 500) //If we haven't gotten this reply in 500ms, it's not coming
				iter = HiddenReplies.erase(iter);
			else
				iter++;
		}

	//Loop through all messages in the ignore list looking for this type
		for (iter = HiddenReplies.begin(); iter != HiddenReplies.end(); iter++)
		{
			if (iter->iType == iType) //Found this type
			{
				iter = HiddenReplies.erase(iter);
				return true;
			}
		}

	return false; //Didn't find one in the list
}

void HideReply(int iType)
{
	sHiddenReplies hide;
	hide.iType = iType;
	hide.Time = steady_clock::now();
	HiddenReplies.push_back(hide);
}

//Sockets thread.  This handles direct socket comms stuff
THREADPROC_DEC CommsThreadProc(THREADPROC_ARG lpParameter)
{
	int n;
	int s;
	char buf[MAX_DATA_LENGTH];

	steady_clock::time_point LastStatusRqst = steady_clock::now();
	XmitMessageQueue.clear();
	RecvMessageQueue.clear();
	HiddenReplies.clear();

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

			s = select(sckCarvera+1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read
			if (s > 0) //Something is available to read
			{					
				n = recv(sckCarvera, buf, MAX_DATA_LENGTH, 0); //Read it
				if (n > 0)
				{
					//Place this message on the receive queue
						TerminateString(buf, n);

						if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
						{
							//TODO: Error message
							Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error waiting for receive buffer access");
							continue;
						}

						//Sometimes multiple messages come in at once, delimited by \n.  Split them up
							char *c = buf;
							while (c < buf+n && c != 0)
							{
								CarveraMessage msg;
								memset(&msg, 0x0, sizeof(msg));

								msg.iProcessed = 0;
								strcpy(msg.cData, c);

								char *z = strstr(msg.cData, "\n");
								if (z != 0)
									*z = 0x0;

								DetermineMsgType(msg);

								RecvMessageQueue.push_back(msg);

								//Display on the console
									if (!IsMessageIgnored(msg.iType) && msg.iType != CARVERA_MSG_OK)
										Console.AddLog(CommsConsole::ITEM_TYPE_RECV, msg.cData);

								//Move on to the next one
									c = strstr(c, "\n");
									if (c != 0)
										c++; //Move past it
							}

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
			else if (s < 0) //Error
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
				//Make sure it's got the terminator or Carvera won't recognize it
					strcpy(buf, XmitMessageQueue.at(0).cData);
					n = strlen(buf);
					if (n > 1)
					{
						if (buf[n] != '\n')
						{
							buf[n + 1] = 0x0;
							buf[n] = '\n';
						}

						n = n + 1;
					}

				n = send(sckCarvera,buf, n, 0); //Send the first message in the queue
				if (n < 0)
				{
					DisplaySocketError();
					break;
				}

				if (!XmitMessageQueue.at(0).bHidden)
					Console.AddLog(CommsConsole::ITEM_TYPE_SENT, XmitMessageQueue.at(0).cData);

				XmitMessageQueue.erase(XmitMessageQueue.begin()); //Delete the message we just sent
			}

			ReleaseMutex(&hBufferMutex);

		//Send status request if it's time
			if (TimeSince_ms(LastStatusRqst) > 500)
			{
				if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
				{
					//TODO: Error message
					continue;
				}

				CarveraMessage msg;
				msg.bHidden = true;

				strcpy(msg.cData, "?");
				XmitMessageQueue.push_back(msg);
				HideReply(CARVERA_MSG_STATUS);

				if (MachineStatus.Status == Carvera::Status::Idle)
				{
					strcpy(msg.cData, "$G");
					XmitMessageQueue.push_back(msg);

					HideReply(CARVERA_MSG_PARSER);
					HideReply(CARVERA_MSG_OK);
				}				

				LastStatusRqst = steady_clock::now();

				ReleaseMutex(&hBufferMutex);
			}

			ThreadSleep(25);
	}

	Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection Closed");
	bCommsConnected = false;

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
		NewMutex(&hBufferMutex);

		return 1;
}

void Comms_ListenForCarveras()
{
	int x;
	
	//Receive buffer
		const int iRecvBufferLen = 5000;
		char sRecv[iRecvBufferLen];
		int iBytes;

	if (!bCommsConnected)
	{
		//Listen for broadcast messages from Carvera
			fd_set stReadFDS;
			FD_ZERO(&stReadFDS);
			FD_SET(sckBroadcast, &stReadFDS);

		//Set the timeout time.  This has to be done every time because of linux
			tv.tv_sec = 0;
			tv.tv_usec = 10000; //10ms

		int t = select(sckBroadcast + 1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read

		if (t > 0)
		{
			iBytes = recv(sckBroadcast, sRecv, iRecvBufferLen, 0);
			if (iBytes > 0)
			{
				TerminateString(sRecv, iBytes);

				//Get the Name, IP, and Port from the message
					char sName[20] = "";
					char sIP[20] = "";
					char sPort[20] = "";

					char* token = 0;
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
	}
}

//Called from the main loop.  Do some processing of incoming messages
void Comms_Update()
{
	char *c;
	char *data;

	Comms_ListenForCarveras();

	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return;
	}

	//Loop through all messages in the recv queue
		//for (int x = 0; x < RecvMessageQueue.size(); x++)
		for (auto iter = RecvMessageQueue.begin(); iter != RecvMessageQueue.end();)
		{
			data = iter->cData;

			if (iter->iType == CARVERA_MSG_STATUS)
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
			else if (iter->iType == CARVERA_MSG_PARSER)
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

			iter->iProcessed++;

			//If this message has been here through 2 loops and nobody's come looking for it, ignore it
				if (iter->iProcessed > 2)
					iter = RecvMessageQueue.erase(iter);
				else
					iter++;
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

	//Create the communications thread
		CloutCreateThread(&hCommsThread, CommsThreadProc);

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

	msg.bHidden = false;

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
		for (auto iter = RecvMessageQueue.begin(); iter != RecvMessageQueue.end(); iter++)
		{
			if (iter->iType == iType)
			{
				*msg = *iter;
				RecvMessageQueue.erase(iter);

				iRes = 1; //Success
				break;
			}
		}

	ReleaseMutex(&hBufferMutex);

	return iRes;
}