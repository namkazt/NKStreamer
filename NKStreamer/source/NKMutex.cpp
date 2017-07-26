#include "NKMutex.h"
#include <memory>

NKMutex* NKMutex::createMutex()
{
	NKMutex* ret = new NKMutex();
	svcCreateMutex(&ret->mutex, true);
	return ret;
}

void NKMutex::waitIfLock()
{
	svcWaitSynchronization(mutex, U64_MAX);
}

void NKMutex::unlock()
{
	svcReleaseMutex(mutex);
}
