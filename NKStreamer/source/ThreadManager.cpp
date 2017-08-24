#include "ThreadManager.h"
#include "UIHelper.h"


static ThreadManager* ref;
ThreadManager* ThreadManager::R()
{
	if (ref == nullptr) ref = new ThreadManager();
	return ref;
}
ThreadManager::ThreadManager()
{
}

void ThreadManager::InitWithThreads(std::string ip, std::string port, int prio)
{
	//-------------------------------------------
	// init main thread
	MainWorker = new SocketManager();
	MainWorker->isMain = true;
	MainWorker->SetServer(ip, port);
	MainWorker->isServerReceivedInput = true;
	MainWorker->StartThread(prio, 999);
	MainWorker->Connect();
	//-------------------------------------------
	// init frame piece workers
	for (int i = 0; i < ConfigManager::R()->_cfg_thread_num; i++)
	{
		SocketManager* sockThread = new SocketManager();
		FramePieceWorkers.push_back(sockThread);
		sockThread->sockIndex = i;
		sockThread->SetServer(ip, port);
		sockThread->StartThread(prio, i);
		sockThread->Connect();
	}
}

void ThreadManager::Stop()
{
	if (FramePieceWorkers.size() == 0) return;
	//-------------------------------------------
	for (int i = 0; i < ConfigManager::R()->_cfg_thread_num; i++)
	{
		FramePieceWorkers[i]->Close();
		FramePieceWorkers[i]->EndThread();
	}
	//-------------------------------------------
	MainWorker->Close();
	MainWorker->EndThread();
	delete MainWorker;
	MainWorker = nullptr;
	//-------------------------------------------
	std::vector<SocketManager*>().swap(FramePieceWorkers);
}

void ThreadManager::RemoveThread(int i)
{
	FramePieceWorkers[i]->EndThread();
	FramePieceWorkers[i] = nullptr;
}

sf2d_texture* ThreadManager::GetNewestFrameThatCompleted()
{
	if (WorkingFrame == -1) return nullptr;
	MutexWait();
	MutexLock();
	if (FramesMap.find(WorkingFrame) == FramesMap.end()) {
		MutexUnlock();
		return nullptr;
	}
	Frame_t* frame = FramesMap[WorkingFrame];
	if(frame == nullptr) {
		MutexUnlock();
		return nullptr;
	}
	sf2d_texture *frameTexture = UIHelper::R()->loadJPEGBuffer_Turbo(frame->texData, frame->frameSize, SF2D_PLACE_RAM);
	LastFrame = WorkingFrame;
	MutexUnlock();
	return frameTexture;
}

void ThreadManager::MutexWait()
{
	if(mutex != nullptr)
		mutex->wait();
}

void ThreadManager::MutexLock()
{
	if (mutex == nullptr)
		mutex.reset(new NKMutex());
	mutex->lock();
}

void ThreadManager::MutexUnlock()
{
	mutex->unlock();
}

Frame_t* ThreadManager::GetActiveFrame(int FrameIndex)
{
	//TODO: need thread safe here.
	// if frame is not create yet
	if(FramesMap.find(FrameIndex) == FramesMap.end())
	{
		Frame_t *frame = new Frame_t();
		frame->w = 400;
		frame->h = 240;
		frame->frameIndex = FrameIndex;
		frame->pieceCount = -1;
		FramesMap.insert(std::pair<int, Frame_t*>(FrameIndex, frame));
		return frame;
	}
	return FramesMap[FrameIndex];
}

void ThreadManager::RemoveWorkingFrame()
{
	if (FramesMap.find(WorkingFrame) != FramesMap.end())
	{
		FramesMap[WorkingFrame]->Release();
		delete FramesMap[WorkingFrame];
		FramesMap.erase(WorkingFrame);
	}
}
