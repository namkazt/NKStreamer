#include "SocketManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include "UIHelper.h"

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

#include <setjmp.h>
#include "ConfigManager.h"
#include "ThreadManager.h"

#define STACKSIZE (30 * 1024)

SocketManager::SocketManager()
{
}

void SocketManager::SetServer(std::string ip, std::string port)
{
	mIpAddress = ip;
	mPort = port;
}

bool SocketManager::Connect()
{
	sharedConnectionState = 1;
	return true;
}

void sockThreadMain(void *arg)
{

	SocketManager* sockManager = (SocketManager*)arg;
	sockManager->threadAlive = true;
	//--------------------------------------------------
	u64 sleepDuration = 1000000ULL * 16;
	while (sockManager->runThreads)
	{
		if (sockManager->sharedConnectionState == 4)
		{
			goto exitThread;
		}
		////--------------------------------------------------
		//// define socket
		if (sockManager->isConnected && sockManager->sharedConnectionState == 3)
		{
			//--------------------------------------------------
			// receive data from server
			sockManager->Listen();
		}
		////--------------------------------------------------
		//// Trying to connect
		if (sockManager->sharedConnectionState == 1)
		{
			sockManager->sharedConnectionState = 2;

			////--------------------------------------------------
			//// Trying to get address information
			struct addrinfo hints, *servinfo, *p;
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET; 
			hints.ai_socktype = SOCK_STREAM;

			int rv = getaddrinfo(sockManager->mIpAddress.c_str(), sockManager->mPort.c_str(), &hints, &servinfo);
			if (rv != 0)
			{
				//char errorRv[256] = "";
				//sprintf(errorRv, "[Error] Fail to get address info: %s", gai_strerror(rv));
				//UIHelper::R()->AddLog(errorRv);

				sockManager->sharedConnectionState = 0;
				continue;
			}

			////--------------------------------------------------
			//// define socket
			sockManager->sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sockManager->sock == -1)
			{
				sockManager->sharedConnectionState = 0;
				//UIHelper::R()->AddLog("[Error] Can't create socket.");
				continue;
			}

			// loop thought all result and connect
			for(p = servinfo; p != NULL; p = p->ai_next)
			{
				////--------------------------------------------------
				auto ret = connect(sockManager->sock, p->ai_addr, p->ai_addrlen);
				if(ret == -1)
				{
					sockManager->sharedConnectionState = 0;
					//UIHelper::R()->AddLog("[Error] Can't connect to server.");
					//UIHelper::R()->AddLog("[Info] Try to switch next information.");
					continue;
				}

				////--------------------------
				// connected
				sockManager->isConnected = true;
				sockManager->sharedConnectionState = 3;
				break;
			}

			if(sockManager->isConnected)
			{
				//UIHelper::R()->AddLog("[Info] Connect to server successfully.");
				// set socket to non blocking so it will not wait on recv and send
				//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
				fcntl(sockManager->sock, F_SETFL, fcntl(sockManager->sock, F_GETFL, 0) | O_NONBLOCK);
				if (sockManager->OnConnectSuccess != nullptr) sockManager->OnConnectSuccess(nullptr);
			}else
			{
				//UIHelper::R()->AddLog("[Error] Can't connect to any address parsed.");
				if (sockManager->OnConnectFail != nullptr) {
					sockManager->OnConnectFail(nullptr);
				}
				sockManager->Close();
				goto exitThread;
			}
		}
		svcSleepThread(sleepDuration);
	}

exitThread:
	//UIHelper::R()->AddLog("[Info] Thread function end.");
	sockManager->sharedConnectionState = 0;
	sockManager->Close();
}

void SocketManager::StartThread(s32 prio, int order)
{
	// do not start new thread if old thread still alive ( not it can't anyway ? )
	if (threadAlive) return;
	sockThread = threadCreate(sockThreadMain, this, STACKSIZE, prio - 1, -2, true);
}

void SocketManager::EndThread()
{
	if (!threadAlive) return;

	// we don't use this (runThreads) to check because threadJoin will wait. so better use threadAlive to check it after Join success.
	runThreads = false;   
	sharedConnectionState = 4;
	threadAlive = false; // TODO: remove it later ?
	sockThread = nullptr;
	// unlock thread mutex
	/*if(mutex) mutex->unlock();*/
	//UIHelper::R()->AddLog(" >>> Exit thread successfully.");
}

void SocketManager::ForceEndThread()
{
	threadExit(-1);
}

void SocketManager::Listen()
{
	if (!isConnected) {
		return;
	}

	const int bufferSize = 1024 * 10;
	std::shared_ptr<char> buffer(new char[bufferSize]);
	//----------------------------------
	int recvAmount = recv(sock, buffer.get(), bufferSize, 0);
	if (recvAmount < 0) {
		if (errno != EWOULDBLOCK)
		{
			EndThread();
		}
		return;
	}
	
	//----------------------------------
	// Process data
	this->ProcessData(buffer.get(), recvAmount);
	
}

