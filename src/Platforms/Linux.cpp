#include "Platforms.h"


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
		else if (iRes == EBUSY)
			return MUTEX_RESULT_TIMEOUT;	//It's not available yet
	}

	return MUTEX_RESULT_ERROR;
}

void ReleaseMutex(CloutMutex* Handle)
{
	pthread_mutex_unlock(Handle);
}