#pragma once

#ifndef _SOCKETMANAGER_H_
#define _SOCKETMANAGER_H_

#include <3ds.h>
#include <string.h>
#include "Message.h"
#include <functional>
#include <queue>
#include "NKMutex.h"

typedef std::function<void(uint32_t frame, void* arg)> NewFrameCallback;
typedef std::function<void(void* arg)> ConnectCallback;



class SocketManager
{
public:
	u32 sock = -1;

	struct sockaddr_in server;
	struct in_addr ipAddress;
	Message *message = nullptr;
	Thread sockThread;

	NKMutex* mutex = nullptr;
	std::queue<Message*> queueFrames;

	bool isConnected = false;
	int sharedConnectionState = 0;

	volatile int FrameIndex = 0;
	NewFrameCallback OnNewFrameCallback = nullptr;
	ConnectCallback OnConnectSuccess = nullptr;
	ConnectCallback OnConnectFail = nullptr;

	bool CheckRet(u32 ret, const char * message);
	void ProcessData(const char* data, size_t size);

	SocketManager();

	bool IsBusy = false;
	bool SetServer(const char* ip, uint16_t port);
	bool Connect();
	void StartThread(s32 prio);
	void EndThread();

	void Listen();
	bool SendMessage(void* message, size_t size);

	void Close();

	void SendMessageWithCode(char code);
	void SendInputMesssage(char code, bool up);
};

#endif