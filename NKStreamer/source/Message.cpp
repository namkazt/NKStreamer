#include "Message.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

Message::Message()
{
	MessageCode = 0;
	TotalSize = 0;
	CSize = 0;
	Received = 0;
}

Message::~Message()
{
	if (Content != nullptr) free(Content);
}

int Message::ReadMessageFromData(const char* data, size_t size)
{
	if(Received == 0)
	{
		//---------------------------------------
		// start read header of message
		//---------------------------------------
		MessageCode = data[0];
		TotalSize = (u32)data[1] |
			(u32)data[2] << 8 |
			(u32)data[3] << 16 |
			(u32)data[4] << 24;
		Received += 5;
		CSize = TotalSize - 5;
		////---------------------------------------
		if(CSize <= 0)
		{
			if (TotalSize == size) return 0;
			int cutOffset = size - TotalSize;
			return cutOffset;
		}
		////---------------------------------------
		Content = (char*)malloc(sizeof(char) * CSize);
		if(TotalSize >= size)
		{
			memcpy(Content, data + 5, size - 5);
			Received += size - 5;
			return TotalSize > size ? -1 : 0;
		}else
		{
			memcpy(Content, data + 5, CSize);
			Received += CSize;
			return CSize;
		}
	}else
	{
		if(size + Received >= TotalSize)
		{
			int cutOffset = TotalSize - Received;
			memcpy(Content + (Received-5), data, cutOffset);
			Received += cutOffset;
			
			if (size - cutOffset == 0) return 0;
			return cutOffset;
		}else
		{
			memcpy(Content + (Received - 5), data, size);
			Received += size;
		}
	}
	return -1;
}

int Message::GetFrameIndex()
{
	if(CSize >= 9 && Received >= 9)
	{
		int frameIndex = (u32)Content[0] |
			(u32)Content[1] << 8 |
			(u32)Content[2] << 16 |
			(u32)Content[3] << 24;
		return frameIndex;
	}
	return -1;
}
