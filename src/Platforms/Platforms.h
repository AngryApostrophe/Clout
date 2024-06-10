

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