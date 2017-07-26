#pragma once

#include <3ds.h>

class NKMutex
{
public:
	Handle mutex;
	static NKMutex* createMutex();

	void waitIfLock();
	void unlock();
};