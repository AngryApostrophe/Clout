//This is adapted from the Carvera firmware source code.  No effort was given to cleaning it up

#include "Platforms/Platforms.h"

#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>

#include <chrono>
using namespace std::chrono;

#include <imgui.h>

#include "Clout.h"
#include "Helpers.h"
#include "Comms.h"
#include "Console.h"

#include "md5.h"

#define XFER_STATE_COMPLETE	99


#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x16
#define CTRLZ 0x1A

static int iFileTransferState;

static bool bTransferComplete;

static int bufsz = 8192;
static unsigned char xbuff[8200];  /* 2 for data length, 8192 for XModem + 3 head chars + 2 crc + nul */
static bool bUseCRC;
static int is_stx = 1; //Indicates 1024 byte packets
static unsigned char packetno = 0;
static bool bMD5Sent;


static std::string sRawData;
static std::vector<std::string> sFileLines;
static std::string sMD5;
static std::string::iterator XmitIter;
static std::string::iterator tempXmitIter; //This one may get moved back if a packet failed to send

static steady_clock::time_point LastMsgAttempt; //Used for repeating stuff


static unsigned int crc16_ccitt(unsigned char* data, unsigned int len)
{
	static const unsigned short crc_table[] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
	};

	unsigned char tmp;
	unsigned short crc = 0;

	for (unsigned int i = 0; i < len; i++) {
		tmp = ((crc >> 8) ^ data[i]) & 0xff;
		crc = ((crc << 8) ^ crc_table[tmp]) & 0xffff;
	}

	return crc & 0xffff;
}




void FileTransfer_BeginUpload(const char *szLocalFilename, int iFirstLine, int iLastLine)
{
	int i;
	sRawData.clear();	//Empty everything out
	sFileLines.clear();

	//Read in line-by-line
		std::ifstream file(szLocalFilename);
		if (file.is_open())
		{
			std::string line;

			//Discard ignored lines
				for (i = 0; i <= iFirstLine; i++)
				{	
					std::getline(file, line);
				}

			//Read in until the last line requested
				while (getline(file, line))
				{					
					sFileLines.push_back(line);

					i++;
					if (i > iLastLine+1 && iLastLine >= 0)
						break;
				}

			file.close();
		}

	//Convert the array to a raw stream
		for (i = 0; i < sFileLines.size(); i++)
		{
			sRawData += sFileLines[i] + "\n";
		}

	//Calculate the MD5
		sMD5 = md5(sRawData);

	//Setup the transfer
		bUseCRC = 0;
		packetno = 0;
		bTransferComplete = false;
		iFileTransferState = 0;
		bMD5Sent = false;
		XmitIter = sRawData.begin();
		tempXmitIter = XmitIter;

	//Show the window
		ImGui::OpenPopup("File Transfer Progress");
}

void FileTransfer_BeginUploadFromMemory(std::string *str)
{
	//Copy data into our buffer
		sRawData = *str;

	//Calculate the MD5
		sMD5 = md5(sRawData);

	//Setup the transfer
		bUseCRC = 0;
		packetno = 0;
		bTransferComplete = false;
		iFileTransferState = 0;
		bMD5Sent = false;
		XmitIter = sRawData.begin();
		tempXmitIter = XmitIter;

	//Show the window
		ImGui::OpenPopup("File Transfer Progress");
}


