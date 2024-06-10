#define MAX_DEVICES 10


int CommsInit();
void CommsDisconnect();
bool SendCommand(const char *c, bool bShowOnLog = true);
bool SendCommandAndWait(const char* c, bool bShowOnLog = true);
unsigned char ProcessIncomingMessage(char* sRecv, const char* sSent=0, bool bShowOnLog = true);

extern bool bCommsConnected;

extern char sConnectedIP[16];
extern unsigned short wConnectedPort;

extern unsigned char bDetectedDevices;
extern char sDetectedDevices[MAX_DEVICES][3][20];

extern CloutEventHandle hProbeResponseEvent;
extern char* sProbeReplyMessage;

extern bool bOperationRunning;

//These are the functions that are called from outside.  TODO: These need to be renamed.  Comms_Disconnect() and CommsDisconnect() is super stupid
void Comms_ConnectDevice(BYTE bDeviceIdx);
void Comms_Disconnect();
void Comms_SendString(const char* sString);

#define MSG_CONNECT_DEVICE		WM_USER+1
#define MSG_DISCONNECT			WM_USER+2
#define MSG_SEND_STRING			WM_USER+3	//Send a string and continue
#define MSG_SEND_STRING_AND_WAIT	WM_USER+4 //Send a string and wait to process a response
#define MSG_MAX MSG_SEND_STRING_AND_WAIT
