#pragma once

#include <3ds.h>

class NKMutex
{
	volatile bool _isLocked = false;
public:
	Handle mutex;

	void lock();
	void wait();
	void unlock();
};