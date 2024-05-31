//TODO:  handle    "<Error:ZProbe triggered before move, aborting command."



#include <Windows.h>

#include <imgui.h>

#include "Clout.h"
#include "Comms.h"
#include "Console.h"

//Winsock stuff
	SOCKET sckCarvera;
	sockaddr_in addrCarvera;

	SOCKET sckBroadcast;
	sockaddr_in addrBroadcast;

	struct timeval tv;

//Status info
	bool bCommsConnected = FALSE;
	char sConnectedIP[16];
	WORD wConnectedPort;

	BYTE bDetectedDevices = 0;
	char sDetectedDevices[MAX_DEVICES][3][20];

//Inter-thread comms
	DWORD dwCommsThreadID=0;
	bool bOperationRunning = false; //Set by other modules when a complex operation is running, and we should limit what we show in the console (like "ok"s)

	HANDLE hProbeResponseEvent;	//Alerts the probing operations when the comms thread receives a probe response message
	char* sProbeReplyMessage = 0;	//The probing completion message


//Make sure the string is null terminated, sometimes they aren't
void TerminateString(char *s, int iBytes)
{
	if (s[iBytes] != 0x0)
		s[iBytes] = 0x0;
}

//Main communications thread
DWORD WINAPI CommsThreadProc(_In_ LPVOID lpParameter)
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
			MSG msg;
			while (PeekMessage(&msg, nullptr, WM_USER, MSG_MAX, PM_REMOVE))
			{
				switch (msg.message)
				{
					case MSG_CONNECT_DEVICE: //User asks to connect to a device
						iLoopCounter = 0;

						//Create the socket for communicating with Carvera
							sckCarvera = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
							if (sckCarvera == INVALID_SOCKET)
							{
								Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error creating transmit socket: %d", WSAGetLastError());
								return 0;
							}

						//Get the address
							addrCarvera.sin_family = AF_INET;
							addrCarvera.sin_port = htons(atoi(sDetectedDevices[msg.wParam][2]));
							addrCarvera.sin_addr.s_addr = inet_addr(sDetectedDevices[msg.wParam][1]);

						//Connect
							if (connect(sckCarvera, (sockaddr*)&addrCarvera, sizeof(addrCarvera)) == SOCKET_ERROR)
							{
								int iError = WSAGetLastError();

								if (iError != WSAEISCONN)
								{
									Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection error: %d", iError);
									CommsDisconnect();

									continue;
								}
							}
					
						//Connection established

						bCommsConnected = TRUE;

						//Save the info to display on the screen
							strcpy_s(sConnectedIP, 16, sDetectedDevices[msg.wParam][1]);
							wConnectedPort = atoi(sDetectedDevices[msg.wParam][2]);

						Console.AddLog("Connected to %s at %s:%d", sDetectedDevices[msg.wParam][0], sConnectedIP, wConnectedPort);

						//Start getting some info
							SendCommandAndWait("?", FALSE);
							SendCommandAndWait("version", TRUE);
					break;

					case MSG_SEND_STRING:	//Send a string to Carvera
						if (bCommsConnected)
						{
							const char *str = (const char*)msg.wParam;

							//Some message we don't want to wait for a response at all.  Homing doesn't give us any response until it's finished
								BOOL bWait = TRUE;
								if (_strnicmp(str, "$H", 2) == 0)
									bWait = false;

									bWait = false;
							if (bWait)
								SendCommandAndWait(str, TRUE);
							else
								SendCommand(str, TRUE);
						}

						//wParam is a string allocated in the other thread.  We can delete it now
							if (msg.wParam != 0)
								free((void*)msg.wParam);
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
						token = strtok_s(sRecv, ",", &next_token);
						if (token != 0)
							strcpy_s(sName, 20, token);
						token = strtok_s(0, ",", &next_token);
						if (token != 0)
							strcpy_s(sIP, 20, token);
						token = strtok_s(0, ",", &next_token);
						if (token != 0)
							strcpy_s(sPort, 20, token);

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
							strcpy_s(sDetectedDevices[x][0], 20, sName);
							strcpy_s(sDetectedDevices[x][1], 20, sIP);
							strcpy_s(sDetectedDevices[x][2], 20, sPort);
							bDetectedDevices++;
						}
					}
					else if (bSearch2 != 0) //We found a duplicate, but the IP or Port has changed, so update that one
					{
						strcpy_s(sDetectedDevices[x][1], 20, sIP);
						strcpy_s(sDetectedDevices[x][2], 20, sPort);
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
			if (t == SOCKET_ERROR)
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Comms select error: %d", WSAGetLastError());
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
					Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Receive error: %d", WSAGetLastError());
					CommsDisconnect();
				}
			}

		//Periodic updates
			if (iLoopCounter > 15) //TODO: This should be based on how long since our last update, not a loop counter
			{
				SendCommandAndWait("?", FALSE);

				if (MachineStatus.Status == Carvera::Status::Idle)
					SendCommandAndWait("$G", FALSE);
				iLoopCounter = 0;
			}
			
		iLoopCounter++;
		Sleep(25);
	}

	return 0;
}

