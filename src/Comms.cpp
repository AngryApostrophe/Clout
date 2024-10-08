//TODO:  process "<Error:ZProbe triggered before move, aborting command."
//TODO:    $RST=#   Erases G54 - G59 WCS offsets and G28 / 30 positions stored in EEPROM.

//https://github.com/Smoothieware/smoothieware-website-v1/blob/main/docs/console-commands.md
//https://smoothieware.github.io/Webif-pack/documentation/web/html/configuring-grbl-v0.html
//https://smoothieware.github.io/Webif-pack/documentation/web/html/console-commands.html
//https://github.com/grbl/grbl/wiki/Interfacing-with-Grbl

#include "Platforms/Platforms.h"

#include <imgui.h>

#include <deque>
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

//Serial stuff
	HANDLE hSerialDevice = 0;

//Status info
	bool bCommsConnected = false;	
	bool bFileTransferInProgress; //This is set by the file transfer code.  It stops us from sending regular status requests

//Message Queue
	std::deque<CarveraMessage> XmitMessageQueue;
	std::deque<CarveraMessage> RecvMessageQueue;
	void DetermineMsgType(CarveraMessage& msg);

	struct sHiddenReplies
	{
		int iType;
		std::chrono::steady_clock::time_point Time;		//Time this one was ignore.  It'll time out after a bit
	};
	std::vector<sHiddenReplies> HiddenReplies; //List of replies from Carvera that we will ignore and not show on console.  Ie, response to regular ? and $G updates

	unsigned long XmitID;	//Basically a running total of all messages sent.  Used for ignoring replies received before a message was sent.  Could also switch to time

	void ProcessUpdateMsg(CarveraMessage& msg);

//Inter-thread comms
	CloutThreadHandle hCommsThread;

	CloutMutex hBufferMutex;  //Mutex for accessing send and recv buffers

//Connection stuff
	int iWifiMode = 1;	//0 = Serial,   1 = Wifi

	char sConnectedIP[16];
	WORD wConnectedPort;

	BYTE bDetectedDevices = 0;
	char sDetectedDevices[MAX_DEVICES][3][30];


//Make sure the string is null terminated, sometimes they aren't
void TerminateString(char *s, int iBytes)
{
	if (s[iBytes] != 0x0)
		s[iBytes] = 0x0;
}

