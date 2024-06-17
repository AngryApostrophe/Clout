//https://gist.github.com/alexklibisz/7cffdfe90c97986f8393

#include <pthread.h>
#include <sys/eventfd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define PIPE_READ	0
#define PIPE_WRITE	1

#define WM_USER	0	//Linux doesn't use a message loop like windows.  Our internal messages will start at 0

//Mutex stuff
	typedef pthread_mutex_t CloutMutex;

//Event stuff
	typedef int CloutEventHandle;	//eventfd

//Thead stuff
	struct CloutThreadHandle
	{
		pthread_t Handle;
		int Pipe[2];
		int MessageMutex;
	};

	void CloutCreateThread(CloutThreadHandle* handle, void* func (void*));
	#define THREADPROC_DEC void*
	#define THREADPROC_ARG void*

//Socket stuff
	typedef int CloutSocket;

//Random stuff
	int _strnicmp(const char* s1, const char* s2, size_t len);