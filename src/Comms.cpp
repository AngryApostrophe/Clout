//TODO:  process "<Error:ZProbe triggered before move, aborting command."

#include "Platforms/Platforms.h"

#include <imgui.h>

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

//Inter-thread comms
	CloutThreadHandle hCommsThread;
	bool bOperationRunning = false;		//Set by other modules when a complex operation is running, and we should limit what we show in the console (like "ok"s)

	CloutEventHandle hProbeResponseEvent;	//Alerts the probing operations when the comms thread receives a probe response message
	char* sProbeReplyMessage = 0;			//The probing completion message


//Make sure the string is null terminated, sometimes they aren't
void TerminateString(char *s, int iBytes)
{
	if (s[iBytes] != 0x0)
		s[iBytes] = 0x0;
}

//Main communications thread
#ifdef WIN32
DWORD WINAPI CommsThreadProc(_In_ LPVOID lpParameter)
#else
void* CommsThreadProc(void* arg)
#endif
{
	//Receive buffer
		const int iRecvBufferLen = 5000;
		char sRecv[iRecvBufferLen];
		int iBytes;
	
	int x;
	int iLoopCounter = 0;

	while (1)
	{
		//Process incoming messages
			CloutThreadMessage msg;
			while (GetThreadMessage(&hCommsThread , &msg))
			{
				switch (msg.iType)
				{
					case MSG_CONNECT_DEVICE: //User asks to connect to a device
					{
						unsigned long Param1 = *(unsigned long*)(&msg.Param1); //Yep, this is really stupid.  TODO: Fix this shit.
						unsigned long Param2 = *(unsigned long*)(&msg.Param2);

						iLoopCounter = 0;

						//Create the socket for communicating with Carvera
							sckCarvera = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
							if (sckCarvera < 0)
							{
								DisplaySocketError();

								return 0;
							}

						//Get the address
							BuildAddress(&addrCarvera, sDetectedDevices[Param1][1], sDetectedDevices[Param1][2]);

						//Connect
							if (connect(sckCarvera, (sockaddr*)&addrCarvera, sizeof(addrCarvera)) != 0)
							{
								if (!DisplaySocketError())
									continue;
							}
					
						//Connection established

						bCommsConnected = true;

						//Save the info to display on the screen
							strcpy(sConnectedIP, sDetectedDevices[Param1][1]);
							wConnectedPort = atoi(sDetectedDevices[Param1][2]);

						Console.AddLog("Connected to %s at %s:%d", sDetectedDevices[Param1][0], sConnectedIP, wConnectedPort);

						//Start getting some info
							SendCommandAndWait("?", false);
							SendCommandAndWait("version", true);
					}
					break;

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

						//wParam is a string allocated in the other thread.  We can delete it now
							if (msg.Param1 != 0)
								free((void*)msg.Param1);
					break;

					case MSG_DISCONNECT:
						CommsDisconnect();
					break;
				}
			}

			if (!bCommsConnected)
			{
				//Listen for broadcast messages from Carvera
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

				continue;
			}


		//We are connected to Carvera

		//Listen for anything coming in
			fd_set stReadFDS;
			FD_ZERO(&stReadFDS);
			FD_SET(sckCarvera, &stReadFDS);
			int t = select(-1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read
			if (t < 0)
			{
				DisplaySocketError();
				CommsDisconnect();

				return 0;
			}
			else if (t > 0) //Something is available
			{
				iBytes = recv(sckCarvera, sRecv, iRecvBufferLen, 0); //Read it
				if (iBytes > 0)
				{
					//Process the received data
					TerminateString(sRecv, iBytes);
					ProcessIncomingMessage(sRecv);
				}
				else if (iBytes == 0)
				{
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection Closed");
					CommsDisconnect();
				}
				else
				{
					DisplaySocketError();
					CommsDisconnect();
				}
			}

		//Periodic updates
			if (iLoopCounter > 15) //TODO: This should be based on how long since our last update, not a loop counter
			{
				SendCommandAndWait("?", false);

				if (MachineStatus.Status == Carvera::Status::Idle)
					SendCommandAndWait("$G", false);
				iLoopCounter = 0;
			}
			
		iLoopCounter++;
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

	//Events
		NewEvent(&hProbeResponseEvent);

	//Create the communications thread
		CloutCreateThread(&hCommsThread, CommsThreadProc);

		return 1;
}

void CommsDisconnect()
{
	Console.AddLog("Disconnected");
	CloseSocket(&sckCarvera);
	bCommsConnected = false;
}

bool SendCommand(const char *c, bool bShowOnLog)
{
	if (!bCommsConnected)
		return false;

	//Make sure it's got the terminator or Carvera won't recognize it
		char s[500];
		strcpy(s, c);
		int x = (int)strlen(s);
		if (x > 1)
		{					
			if (s[x] != '\n')
			{
				s[x+1] = 0x0;
				s[x] = '\n';
			}

			x = x + 1;
		}

	int n_bytes = send(sckCarvera, s, x, 0);
	if (n_bytes < 0)
	{
		//Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error sending command: %d", WSAGetLastError());
		DisplaySocketError();
		CommsDisconnect();
		return false;
	}

	if (bShowOnLog)
		Console.AddLog(CommsConsole::ITEM_TYPE_SENT, ">%s", c);

	return true;
}

bool SendCommandAndWait(const char* c, bool bShowOnLog)
{
	int iBytes;
	char sRecv[5000];
	BYTE bResult;

	//Should we listen for an OK response?
		bool bWaitForOK = false;
		if (c[0] == 'G' || c[0] == 'g')
			bWaitForOK = true;
		if (c[0] == 'M' || c[0] == 'm')
			bWaitForOK = true;
		else if (c[0] == '$')
			bWaitForOK = true;

	//Send the command
		if (!SendCommand(c, bShowOnLog))
			return false;

	//Listen for a response
		do
		{
			iBytes = recv(sckCarvera, sRecv, 5000, 0);
			if (iBytes > 0)
				TerminateString(sRecv, iBytes);
			else if (iBytes == 0)
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection Closed");
				CommsDisconnect();
				return false;
			}
			else
			{
				//Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Receive error: %d", WSAGetLastError());
				DisplaySocketError();
				CommsDisconnect();
				return false;
			}

		//Process the response
			bResult = ProcessIncomingMessage(sRecv, c, bShowOnLog);
		} while (bWaitForOK && bResult); //Keep going until we get "ok"

	return true;
}


BYTE ProcessIncomingMessage(char *sRecv, const char *sSent, bool bShowOnLog)
{
	char *c;
	BYTE bRetVal = 1;

	if (strncmp(sRecv, "ok", 2) == 0)
	{
		bRetVal = 0;

		if (bOperationRunning)
			bShowOnLog = false;
	}
	else if (_strnicmp(sRecv, "G28 means goto", 14) == 0) //Annoying messages in response to a G28 command
		bShowOnLog = false;
	else if (strncmp(sRecv, "<Idle|MPos", 10) == 0 || strncmp(sRecv, "<Run|MPos", 9) == 0 || strncmp(sRecv, "<Home|MPos", 10 || strncmp(sRecv, "<Alarm|MPos", 11)) == 0)
	{
		if (bOperationRunning)
			bShowOnLog = false;

		//https://smoothieware.github.io/Webif-pack/documentation/web/html/configuring-grbl-v0.html
		if (strncmp(sRecv, "<Idle|MPos", 10) == 0)
			MachineStatus.Status = Carvera::Status::Idle;
		else if (strncmp(sRecv, "<Run|MPos", 9) == 0)
			MachineStatus.Status = Carvera::Status::Busy;
		else if (strncmp(sRecv, "<Home|MPos", 10) == 0)
			MachineStatus.Status = Carvera::Status::Homing;
		else if (strncmp(sRecv, "<Alarm|MPos", 11) == 0)
			MachineStatus.Status = Carvera::Status::Alarm;

		//Read WCS coords
			c = strstr(sRecv, "WPos:");
			if (c != 0)
			{
				c += 5; //The start of X pos
				CommaStringTo3Doubles(c, &MachineStatus.Coord.Working.x, &MachineStatus.Coord.Working.y, &MachineStatus.Coord.Working.z);
			}
		
		//Read machine coords
			c = strstr(sRecv, "MPos:");
			if (c != 0)
			{
				c += 5; //The start of X pos
				CommaStringTo3Doubles(c, &MachineStatus.Coord.Machine.x, &MachineStatus.Coord.Machine.y, &MachineStatus.Coord.Machine.z);
			}

		//Read feed rates
			c = strstr(sRecv, "F:");
			if (c != 0)
			{
				c += 2; //The start of X feed rate
				CommaStringTo3Doubles(c, &MachineStatus.FeedRates.x, &MachineStatus.FeedRates.y, &MachineStatus.FeedRates.z);

				//Actually the first one isn't X feedrate.  Y feedrate is shared with X.  The first value always seems to be 0?
				MachineStatus.FeedRates.x = MachineStatus.FeedRates.y;
			}
	}
	else if (strncmp(sRecv, "[PRB:", 5) == 0) //Response to a probing operation	eg: [PRB:-227.990, -1.000, -1.000 : 1]
	{
		if (sProbeReplyMessage == 0)
		{
			sProbeReplyMessage = (char*)malloc(strlen(sRecv) + 1);
			strcpy(sProbeReplyMessage, sRecv);
			TriggerEvent(&hProbeResponseEvent); //Alert any probing operations about this message
		}
		else
		{
			//This means we got another probe reply before the previous one was process.  This shouldn't happen.
		}
	}		
	else	if (sSent != 0) //Process based on what we sent, if available
	{
		if (strncmp(sSent, "$G", 2) == 0) //We asked for gcode parser state
		{
			//Make sure we got what we're expecting
			if (sRecv[0] != '[') //Uh oh, don't recognize this message
				c = 0;
				//return 1;

			if (strstr(sRecv, "G90") != 0)
				MachineStatus.Positioning = Carvera::Positioning::Absolute;
			else if (strstr(sRecv, "G91") != 0)
				MachineStatus.Positioning = Carvera::Positioning::Relative;

			
			//Current WCS
				Carvera::CoordSystem::eCoordSystem NewWCS = Carvera::CoordSystem::Unknown;

				if (strstr(sRecv, "G53") != 0)
					NewWCS = Carvera::CoordSystem::G53;
				else if (strstr(sRecv, "G54") != 0)
					NewWCS = Carvera::CoordSystem::G54;
				else if (strstr(sRecv, "G55") != 0)
					NewWCS = Carvera::CoordSystem::G55;
				else if (strstr(sRecv, "G56") != 0)
					NewWCS = Carvera::CoordSystem::G56;
				else if (strstr(sRecv, "G57") != 0)
					NewWCS = Carvera::CoordSystem::G57;
				else if (strstr(sRecv, "G58") != 0)
					NewWCS = Carvera::CoordSystem::G58;
				else if (strstr(sRecv, "G59") != 0)
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

			if (strstr(sRecv, "G20") != 0)
				MachineStatus.Units = Carvera::Units::inch;
			else if (strstr(sRecv, "G21") != 0)
				MachineStatus.Units = Carvera::Units::mm;

			if (strstr(sRecv, "G17") != 0)
				MachineStatus.Plane = Carvera::Plane::XYZ;
			else if (strstr(sRecv, "G18") != 0)
					MachineStatus.Plane = Carvera::Plane::XZY;
			else if (strstr(sRecv, "G19") != 0)
				MachineStatus.Plane = Carvera::Plane::YZX;
		}
	}

	if (bShowOnLog)
		Console.AddLog(CommsConsole::ITEM_TYPE_RECV, "<%s", sRecv);

	return bRetVal;
}



void Comms_ConnectDevice(BYTE bDeviceIdx)
{
	//Post a message to the comms thread to connect to this device
		SendThreadMessage(&hCommsThread, MSG_CONNECT_DEVICE, (void*)bDeviceIdx, 0);
}
void Comms_Disconnect()
{
	//Post a message to the comms thread to disconnect from the device
		SendThreadMessage(&hCommsThread, MSG_DISCONNECT, 0, 0);
}
void Comms_SendString(const char* sString)
{
	//Send a message to the comms thread to send this string to Carvera

	//First save the string for that thread to access
		char* c = (char*)malloc(strlen(sString) + 2); //This will get free'd by the comms thread
		strcpy(c, sString);

	//Now post the message to the thread
		SendThreadMessage(&hCommsThread, MSG_SEND_STRING, (void*)c, 0);
}