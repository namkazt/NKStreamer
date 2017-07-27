#include "NKMutex.h"
#include <memory>
#include <map>

static std::map<std::string, NKMutex*> mutexHolder;

NKMutex* NKMutex::createMutex()
{
	NKMutex* ret = new NKMutex();
	svcCreateMutex(&ret->mutex, true);
	return ret;
}

void NKMutex::NK_LOCK(const char* name)
{
	if(mutexHolder.find(name) != mutexHolder.end())
	{
		NKMutex* m = mutexHolder[name];
		m->waitIfLock();
	}else
	{
		NKMutex* m = NKMutex::createMutex();
		mutexHolder[name] = m;
	}
}

void NKMutex::NK_UNLOCK(const char* name)
{
	if (mutexHolder.find(name) != mutexHolder.end())
	{
		NKMutex* m = mutexHolder[name];
		m->unlock();
		delete m;
		mutexHolder.erase(name);
	}
}


void NKMutex::waitIfLock()
{
	svcWaitSynchronization(mutex, U64_MAX);
}

void NKMutex::unlock()
{
	svcReleaseMutex(mutex);
}
