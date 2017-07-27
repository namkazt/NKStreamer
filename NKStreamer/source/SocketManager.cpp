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

#define STACKSIZE (30 * 1024)
volatile bool runThreads = true;

SocketManager::SocketManager()
{
}

bool SocketManager::SetServer(const char* ip, uint16_t port)
{
	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	struct hostent* host = gethostbyname(ip);
	if (host == nullptr)
	{
		printf("Fail to get host address.\n");
		return false;
	}
	bcopy((char*)host->h_addr, (char*)&server.sin_addr.s_addr, host->h_length);
	server.sin_port = htons(port);
	//--------------------------------------------------
	return true;
}
bool SocketManager::Connect()
{
	sharedConnectionState = 1;
	return true;
}



void sockThreadMain(void *arg)
{
	SocketManager* sockManager = (SocketManager*)arg;
	sockManager->queueFrames = std::queue<Message*>();
	//--------------------------------------------------
	u64 sleepDuration = 1000000ULL * 2;
	int i = 0;
	while (runThreads)
	{
		////--------------------------------------------------
		//// define socket
		if (sockManager->isConnected && sockManager->sharedConnectionState)
		{
			//--------------------------------------------------
			// receive data from server
			try
			{
				sockManager->Listen();
			}
			catch (...)
			{
				printf("[Socket Thread] Unexception Error -> close.\n");
				sockManager->Close();
				return;
			}
		}
		////--------------------------------------------------
		//// Trying to connect
		if (sockManager->sharedConnectionState == 1)
		{
			sockManager->sharedConnectionState = 2;
			////--------------------------------------------------
			//// define socket
			sockManager->sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sockManager->sock == -1)
			{
				sockManager->sharedConnectionState = 0;
				printf("[Error] Can't create socket.\n");
				continue;
			}
			////--------------------------------------------------
			auto ret = connect(sockManager->sock, reinterpret_cast<const sockaddr*>(&sockManager->server), sizeof(sockManager->server));
			// set socket to non blocking so it will not wait on recv and send
			//fcntl(sockManager->sock, F_SETFL, O_NONBLOCK);
			fcntl(sockManager->sock, F_SETFL, fcntl(sockManager->sock, F_GETFL, 0) | O_NONBLOCK);

			if (ret != 0) {
				printf("[Error] Can't connect to server.\n");

				sockManager->sharedConnectionState = 0;
				if (sockManager->OnConnectFail != nullptr) {
					sockManager->OnConnectFail(nullptr);
				}
				sockManager->Close();
				continue;;
			}

			sockManager->isConnected = true;
			sockManager->sharedConnectionState = 3;
			//--------------------------------------------------
			if (sockManager->OnConnectSuccess != nullptr) sockManager->OnConnectSuccess(nullptr);
			//--------------------------------------------------
			printf("[Socket Thread] Connect to server success.\n");
		}

		svcSleepThread(sleepDuration);
	}

}

void SocketManager::StartThread(s32 prio)
{
	printf("[Socket Thread] Start thread.\n");
	sockThread = threadCreate(sockThreadMain, this, STACKSIZE, prio - 1, -2, false);
}

void SocketManager::EndThread()
{
	if (sockThread == nullptr) return;
	printf("[Socket Thread] End thread.\n");
	runThreads = false;
	threadJoin(sockThread, U64_MAX);
	threadFree(sockThread);
	sockThread = nullptr;
}

void SocketManager::Listen()
{
	if (!isConnected) {
		return;
	}
	int bufferSize = 1024 * 10;
	char buffer[bufferSize];
	bzero(buffer, bufferSize);
	//----------------------------------
	int recvAmount = recv(sock, buffer, bufferSize, 0);
	if (recvAmount < 0) {
		if (errno != EWOULDBLOCK)
		{
			printf("Error when recv amount of data: %d", recvAmount);
			// should be close socket on this cause.
			this->Close();
		}
		return;
	}
	//----------------------------------
	// Process data
	this->ProcessData(buffer, recvAmount);
	//----------------------------------
	//bzero(buffer, bufferSize);
	//delete[] buffer;
}

void SocketManager::ProcessData(const char* data, size_t size)
{
	if (message == nullptr)
		message = new Message();
	int cutOffset = message->ReadMessageFromData(data, size);
	if (cutOffset >= 0)
	{
		this->IsBusy = true;
		try
		{
			//printf("[Recv] code: %d size: %d.\n", message->MessageCode, message->ContentSize);
			switch (message->MessageCode)
			{
			case IMAGE_PACKET:
			{
				// create a mutex to lock this queueFrames in main thread from pop it element
				mutex = NKMutex::createMutex();
				//--------------------------------
				// get frame index of this message
				FrameIndex = (u32)message->Content[0] |
					(u32)message->Content[1] << 8 |
					(u32)message->Content[2] << 16 |
					(u32)message->Content[3] << 24;
				// push
				std::queue<Message*> empty;
				std::swap(queueFrames, empty);
				// start push element
				queueFrames.push(message);
				// unlock queue
				mutex->unlock();
				delete mutex;
				mutex = nullptr;


				break;
			}
			}

		}
		catch (...)
		{
			printf("[Error] Parse error message code: %c size: %i.\n", message->MessageCode, message->TotalSize);
		}
		//printf("--> Send received.\n");
		SendMessageWithCode(IMAGE_RECEIVED_PACKET);
		this->IsBusy = false;


		message = new Message();
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
		printf("[Error] Trying to send message after connect.\n");
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
				printf("[Error]socket closed .\n");
				return false;;
			}
			printf("[Error] when send message .\n");
			return false;
		}
		totalSent += sendAmount;
		//printf("[Send] amount: %d - total: %d.\n", sendAmount, totalSent);
	} while (totalSent < messageSize);
	//printf("[Send] sent %d bytes successfully\n", totalSent);
	return true;
}

void SocketManager::Close()
{
	if (isConnected)
	{
		printf("Close Socket.\n");

		sharedConnectionState = 0;
		isConnected = false;
		Result rc = shutdown(sock, SHUT_WR);
		if (rc != 0)
		{
			printf("Shutdown Socket Errror.\n");
		}
		rc = close(sock);
		if (rc != 0)
		{
			printf("Close Socket Errror.\n");
		}
		sock = -1;
	}
	//EndThread();
}

bool SocketManager::CheckRet(u32 ret, const char* message)
{
	if (ret != 0)
	{
		printf(message);
		return false;
	}
	return true;
}


void SocketManager::SendMessageWithCode(char code)
{
	printf("Send message code: %d\n", code);

	Message* msg = new Message();
	msg->MessageCode = code;
	msg->TotalSize = 5;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*data++ = msg->MessageCode;
	*data++ = msg->TotalSize;
	*data++ = msg->TotalSize >> 8;
	*data++ = msg->TotalSize >> 16;
	*data++ = msg->TotalSize >> 24;
	SendMessage(msgContent, msg->TotalSize);

	free(msgContent);
}

void SocketManager::SendInputMesssage(char code, bool up)
{

	printf("Send input code: %d\n", code);

	Message* msg = new Message();
	msg->MessageCode = code;
	msg->TotalSize = 6;
	char* msgContent = (char*)malloc(sizeof(char) * msg->TotalSize);
	char* data = msgContent;
	*data++ = msg->MessageCode;
	*data++ = msg->TotalSize;
	*data++ = msg->TotalSize >> 8;
	*data++ = msg->TotalSize >> 16;
	*data++ = msg->TotalSize >> 24;
	if (up) *data++ = char(2);
	else *data++ = char(1);
	SendMessage(msgContent, msg->TotalSize);

	free(msgContent);
}