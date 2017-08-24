#pragma once

#ifndef _SOCKETMANAGER_H_
#define _SOCKETMANAGER_H_

#include <3ds.h>
#include <string.h>
#include "Message.h"
#include <functional>
#include <queue>
#include "NKMutex.h"
#include <memory>
#include <map>

typedef std::function<void(uint32_t frame, void* arg)> NewFrameCallback;
typedef std::function<void(void* arg)> ConnectCallback;



class SocketManager
{
public:

	u32 sock = -1;
	std::string mIpAddress;
	std::string mPort;
	Message* message = nullptr;

	Thread sockThread;
	int sockIndex = -1;
	bool threadAlive = false;
	volatile bool runThreads = true;


	bool isConnected = false;
	bool isMain = false;
	bool isServerReceivedInput = false;
	int sharedConnectionState = 0;

	int FrameIndex = 0;
	NewFrameCallback OnNewFrameCallback = nullptr;
	ConnectCallback OnConnectSuccess = nullptr;
	ConnectCallback OnConnectFail = nullptr;

	SocketManager();

	void StartThread(s32 prio, int order);
	void EndThread();
	void ForceEndThread();

	void SetServer(std::string ip, std::string port);
	bool Connect();
	void Listen();
	void ProcessData(const char* data, size_t size);
	void Close();

	bool SendMessage(void* message, size_t size);
	void SendMessageWithCode(char code);

	void SendInputMesssage(char code, bool up);
	void SendCircleMesssage(char code, circlePosition pos);

	void SendOptMessage(char code, int value);

	void SendInputPacketMesssage(u32 down, u32 up, circlePosition cPos);
};

#endif