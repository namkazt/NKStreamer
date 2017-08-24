#include "NKMutex.h"
#include <memory>
#include <map>

void NKMutex::lock()
{
	_isLocked = true;
	this->mutex = Handle();
	svcCreateMutex(&this->mutex, true);
}

void NKMutex::wait()
{
	if(_isLocked)
		svcWaitSynchronization(mutex, U64_MAX);
}

void NKMutex::unlock()
{
	_isLocked = false;
	svcReleaseMutex(mutex);
}