static void FileTransfer_StateMachine()
{
	switch (iFileTransferState)
	{
		case 0:	//Tell Carvera we're sending a file
			Comms_SendString("upload sd/gcodes/Clout.nc");
			LastMsgAttempt = steady_clock::now();
			bFileTransferInProgress = true;
			iFileTransferState++;
		break;

		case 1:	//We'll get a response telling us if we're using CRC or not
			if (Comms_PopMessageOfType(CARVERA_MSG_XMODEM_C))
			{
				bUseCRC = true;
				iFileTransferState++;
			}
			else if (Comms_PopMessageOfType(CARVERA_MSG_XMODEM_NAK))
			{
				bUseCRC = false;
				iFileTransferState++;
			}
			else if (TimeSince_ms(LastMsgAttempt) > 1000) //If we didn't hear anything back, try again
			{
				iFileTransferState = 0;
			}
		break;

		case 2:	//Send the data
		{
			int i = 0;
			if (packetno == 0)	//If this is our first time through, send just the MD5
			{
				i = (int)strlen(sMD5.c_str());
				memcpy(&xbuff[4 + is_stx], sMD5.c_str(), i);
			}
			else
			{
				for (i = 0; i < bufsz; i++)
				{
					if (tempXmitIter == sRawData.end())
					{
						bTransferComplete = true;
						break;
					}

					xbuff[4 + is_stx + i] = tempXmitIter[0];
					tempXmitIter++;
				}
			}

			//Build the header
				xbuff[0] = is_stx ? STX : SOH;
				xbuff[1] = packetno;
				xbuff[2] = ~packetno;
				xbuff[3] = is_stx ? i >> 8 : i;

			if (is_stx)
			{
				xbuff[4] = i & 0xff;
			}

			//Fill up the remainder of the buffer with CTRLZ chars
				if (i < bufsz)
				{
					memset(&xbuff[4 + is_stx + i], CTRLZ, bufsz - i);
				}

			//Compute checksum as appropriate
				if (bUseCRC)
				{
					unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz + 1 + is_stx);
					xbuff[bufsz + 4 + is_stx] = (ccrc >> 8) & 0xFF;
					xbuff[bufsz + 5 + is_stx] = ccrc & 0xFF;
				}
				else
				{
					unsigned char ccks = 0;
					for (i = 3; i < bufsz + 1 + is_stx; ++i)
					{
						ccks += xbuff[i];
					}
					xbuff[bufsz + 4 + is_stx] = ccks;
				}

			//Send the data
				Comms_SendBytes((const char*)xbuff, bufsz + 5 + is_stx + (bUseCRC ? 1 : 0));
				iFileTransferState++;
		}
		break;

		case 3: //Look for a response
			if (Comms_PopMessageOfType(CARVERA_MSG_XMODEM_ACK))
			{
				iFileTransferState = 2;	//Default is to go back for the next packet

				if (packetno == 0 && !bMD5Sent)
					bMD5Sent = true;
				else if (!bTransferComplete)
					XmitIter = tempXmitIter;
				else if (bTransferComplete)
				{
					iFileTransferState = 4; //All done

					//Send end of file transfer alert
						xbuff[0] = EOT;
						Comms_SendBytes((const char*)xbuff, 1);
				}

				packetno++;
			}
			else if (Comms_PopMessageOfType(CARVERA_MSG_XMODEM_NAK))
			{
				tempXmitIter = XmitIter; //Move back and try again

				iFileTransferState = 2;	//Resend it
			}
		break;

		case 4: //Wait for completion
			if (Comms_PopMessageOfType(CARVERA_MSG_UPLOAD_SUCCESS))
			{
				//Console.AddLog(CommsConsole::ITEM_TYPE_NONE, "Expected MD5: %s", sMD5.c_str());
				//Comms_SendString("md5sum sd/gcodes/Clout.nc");
				
				iFileTransferState = XFER_STATE_COMPLETE;
			}
		break;
	}
}

int FileTransfer_DoTransfer()
{
	FileTransfer_StateMachine();

	//Draw the progress
	if (ImGui::BeginPopupModal("File Transfer Progress", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("File transfer in progress");
		ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space before the separator
		ImGui::Separator();
		ImGui::Dummy(ScaledByWindowScale(0.0f, 5)); //Extra empty space before the buttons
		
		float fProgress = (float)(XmitIter - sRawData.begin()) / (float)sRawData.size();
		ImGui::ProgressBar(fProgress, ImVec2(0.0f, 0.0f));

		if (iFileTransferState == XFER_STATE_COMPLETE)
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	if (iFileTransferState == XFER_STATE_COMPLETE)
		bFileTransferInProgress = false;
	else
		bFileTransferInProgress = true;

	if (iFileTransferState == XFER_STATE_COMPLETE)
		return 1;
	else
		return 0;
	//May use a -1 for an abort
}