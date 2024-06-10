
#include "Platforms.h"

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