bool IsMessageIgnored(int iType)
{
	std::vector<sHiddenReplies>::iterator iter;

/*	//First remove any old ones
		for (iter = HiddenReplies.begin(); iter != HiddenReplies.end();)
		{
			if (TimeSince_ms(iter->Time) > 500) //If we haven't gotten this reply in 500ms, it's not coming
				iter = HiddenReplies.erase(iter);
			else
				iter++;
		}
		*/
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

void Comms_HideReply(int iType)
{
	sHiddenReplies hide;
	hide.iType = iType;
	hide.Time = steady_clock::now();
	HiddenReplies.push_back(hide);
}

//Sockets thread.  This handles direct socket comms stuff
THREADPROC_DEC CommsThreadProc(THREADPROC_ARG lpParameter)
{
	long n;
	unsigned long ul;
	int s;
	char buf[MAX_DATA_LENGTH];
	unsigned long len = 0;

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
			if (iWifiMode)
				FD_SET(sckCarvera, &stReadFDS);

			//Set the timeout time.  This has to be done every time because of linux
				tv.tv_sec = 0;
				tv.tv_usec = 10000; //10ms

			//Check if data is available to read
				if (iWifiMode)
					s = select((int)sckCarvera + 1, &stReadFDS, 0, 0, &tv);
				else
					s = 1; //Serial port won't block, so just assume it's there
			
			if (s > 0) //Something is available to read
			{					
				//Read the data
					if (iWifiMode)
						n = recv(sckCarvera, buf + len, MAX_DATA_LENGTH - len, 0); //Read it
					else
					{
						if (ReadFile(hSerialDevice, buf + len, MAX_DATA_LENGTH - len, &ul, 0) != 0)
							n = ul;	//Bytes received
						else
							n = -1; //Alert that we had an error
					}

				len += n;
				if (n > 0 && memchr(buf, '\n', len)) //If there's no newline, it's a partial message.  Keep reading
				{
					//Place this message on the receive queue
						TerminateString(buf, len);

						if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
						{
							Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Error waiting for receive buffer access");
							continue;
						}

						//Sometimes multiple messages come in at once, delimited by \n.  Split them up
						char* start = buf;
						char* end = strchr(buf, '\n');
						while (end) {
							*end = 0;

							CarveraMessage msg;
							memset(&msg, 0x0, sizeof(msg));

							msg.iProcessed = 0;
							strcpy(msg.cData, start);
							msg.iLen = (int)strlen(msg.cData);
							msg.markpoint = XmitID;

							DetermineMsgType(msg);

							if (msg.iType == CARVERA_MSG_STATUS || msg.iType == CARVERA_MSG_PARSER || msg.iType == CARVERA_MSG_DEBUG)
								ProcessUpdateMsg(msg);

							//Display on the console
							if (!IsMessageIgnored(msg.iType)/* && msg.iType != CARVERA_MSG_OK*/)
							{
								/*if (msg.iType == CARVERA_MSG_UNKNOWN && msg.iLen == 1)
									Console.AddLog(CommsConsole::ITEM_TYPE_RECV, "0x%x", msg.cData[0]);
								else if (msg.iType == CARVERA_MSG_XMODEM_ACK)
									Console.AddLog(CommsConsole::ITEM_TYPE_RECV, "<Ack>");
								else if (msg.iType == CARVERA_MSG_XMODEM_NAK)
									Console.AddLog(CommsConsole::ITEM_TYPE_RECV, "<Nak>");
								else*/ if (msg.iType != CARVERA_MSG_OK && msg.iType != CARVERA_MSG_XMODEM_C && msg.iType != CARVERA_MSG_XMODEM_NAK && msg.iType != CARVERA_MSG_XMODEM_ACK)
											Console.AddLogSimple(CommsConsole::ITEM_TYPE_RECV, msg.cData);

								if (msg.iType == CARVERA_MSG_PROGRESS)
									msg.iLen = msg.iLen;


								RecvMessageQueue.push_back(msg);
							}

							//Move on to the next message
							start = end + 1;
							end = strchr(start, '\n');
						}

						//The remainder doesn't contain a newline.  Go back to reading from the socket
						strcpy(buf, start);
						len = strlen(buf);

						ReleaseMutex(&hBufferMutex);
				}
				else if (n < 0) //Error
				{
					DisplaySocketError();
					break;
				}
				else if (n == 0 && iWifiMode)  //Connection has been closed (only on wifi)
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
				if (XmitMessageQueue[0].iType == CARVERA_OUTBOUND_STRING)
				{
					//Make sure it's got the terminator or Carvera won't recognize it
						strcpy(buf, XmitMessageQueue[0].cData);
						n = (int)strlen(buf);
						if (n > 0 && buf[n - 1] != '\n')
						{
							buf[n] = '\n';
							buf[n + 1] = 0x0;
							n++;
						}
				}
				else
				{
					n = XmitMessageQueue[0].iLen;
					memcpy(buf, XmitMessageQueue[0].cData, n);
				}

				//Write the data to the output
					if (iWifiMode)
						n = send(sckCarvera,buf, n, 0);
					else
					{
						unsigned long BytesWritten = 0;
						if (WriteFile(hSerialDevice, buf, n, &BytesWritten, 0) != 0)
							n = BytesWritten;
						else
							n = -1;	//Alert to an error
					}

				if (n < 0)
				{
					DisplaySocketError();
					break;
				}

				if (!XmitMessageQueue.at(0).bHidden)
					Console.AddLog(CommsConsole::ITEM_TYPE_SENT, XmitMessageQueue.at(0).cData);

				XmitMessageQueue.pop_front();  //Delete the message we just sent
			}

			ReleaseMutex(&hBufferMutex);

		//Send status request if it's time
			if (TimeSince_ms(LastStatusRqst) > 500 && !bFileTransferInProgress)
			{
				if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
				{
					//TODO: Error message
					continue;
				}

				CarveraMessage msg;
				msg.iType = CARVERA_OUTBOUND_STRING;
				msg.bHidden = true;

				strcpy(msg.cData, "?");
				XmitID++; //This is kinda hacky since we're skipping Comms_SendString
				XmitMessageQueue.push_back(msg);
				Comms_HideReply(CARVERA_MSG_STATUS);

				if (MachineStatus.Status == Carvera::Status::Idle)
				{
					strcpy(msg.cData, "$G");
					XmitID++; //This is kinda hacky since we're skipping Comms_SendString
					XmitMessageQueue.push_back(msg);

					Comms_HideReply(CARVERA_MSG_PARSER);
					Comms_HideReply(CARVERA_MSG_OK);
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

void Comms_ListenForCarverasOnWifi()
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

		int t = select((int)sckBroadcast + 1, &stReadFDS, 0, 0, &tv); //Check if any data is available to read

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
			} //if (iBytes > 0)
		} //if (t > 0)
	}
}

