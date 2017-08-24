#pragma once
#ifndef _THREAD_MANAGER_H_
#define _THREAD_MANAGER_H_
#include <string>
#include "ConfigManager.h"
#include "SocketManager.h"
#include <sf2d.h>


class FrameCompare
{
public:
	bool operator() (Frame_t* left, Frame_t* right)
	{
		return left->frameIndex > right->frameIndex;
	}
};


class ThreadManager
{
	std::unique_ptr<NKMutex> mutex;
	
public:
	static ThreadManager* R();
	ThreadManager();

	//---------------------------------------------------
	// take care of each frame data
	std::vector<SocketManager*> FramePieceWorkers;
	//---------------------------------------------------
	// take care of input and config
	SocketManager* MainWorker = nullptr;
	//---------------------------------------------------
	void InitWithThreads(std::string ip, std::string port, int prio);
	void Stop();
	void RemoveThread(int index);
	//---------------------------------------------------
	int MainState() const { return MainWorker!= nullptr? MainWorker->sharedConnectionState : 0; };


	sf2d_texture* GetNewestFrameThatCompleted();
	//---------------------------------------------------
	// Mutex
	void MutexWait();
	void MutexLock();
	void MutexUnlock();

	//---------------------------------------------------
	// Frame pieces
	int WorkingFrame = -1;
	int LastFrame = -1;
	//std::priority_queue<Frame_t*, std::vector<Frame_t*>, FrameCompare> FramesMap;
	std::map<int, Frame_t*> FramesMap;
	Frame_t* GetActiveFrame(int FrameIndex);
	void RemoveWorkingFrame();
};

#endif