void SocketManager::ProcessData(const char* data, size_t size)
{
	if (size == 0) return;
	if (message == nullptr)
		message = new Message();
	int cutOffset = message->ReadMessageFromData(data, size);
	if (cutOffset >= 0)
	{
		
		//printf("[Recv] code: %d size: %d.\n", message->MessageCode, message->ContentSize);
		switch (message->MessageCode)
		{
		case IMAGE_PACKET:
		{
			//--------------------------------
			// get frame index of this message
			FrameIndex = (u32)message->Content[0] |
				(u32)message->Content[1] << 8 |
				(u32)message->Content[2] << 16 |
				(u32)message->Content[3] << 24;

			//--------------------------------
			// get piece info
			u8 totalPieces = (u8)message->Content[4];
			u8 pieceIndex = (u8)message->Content[5];

			//--------------------------------
			ThreadManager::R()->MutexWait();
			ThreadManager::R()->MutexLock();

			//--------------------------------
			// get active frame to work on it
			Frame_t* frame = ThreadManager::R()->GetActiveFrame(FrameIndex);
			if (frame->pieceCount == -1)
				frame->pieceCount = totalPieces;

			//-------------------------------
			// init new frame pieces
			FramePiece_t * framePiece = new FramePiece_t();
			framePiece->index = pieceIndex;
			framePiece->pieceSize = message->CSize - 6;
			framePiece->pieceData = (char*)malloc(framePiece->pieceSize);
			memcpy(framePiece->pieceData, message->Content + 6, framePiece->pieceSize);
			frame->pieces.insert(std::pair<int, FramePiece_t*>(pieceIndex, framePiece));

			//-------------------------------
			frame->frameSize += message->CSize - 6;
			//-------------------------------
			if (frame->pieces.size() == totalPieces)
			{
				frame->complete = true;
				//------------------------------------------------
				// merge all pieces together
				frame->texData = (u8*)malloc(frame->frameSize);
				int cursor = 0;
				for(int i = 0; i < frame->pieceCount; i++)
				{
					memcpy(frame->texData + cursor, frame->pieces[i]->pieceData, frame->pieces[i]->pieceSize);
					cursor += frame->pieces[i]->pieceSize;
					//--------------------------------------------
					// release right after copy data
					frame->pieces[i]->Release();
					delete frame->pieces[i];
					frame->pieces[i] = nullptr;
				}
				//--------------------------------
				// update working frame if frameindex > working frame
				if(FrameIndex > ThreadManager::R()->WorkingFrame)
				{
					//------------------------------------------------
					// remove last working frame
					ThreadManager::R()->RemoveWorkingFrame();
					//------------------------------------------------
					ThreadManager::R()->WorkingFrame = FrameIndex;
				}
			}
			ThreadManager::R()->MutexUnlock();
			//------------------------------------------------
			// send as received
			SendMessageWithCode(IMAGE_RECEIVED_PACKET);

			break;
		}
		case INPUT_PACKET_FRAME:
		{
			
			this->isServerReceivedInput = true;
			break;
		}
		case OPT_RECEIVE_INPUT_PROFILE:
		{
			// clean up
			std::vector<std::string>().swap(UIHelper::R()->_inputProfileName);
			if(message->CSize > 0)
			{
				uint32_t cursor = 0;
				uint32_t contentLeft = message->CSize;
				do
				{
					UIHelper::R()->AddLog("Create new profile name");

					uint8_t profileNameSize = (uint8_t)message->Content[cursor];
					cursor++;
					if(contentLeft - 1 >= profileNameSize)
					{
						char profileName[256] = "";
						memcpy(profileName, message->Content + cursor, profileNameSize);
						std::string name = std::string(profileName);
						name.shrink_to_fit();
						if(name != "")
							UIHelper::R()->_inputProfileName.push_back(name);
						cursor += profileNameSize;
						contentLeft -= profileNameSize;
						if(contentLeft <= 0) break;
					}else
						break;
				} while (true);
			}

			break;
		}
		default:
		{
			//UIHelper::R()->AddLog("[Error] invalid message.");
			break;
		}
		}
		//--------------------------------------------------
		if(message != nullptr)
		{
			delete message;
			message = nullptr;
		}
			
		message = new Message();
		//--------------------------------------------------

		if (cutOffset > 0)
		{
			const int amountLeft = size - cutOffset;
			char newBuffer[amountLeft];
			bzero(newBuffer, amountLeft);

			memcpy(newBuffer, data + cutOffset, amountLeft);
			// continue process
			this->ProcessData(newBuffer, amountLeft);
		}
	}
}

bool SocketManager::SendMessage(void* message, size_t size)
{
	if (!isConnected) {
		//UIHelper::R()->AddLog("[Error] trying to send message before connect.");
		return false;
	}

	int messageSize = size;
	int totalSent = 0;
	//printf("[Send] Prepare message total size: %i bytes\n", messageSize);
	//--------------------------------
	// process send loop until finish
	do
	{
		int sendAmount = send(sock, message, size, 0);
		if (sendAmount < 0)
		{
			if (!isConnected)
			{
				//UIHelper::R()->AddLog("[Error] socket is closed");
				return false;;
			}
			//UIHelper::R()->AddLog("[Error] something wrong when sending message.");
			return false;
		}
		totalSent += sendAmount;
	} while (totalSent < messageSize);
	return true;
}

