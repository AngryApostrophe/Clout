
#include <Windows.h>

//Mutex stuff
	typedef HANDLE CloutMutex;

//Event stuff
	typedef HANDLE CloutEventHandle;

//Thead stuff
	typedef DWORD CloutThreadHandle;

	void CloutCreateThread(CloutThreadHandle* handle, void* func);

//Socket stuff
	typedef SOCKET CloutSocket;

//Random stuff
	#if defined(_WIN32) || defined(_WIN64)
	/* We are on Windows */
	# define strtok_r strtok_s
	#endif