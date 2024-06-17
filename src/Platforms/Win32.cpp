
#include "Platforms.h"

#pragma comment(lib, "Ws2_32.lib")

#include "../imgui/imgui.h"

#include "../Comms.h"
#include "../Console.h"


//Mutex stuff
void NewMutex(CloutMutex *Handle)
{
	*Handle = CreateMutex(0, false, 0);
}

void CloseMutex(CloutMutex* Handle)
{
	CloseHandle(*Handle);
}

int WaitForMutex(CloutMutex* Handle, bool bWait) 	//This either blocks forever or returns immediately.  I don't think we ever need a timeout.
{
	int iRes = WaitForSingleObject(*Handle, (bWait ? INFINITE : 0));	//This either blocks forever or returns immediately.  I don't think we ever need a timeout.

	if (iRes == WAIT_OBJECT_0)
		return MUTEX_RESULT_SUCCESS;
	else if (iRes == WAIT_TIMEOUT)
		return MUTEX_RESULT_TIMEOUT;

	return MUTEX_RESULT_ERROR;
}

void ReleaseMutex(CloutMutex* Handle)
{
	ReleaseMutex(*Handle);
}


//Event stuff
void NewEvent(CloutEventHandle *Handle)
{
	*Handle = CreateEventA(NULL, FALSE, FALSE, NULL);
}

void TriggerEvent(CloutEventHandle* Handle)
{	
	SetEvent(*Handle);
}

int IsEventSet(CloutEventHandle* Handle)
{
	int iRes = WaitForSingleObject(*Handle, 0);

	if (iRes == WAIT_OBJECT_0)
		return EVENT_RESULT_SUCCESS;
	else if (iRes == WAIT_TIMEOUT)
		return EVENT_RESULT_TIMEOUT;

	return EVENT_RESULT_ERROR;
}


//Threads
void CloutCreateThread(CloutThreadHandle *handle, void* func)
{
	*handle = 0;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, 0, 0, handle);
}

bool GetThreadMessage(CloutThreadHandle *Thread, CloutThreadMessage *msg)
{
	//CloutThreadHandle *Thread isn't used in Windows, but it's used in Linux

	MSG winmsg;
	if (PeekMessage(&winmsg, nullptr, WM_USER, MSG_MAX, PM_REMOVE))
	{
		msg->iType = winmsg.message;
		msg->Param1 = (void*)winmsg.wParam;
		msg->Param2 = (void*)winmsg.lParam;
		return true;
	}
		
	return false;
}

void SendThreadMessage(CloutThreadHandle* Thread, int Msg, void* Param1, void* Param2)
{
	PostThreadMessageA(*Thread, Msg, (WPARAM)Param1, (LPARAM)Param2);
}

void ThreadSleep(int ms)
{
	Sleep(ms);
}

//Socket stuff
int StartupSockets()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != NO_ERROR)
		return 0;

	return 1;
}

void BuildAddress(sockaddr_in *addr, char* szAddress, char* szPort)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(atoi(szPort));
	addr->sin_addr.s_addr = inet_addr(szAddress);
}

bool DisplaySocketError(const char *s)	//Returns true if it was an acceptable error and we can continue
{
	int iError = WSAGetLastError();

	if (iError != WSAEISCONN)
	{
		Console.AddLog(CommsConsole::ITEM_TYPE_ERROR, "Connection error at %s: %d", s, iError);
		CommsDisconnect();

		return false;
	}

	return true;
}

void CloseSocket(CloutSocket *sock)
{
	closesocket(*sock);
}