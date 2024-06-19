#define MAX_DEVICES 10
#define MAX_DATA_LENGTH 8200		//Maximum length of data packet to/from Carvera.  Their XModem protocol uses up to 8192


int CommsInit();
void CommsDisconnect();
unsigned char ProcessIncomingMessage(char* sRecv, const char* sSent=0, bool bShowOnLog = true);

extern bool bCommsConnected;

extern char sConnectedIP[16];
extern unsigned short wConnectedPort;

extern unsigned char bDetectedDevices;
extern char sDetectedDevices[MAX_DEVICES][3][20];

extern bool bFileTransferInProgress;



//Message queue stuff
#define CARVERA_OUTBOUND_STRING		0
#define CARVERA_OUTBOUND_BYTES		1

#define CARVERA_MSG_UNKNOWN			0  //Unknown message.  Check raw data.
#define CARVERA_MSG_STATUS			1  //A typical > reply to our ?
#define CARVERA_MSG_VERSION			2  //Response to "version"
#define CARVERA_MSG_PARSER			3  //Response to "$G". Contains the current GCode parser state
#define CARVERA_MSG_PROBE			4  //A probing even has occured
#define CARVERA_MSG_OK				5  //General response that a command has been received, not necessarily completed
#define CARVERA_MSG_G28				6  //Annoying messages in response to a G28 command:  "G28 means goto..."
#define CARVERA_MSG_ATC_HOMING		7  //ATC is beginning to home the collet.	"Homing atc..."
#define CARVERA_MSG_ATC_HOMED		8  //ATC has homed the collet.	"ATC homed!"
#define CARVERA_MSG_ATC_LOOSED		9  //ATC has opened the collet.	"ATC loosed!" or "Already loosed!"
#define CARVERA_MSG_ATC_CLAMPED		10 //ATC has closed the collet.	"ATC clamped!"
#define CARVERA_MSG_ATC_DONE		11 //ATC has finished changing tools.  "Done ATC"
#define CARVERA_MSG_XMODEM_C		12 //XModem protocol for sending files.  A single "C"
#define CARVERA_MSG_XMODEM_EOT		13 //XModem protocol for sending files.  0x04
#define CARVERA_MSG_XMODEM_ACK		14 //XModem protocol for sending files.  0x06
#define CARVERA_MSG_XMODEM_NAK		15 //XModem protocol for sending files.  0x15
#define CARVERA_MSG_XMODEM_CAN		16 //XModem protocol for sending files.  0x16
#define CARVERA_MSG_UPLOAD_SUCCESS	17 //File upload successful.  "Info: upload success"
#define CARVERA_MSG_PARAMETERS		18 //A bunch of parameters, response to "$#"
#define CARVERA_MSG_DEBUG			19 //Debug parameters, response to "*"	diagnose_command() in https://github.com/faecorrigan/CarveraCommunityFirmware/blob/makeraCommunity/src/modules/utils/simpleshell/SimpleShell.cpp
#define CARVERA_MSG_PLAYING			20 //Beginning to play a gcode file
#define CARVERA_MSG_CLOUTSYNC		21 //A message we insert into custom GCode to sync up with the end of the file


//#ifdef _CHRONO_
struct CarveraMessage
{
	int iType; //Only used for inbound messages
	//std::chrono::steady_clock::time_point Time;		//Time this message was placed on the queue
	unsigned long markpoint;	//Really it's a running counter of sent messages when this message was received
	
	char cData[MAX_DATA_LENGTH];
	int iLen;

	int iProcessed; //After a message is received, it's placed on the recv queue.  If we loop through the main loop a couple times and nobody has removed this message, it is ignored
	bool bHidden; //True if we don't want to show this on the console
};
//#endif





//These are the functions that are called from outside.  TODO: These need to be renamed.  Comms_Disconnect() and CommsDisconnect() is super stupid
void Comms_ConnectDevice(BYTE bDeviceIdx);
void Comms_Disconnect();
unsigned long Comms_SendString(const char* sString, bool bShowInConsole = true);
void Comms_SendBytes(const char *bytes, int iLen);
void Comms_Update();
void Comms_HideReply(int iType);
int Comms_PopMessageOfType(int iType, CarveraMessage *msg = 0, unsigned long EarliestID = 0);

#define MSG_CONNECT_DEVICE		WM_USER+1
#define MSG_DISCONNECT			WM_USER+2
#define MSG_SEND_STRING			WM_USER+3	//Send a string and continue
#define MSG_SEND_STRING_AND_WAIT	WM_USER+4 //Send a string and wait to process a response
#define MSG_MAX MSG_SEND_STRING_AND_WAIT
