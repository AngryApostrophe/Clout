#define MAX_DEVICES 10
#define MAX_DATA_LENGTH 500		//Maximum length of data packet to/from Carvera


int CommsInit();
void CommsDisconnect();
unsigned char ProcessIncomingMessage(char* sRecv, const char* sSent=0, bool bShowOnLog = true);

extern bool bCommsConnected;

extern char sConnectedIP[16];
extern unsigned short wConnectedPort;

extern unsigned char bDetectedDevices;
extern char sDetectedDevices[MAX_DEVICES][3][20];

extern bool bOperationRunning;



//Message queue stuff
#define CARVERA_MSG_UNKNOWN 0 //Unknown message.  Check raw data.
#define CARVERA_MSG_STATUS 1 //A typical > reply to our ?
#define CARVERA_MSG_VERSION 2 //Response to "version"
#define CARVERA_MSG_PARSER 3 //Response to "$G". Contains the current GCode parser state
#define CARVERA_MSG_PROBE 4  //A probing even has occured
#define CARVERA_MSG_OK 5 //General response that a command has been received, not necessarily completed
#define CARVERA_MSG_G28 6 //Annoying messages in response to a G28 command:  "G28 means goto..."

//#ifdef _CHRONO_
struct CarveraMessage
{
	int iType; //Only used for inbound messages
	//std::chrono::steady_clock::time_point Time;		//Time this message was placed on the queue
	char cData[MAX_DATA_LENGTH];

	int iProcessed; //After a message is received, it's placed on the recv queue.  If we loop through the main loop a couple times and nobody has removed this message, it is ignored
	bool bHidden; //True if we don't want to show this on the console
};
//#endif





//These are the functions that are called from outside.  TODO: These need to be renamed.  Comms_Disconnect() and CommsDisconnect() is super stupid
void Comms_ConnectDevice(BYTE bDeviceIdx);
void Comms_Disconnect();
void Comms_SendString(const char* sString);
void Comms_Update();
int Comms_PopMessageOfType(CarveraMessage* msg, int iType);

#define MSG_CONNECT_DEVICE		WM_USER+1
#define MSG_DISCONNECT			WM_USER+2
#define MSG_SEND_STRING			WM_USER+3	//Send a string and continue
#define MSG_SEND_STRING_AND_WAIT	WM_USER+4 //Send a string and wait to process a response
#define MSG_MAX MSG_SEND_STRING_AND_WAIT