void ProcessUpdateMsg(CarveraMessage &msg)
{
	char *data = msg.cData;
	char *c;

	if (msg.iType == CARVERA_MSG_STATUS)
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

		//Spindle info
		MachineStatus.dSpindleRPM = 0;
		MachineStatus.dSpindleTargetRPM = 0;
		c = strstr(data, "|S:");
		if (c != 0)
		{
			c += 3; //The start of the data
			CommaStringTo3Doubles(c, &MachineStatus.dSpindleRPM, &MachineStatus.dSpindleTargetRPM, &MachineStatus.dSpindleRPMFactor);
		}

		//Play status
		c = strstr(data, "|P:");
		if (c != 0)
		{
			c += 3; //The start of the info
			CommaStringTo3Ints(c, &MachineStatus.Playing.iLinesComplete, &MachineStatus.Playing.iPercentComplete, &MachineStatus.Playing.iElapsedSecs);
		}

		//Tool info
		MachineStatus.iCurrentTool = -1;

		c = strstr(data, "|T:");
		if (c != 0)
		{
			c += 3; //The number
			int iTool = atoi(c);
			if (iTool > 0 && iTool <= 6)
				MachineStatus.iCurrentTool = iTool;

			c += 2; //To the TLO
			MachineStatus.fToolLengthOffset = (float)atof(c);
		}
	}
	else if (msg.iType == CARVERA_MSG_PARSER)
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
	else if (msg.iType == CARVERA_MSG_DEBUG)
	{
		c = strstr(msg.cData, "|P:");
		if (c != 0)
		{
			c += 3;
			if (*c == '1')
				MachineStatus.bProbeTriggered = true;
			else
				MachineStatus.bProbeTriggered = false;
		}
	}
}


//Called from the main loop.  Do some processing of incoming messages
void Comms_Update()
{
	char *data;

	if (iWifiMode)
		Comms_ListenForCarverasOnWifi();

	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return;
	}

	//Loop through all messages in the recv queue
		for (auto iter = RecvMessageQueue.begin(); iter != RecvMessageQueue.end();)
		{
			data = iter->cData;

			ProcessUpdateMsg(*iter);

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
	if (iWifiMode)
		CloseSocket(&sckCarvera); //This alone will alert the sockets thread to shutdown and clean things up
	else
		CloseHandle(hSerialDevice);
}


void DetermineMsgType(CarveraMessage &msg)
{
	msg.iType = CARVERA_MSG_UNKNOWN;

	if (strncmp(msg.cData, "ok", 2) == 0)
		msg.iType = CARVERA_MSG_OK;
	else if (strncmp(msg.cData, "[G54:", 5) == 0 && strstr(msg.cData, "[PRB:") != 0)
		msg.iType = CARVERA_MSG_PARAMETERS;
	else if (strncmp(msg.cData, "{S:", 3) == 0)
		msg.iType = CARVERA_MSG_DEBUG;
	else if (_strnicmp(msg.cData, "G28 means goto", 14) == 0) //Annoying messages in response to a G28 command
		msg.iType = CARVERA_MSG_G28;
	else if (strncmp(msg.cData, "<Idle|MPos", 10) == 0 || strncmp(msg.cData, "<Run|MPos", 9) == 0 || strncmp(msg.cData, "<Home|MPos", 10 || strncmp(msg.cData, "<Alarm|MPos", 11) || strncmp(msg.cData, "<Hold|MPos", 10) || strncmp(msg.cData, "<Sleep|MPos", 11) || strncmp(msg.cData, "<Wait|MPos", 10 || strncmp(msg.cData, "<Pause|MPos", 11))) == 0)
		msg.iType = CARVERA_MSG_STATUS;
	else if (strncmp(msg.cData, "[PRB:", 5) == 0) //Response to a probing operation	eg: [PRB:-227.990, -1.000, -1.000 : 1]
		msg.iType = CARVERA_MSG_PROBE;
	else if (strncmp(msg.cData, "[G", 2) == 0)  //Response to G Code parser status request
		msg.iType = CARVERA_MSG_PARSER;
	else if (strncmp(msg.cData, "Homing atc...", 13) == 0)
		msg.iType = CARVERA_MSG_ATC_HOMING;
	else if (strncmp(msg.cData, "ATC homed!", 10) == 0)
		msg.iType = CARVERA_MSG_ATC_HOMED;
	else if (strncmp(msg.cData, "ATC loosed!", 11) == 0 || strncmp(msg.cData, "Already loosed!", 15) == 0)
		msg.iType = CARVERA_MSG_ATC_LOOSED;
	else if (strncmp(msg.cData, "ATC clamped!", 12) == 0 || strncmp(msg.cData, "Already clamped!", 16) == 0)
		msg.iType = CARVERA_MSG_ATC_CLAMPED;
	else if (strncmp(msg.cData, "Done ATC", 8) == 0)
		msg.iType = CARVERA_MSG_ATC_DONE;
	else if (msg.cData[0] == 'C' && msg.iLen == 1)
		msg.iType = CARVERA_MSG_XMODEM_C;
	else if (msg.cData[0] == 0x04 && msg.iLen == 1)
		msg.iType = CARVERA_MSG_XMODEM_EOT;
	else if (msg.cData[0] == 0x06 && msg.iLen == 1)
		msg.iType = CARVERA_MSG_XMODEM_ACK;
	else if (msg.cData[0] == 0x15 && msg.iLen == 1)
		msg.iType = CARVERA_MSG_XMODEM_NAK;
	else if (msg.cData[0] == 0x16 && msg.iLen == 1)
		msg.iType = CARVERA_MSG_XMODEM_CAN;
	else if (strncmp(msg.cData, "Info: upload success", 20) == 0)
		msg.iType = CARVERA_MSG_UPLOAD_SUCCESS;
	else if (strncmp(msg.cData, "Playing ", 8) == 0)
		msg.iType = CARVERA_MSG_PLAYING;
	else if (strncmp(msg.cData, "M2 (Clout sync)", 15) == 0)
		msg.iType = CARVERA_MSG_CLOUTSYNC;
	else if (strncmp(msg.cData, "file: ", 6) == 0)
		msg.iType = CARVERA_MSG_PROGRESS;
	
}

