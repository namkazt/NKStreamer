#pragma once

#include <3ds.h>

class NKMutex
{
public:
	Handle mutex;
	static NKMutex* createMutex();

	static void NK_LOCK(const char* name);
	static void NK_UNLOCK(const char* name);

	void waitIfLock();
	void unlock();
};