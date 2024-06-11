

//Platform-specific stuff
#ifdef WIN32
#include "Win32.h"
#else
#include "Linux.h"
#endif


//Mutex stuff
	#define MUTEX_RESULT_ERROR	0
	#define MUTEX_RESULT_SUCCESS	1
	#define MUTEX_RESULT_TIMEOUT	2

	void NewMutex(CloutMutex* Handle);
	void CloseMutex(CloutMutex* Handle);
	int WaitForMutex(CloutMutex* Handle, bool bWait = false);	//Wait for a mutex to become available.
	void ReleaseMutex(CloutMutex* Handle);

//Event stuff
	#define EVENT_RESULT_ERROR	0
	#define EVENT_RESULT_SUCCESS	1
	#define EVENT_RESULT_TIMEOUT	2

	void NewEvent(CloutEventHandle* Handle);
	void TriggerEvent(CloutEventHandle* Handle);
	int IsEventSet(CloutEventHandle* Handle);

//Threading
	struct CloutThreadMessage
	{
		int iType;
		void* Param1;
		void* Param2;
	};

	bool GetThreadMessage(CloutThreadHandle* Thread, CloutThreadMessage* msg);
	void SendThreadMessage(CloutThreadHandle* Thread, int Msg, void* Param1, void* Param2);
	void ThreadSleep(int ms);

//Socket stuff
	int StartupSockets();
	void BuildAddress(sockaddr_in* addr, char *szAddress, char *szPort);
	bool DisplaySocketError(char *s = "");
	void CloseSocket(CloutSocket *sock);