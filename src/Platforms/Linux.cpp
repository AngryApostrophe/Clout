#include "Platforms.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "../imgui/imgui.h"

#include "../Console.h"


//Mutex stuff
void NewMutex(CloutMutex* Handle)
{
}

void CloseMutex(CloutMutex* Handle)
{

}

int WaitForMutex(CloutMutex* Handle, bool bWait)	//This either blocks forever or returns immediately.  I don't think we ever need a timeout.
{
	if (bWait)
	{
		int iRes = pthread_mutex_lock(Handle);
		if (iRes == 0) //Success
			return MUTEX_RESULT_SUCCESS;
	}
	else
	{
		int iRes = pthread_mutex_trylock(Handle);
		if (iRes == 0) //Success
			return MUTEX_RESULT_SUCCESS;

		return MUTEX_RESULT_TIMEOUT;
		//else if (iRes == EBUSY)	//TODO: EBUSY isn't defined for some reason
		//	return MUTEX_RESULT_TIMEOUT;	//It's not available yet
	}

	return MUTEX_RESULT_ERROR;
}

void ReleaseMutex(CloutMutex* Handle)
{
	pthread_mutex_unlock(Handle);
}



//Event stuff
void NewEvent(CloutEventHandle* Handle)
{
	*Handle = eventfd(0, EFD_NONBLOCK);
}

void TriggerEvent(CloutEventHandle* Handle)
{
	uint64_t i = 1;
	write(*Handle, &i, sizeof(i));
}

int IsEventSet(CloutEventHandle* Handle)
{
	uint64_t i;
	if (read(*Handle, &i, sizeof(i)) == 0)
		return EVENT_RESULT_TIMEOUT;
	
	return EVENT_RESULT_SUCCESS;
}


//Threads
void CloutCreateThread(CloutThreadHandle* handle, void* func(void*))
{
	pthread_create(&handle->Handle, NULL, func, 0);

	pipe2(handle->Pipe, O_NONBLOCK);	//The pipe for sending data
	handle->MessageMutex = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE); //Counter of how many messages are in the queue
}

bool GetThreadMessage(CloutThreadHandle* Thread, CloutThreadMessage* msg)
{
	uint64_t i;

	//if (read(Thread->MessageMutex, &i, sizeof(i) > 0))	//If there's something on the queue

	int flags = fcntl(Thread->MessageMutex, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(Thread->MessageMutex, F_SETFL, flags);


	int x = eventfd_read(Thread->MessageMutex, &i);
	if (x == 0)
	{
		int iRes = read(Thread->Pipe[PIPE_READ], msg, sizeof(CloutThreadMessage));	//Then read it.  
		if (iRes > 0)
			return true;
	}

	return false;
}

void SendThreadMessage(CloutThreadHandle* Thread, int Msg, void* Param1, void* Param2)
{
	//PostThreadMessageA(*Thread, Msg, Param1, Param2);
	CloutThreadMessage msgout;
	msgout.iType = Msg;
	msgout.Param1 = Param1;
	msgout.Param2 = Param2;

	write(Thread->Pipe[PIPE_WRITE], &msgout, sizeof(msgout));

	uint64_t i = 1;
	write(Thread->MessageMutex, &i, sizeof(i));

}

void ThreadSleep(int ms)
{
	usleep(ms * 1000);
}


//Socket stuff
int StartupSockets()	//Don't need this in Linux
{
	return 1;
}

void BuildAddress(sockaddr_in* addr, char* szAddress, char* szPort)
{
	bzero((char*)addr, sizeof(sockaddr_in));

	addr->sin_family = AF_INET;

	inet_pton(AF_INET, szAddress, &(addr->sin_addr));
	addr->sin_port = htons(atoi(szPort));
	//addr->sin_addr.s_addr = inet_addr(szAddress);
	
	//bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
}

bool DisplaySocketError(char *s)	//TODO: Get error info
{
	Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection error at %s: &d", s, errno);

	return false;
}

void CloseSocket(CloutSocket* sock)
{
	close(*sock);
}


//Random stuff
int _strnicmp(const char* s1, const char* s2, size_t len)
{
	return strncasecmp(s1, s2, len);
}