void Comms_ConnectWifiDevice(BYTE bDeviceIdx)
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
		XmitID = 0;
		bFileTransferInProgress = false;

	//Create the communications thread
		CloutCreateThread(&hCommsThread, CommsThreadProc);

	//Save the info to display on the screen
		strcpy(sConnectedIP, sDetectedDevices[bDeviceIdx][1]);
		wConnectedPort = atoi(sDetectedDevices[bDeviceIdx][2]);

		Console.AddLog("Connected to %s at %s:%d", sDetectedDevices[bDeviceIdx][0], sConnectedIP, wConnectedPort);

	//Switch to the status page
		ImGui::SetWindowFocus("Status");
}

void Comms_ConnectSerialDevice(BYTE bDeviceIdx)
{
	if (ConnectToSerialPort(sDetectedDevices[bDeviceIdx][0], &hSerialDevice) != 1)
	{
		return;
	}

	//Setup timeouts
		COMMTIMEOUTS timeouts;

		timeouts.ReadIntervalTimeout = MAXDWORD;
		timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		timeouts.ReadTotalTimeoutConstant = 1;
		timeouts.WriteTotalTimeoutMultiplier = 1;
		timeouts.WriteTotalTimeoutConstant = 1;
		SetCommTimeouts(hSerialDevice, &timeouts);

	//Connection established
		bCommsConnected = true;
		XmitID = 0;
		bFileTransferInProgress = false;

	//Create the communications thread
		CloutCreateThread(&hCommsThread, CommsThreadProc);

	//Save the info to display on the screen
		Console.AddLog("Connected to Carvera on %s", sDetectedDevices[bDeviceIdx][0]);

	//Switch to the status page
		ImGui::SetWindowFocus("Status");
}

void Comms_ConnectDevice(BYTE bDeviceIdx)
{
	if (iWifiMode)
		Comms_ConnectWifiDevice(bDeviceIdx);
	else
		Comms_ConnectSerialDevice(bDeviceIdx);
}


void Comms_Disconnect()
{
	CommsDisconnect();
}
unsigned long Comms_SendString(const char* sString, bool bShowInConsole)
{
	CarveraMessage msg;

	msg.bHidden = !bShowInConsole;
	msg.iType = CARVERA_OUTBOUND_STRING;

	//msg.Time = steady_clock::now();
	strcpy(msg.cData, sString);
	
	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return 0;
	}

	XmitMessageQueue.push_back(msg);

	XmitID++;

	ReleaseMutex(&hBufferMutex);

	return XmitID;
}

void Comms_SendBytes(const char* bytes, int iLen)
{
	CarveraMessage msg;
	msg.iType = CARVERA_OUTBOUND_BYTES;

	memcpy(msg.cData, bytes, iLen);
	msg.iLen = iLen;

	if (WaitForMutex(&hBufferMutex, true) != MUTEX_RESULT_SUCCESS)
	{
		//TODO: Error message
		return;
	}

	XmitMessageQueue.push_back(msg);

	ReleaseMutex(&hBufferMutex);
}

int Comms_PopMessageOfType(int iType, CarveraMessage *msg, unsigned long EarliestID) //Removes the first message of the desired type from the queue (FIFO)
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
			if (iter->iType == iType && iter->markpoint >= EarliestID)
			{
				if (msg != 0)
					*msg = *iter;

				RecvMessageQueue.erase(iter);

				iRes = 1; //Success
				break;
			}
		}

	ReleaseMutex(&hBufferMutex);

	return iRes;
}

void Comms_ChangeConnectionType(int iNewWifiMode)
{
	iWifiMode = iNewWifiMode;
	bDetectedDevices = 0;

	if (!iWifiMode)
		ListSerialPorts();
}