void SocketManager::Close()
{
	// if socket is not created so we do no need to close it.
	if (sock == -1) return;
	isConnected = false;
	Result rc = close(sock);
	if (rc != 0)
	{
		//UIHelper::R()->AddLog("[Error] close socket error");
	}
	sock = -1;
}

void SocketManager::SendMessageWithCode(char code)
{
	//printf("Send message code: %d\n", code);

	Message* msg = new (std::nothrow) Message();
	msg->MessageCode = code;
	msg->TotalSize = 5;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*(data++) = msg->MessageCode;
	*(data++) = msg->TotalSize;
	*(data++) = msg->TotalSize >> 8;
	*(data++) = msg->TotalSize >> 16;
	*(data++) = msg->TotalSize >> 24;
	SendMessage(msgContent, msg->TotalSize);

	free(msgContent);
	msgContent = nullptr;
}

void SocketManager::SendInputMesssage(char code, bool up)
{
	Message* msg = new (std::nothrow)  Message();
	msg->MessageCode = code;
	msg->TotalSize = 6;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*(data++) = msg->MessageCode;
	*(data++) = msg->TotalSize;
	*(data++) = msg->TotalSize >> 8;
	*(data++) = msg->TotalSize >> 16;
	*(data++) = msg->TotalSize >> 24;
	if (up) *(data++) = char(2);
	else *(data++) = char(1);
	SendMessage(msgContent, msg->TotalSize);

	free(msgContent);
	msgContent = nullptr;
}

void SocketManager::SendCircleMesssage(char code, circlePosition pos)
{
	Message* msg = new (std::nothrow)  Message();
	msg->MessageCode = code;
	msg->TotalSize = 9;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*(data++) = msg->MessageCode;
	*(data++) = msg->TotalSize;
	*(data++) = msg->TotalSize >> 8;
	*(data++) = msg->TotalSize >> 16;
	*(data++) = msg->TotalSize >> 24;
	//-------------------------------------
	// each dir have 2 bytes
	// form: 
	// first byte: 0 or 1  ( - or + )
	// second byte: 0 <-> 156
	if (pos.dx >= 0) *(data++) = uint8_t(1);
	else  *(data++) = uint8_t(0);
	*(data++) = uint8_t(abs(pos.dx));
	//-------------------------------------
	// each dir have 2 bytes
	if (pos.dy >= 0) *(data++) = uint8_t(1);
	else  *(data++) = uint8_t(0);
	*(data++) = uint8_t(abs(pos.dy));
	//-------------------------------------
	SendMessage(msgContent, msg->TotalSize);
	free(msgContent);
	msgContent = nullptr;
}

void SocketManager::SendOptMessage(char code, int value)
{
	Message* msg = new (std::nothrow)  Message();
	msg->MessageCode = code;
	msg->TotalSize = 9;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*(data++) = msg->MessageCode;
	*(data++) = msg->TotalSize;
	*(data++) = msg->TotalSize >> 8;
	*(data++) = msg->TotalSize >> 16;
	*(data++) = msg->TotalSize >> 24;

	*(data++) = value;
	*(data++) = value >> 8;
	*(data++) = value >> 16;
	*(data++) = value >> 24;

	SendMessage(msgContent, msg->TotalSize);
	free(msgContent);
	msgContent = nullptr;
}

void SocketManager::SendInputPacketMesssage(u32 down, u32 up, circlePosition cPos)
{
	this->isServerReceivedInput = false;
	Message* msg = new (std::nothrow)  Message();
	msg->MessageCode = INPUT_PACKET_FRAME;
	msg->TotalSize = 17; // 5 + 4 + 4 + 4 = 17 bytes
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*(data++) = msg->MessageCode;
	*(data++) = msg->TotalSize;
	*(data++) = msg->TotalSize >> 8;
	*(data++) = msg->TotalSize >> 16;
	*(data++) = msg->TotalSize >> 24;
	//------------------------------------
	// input down
	*(data++) = down;
	*(data++) = down >> 8;
	*(data++) = down >> 16;
	*(data++) = down >> 24;
	//------------------------------------
	// input up
	*(data++) = up;
	*(data++) = up >> 8;
	*(data++) = up >> 16;
	*(data++) = up >> 24;

	//-------------------------------------
	// cPos X
	uint8_t dx = abs(cPos.dx);
	*(data++) = dx;
	*(data++) = (uint8_t)(cPos.dx >= 0 ? 1 : 0);
	//-------------------------------------
	// cPos Y
	uint8_t dy = abs(cPos.dy);
	*(data++) = dy;
	*(data++) = (uint8_t)(cPos.dy >= 0 ? 1 : 0);
	//-------------------------------------
	// send
	SendMessage(msgContent, msg->TotalSize);
	free(msgContent);
	msgContent = nullptr;
}
