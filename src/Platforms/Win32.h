
#include <Windows.h>

//Mutex stuff
	typedef HANDLE CloutMutex;

//Event stuff
	typedef HANDLE CloutEventHandle;

//Thead stuff
	typedef DWORD CloutThreadHandle;

	void CloutCreateThread(CloutThreadHandle* handle, void* func);
	#define THREADPROC_DEC DWORD WINAPI
	#define THREADPROC_ARG _In_ LPVOID

//Socket stuff
	typedef SOCKET CloutSocket;

//Serial stuff
	void ListSerialPorts();
	int ConnectToSerialPort(char* szName, HANDLE* hFile);

//Random stuff
	#if defined(_WIN32) || defined(_WIN64)
	/* We are on Windows */
	# define strtok_r strtok_s
	#endif