//Setup communications
int CommsInit()
{
	int iResult;

	bCommsConnected = FALSE;
	bDetectedDevices = 0;

	//Startup Winsock
		WSADATA wsaData;
		iResult = WSAStartup(MAKEWORD(1, 1), &wsaData);
		if (iResult != NO_ERROR)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error starting Winsock!");
			return 0;
		}
	
	//Create the socket for finding Carvera devices on the network
		sckBroadcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sckBroadcast == INVALID_SOCKET)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error creating DGRAM socket: %d", WSAGetLastError());
			return 0;
		}
		
		addrBroadcast.sin_family = AF_INET;
		addrBroadcast.sin_port = htons(3333);
		addrBroadcast.sin_addr.s_addr = INADDR_ANY;

		//Bind the socket for receiving
		if (bind(sckBroadcast, (sockaddr*)&addrBroadcast, sizeof(addrBroadcast)) < 0)
		{
			Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error binding DGRAM socket: %d", WSAGetLastError());
			return 0;
		}

	//Set the recv timeout.  This is how long select() will wait to see if anything is available
		tv.tv_sec = 0;
		tv.tv_usec = 10000; //10ms

	//Events
		hProbeResponseEvent = CreateEventA(NULL, FALSE, FALSE, NULL); //TODO: add a CloseHandle() to the shutdown function, when I make one some day

	//Create the communications thread
		dwCommsThreadID = 0;
		CreateThread(NULL, 0, CommsThreadProc, 0, 0, &dwCommsThreadID);

		return 1;
}

void CommsDisconnect()
{
	Console.AddLog("Disconnected");
	closesocket(sckCarvera);
	bCommsConnected = FALSE;
}

BOOL SendCommand(const char *c, BOOL bShowOnLog)
{
	if (!bCommsConnected)
		return FALSE;

	//Make sure it's got the terminator or Carvera won't recognize it
		char s[500];
		strcpy_s(s, 500, c);
		int x = strlen(s);
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
	if (n_bytes == SOCKET_ERROR)
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error sending command: %d", WSAGetLastError());
		CommsDisconnect();
		return FALSE;
	}

	if (bShowOnLog)
		Console.AddLog(CommsConsole::ITEM_TYPE_SENT, ">%s", c);

	return TRUE;
}

BOOL SendCommandAndWait(const char* c, BOOL bShowOnLog)
{
	int iBytes;
	char sRecv[5000];
	BYTE bResult;

	//Should we listen for an OK response?
		BOOL bWaitForOK = false;
		if (c[0] == 'G' || c[0] == 'g')
			bWaitForOK = true;
		if (c[0] == 'M' || c[0] == 'm')
			bWaitForOK = true;
		else if (c[0] == '$')
			bWaitForOK = true;

	//Send the command
		if (!SendCommand(c, bShowOnLog))
			return FALSE;

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
				return FALSE;
			}
			else
			{
				Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Receive error: %d", WSAGetLastError());
				CommsDisconnect();
				return FALSE;
			}

		//Process the response
			bResult = ProcessIncomingMessage(sRecv, c, bShowOnLog);
		} while (bWaitForOK && bResult); //Keep going until we get "ok"

	return TRUE;
}


BYTE ProcessIncomingMessage(char *sRecv, const char *sSent, BOOL bShowOnLog)
{
	char sTemp[100];
	char *c;
	BYTE bRetVal = 1;
	int x;

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
			}
	}
	else if (strncmp(sRecv, "[PRB:", 5) == 0) //Response to a probing operation	eg: [PRB:-227.990, -1.000, -1.000 : 1]
	{
		if (sProbeReplyMessage == 0)
		{
			sProbeReplyMessage = (char*)malloc(strlen(sRecv) + 1);
			strcpy_s(sProbeReplyMessage, strlen(sRecv) + 1, sRecv);
			SetEvent(hProbeResponseEvent); //Alert any probing operations about this message
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

					strcpy_s(szWCSChoices[1], 20, "G53");
					strcpy_s(szWCSChoices[2], 20, "G54");
					strcpy_s(szWCSChoices[3], 20, "G55");
					strcpy_s(szWCSChoices[4], 20, "G56");
					strcpy_s(szWCSChoices[5], 20, "G57");
					strcpy_s(szWCSChoices[6], 20, "G58");
					strcpy_s(szWCSChoices[7], 20, "G59");

					if (NewWCS > Carvera::CoordSystem::G53)
						strcat_s(szWCSChoices[NewWCS], 20, " (Active)");
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
		PostThreadMessageA(dwCommsThreadID, MSG_CONNECT_DEVICE, bDeviceIdx, 0);
}
void Comms_Disconnect()
{
	//Post a message to the comms thread to disconnect from the device
		PostThreadMessageA(dwCommsThreadID, MSG_DISCONNECT, 0, 0);
}
void Comms_SendString(const char* sString)
{
	//Send a message to the comms thread to send this string to Carvera

	//First save the string for that thread to access
		char* c = (char*)malloc(strlen(sString) + 2); //This will get free'd by the comms thread
		strcpy_s(c, strlen(sString) + 2, sString);

	//Now post the message to the thread
		PostThreadMessageA(dwCommsThreadID, MSG_SEND_STRING, (WPARAM)c, 0);
}