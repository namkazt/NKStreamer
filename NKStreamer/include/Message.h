#pragma once
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdint>
#include <3ds.h>
#include <memory>
#include <vector>
#include <map>

//=================================================================================
// MESSAGE PACKET
//=================================================================================
#define START_STREAM_PACKET				20
#define STOP_STREAM_PACKET				21
#define IMAGE_PACKET					30
#define IMAGE_RECEIVED_PACKET			31

#define OPT_VIDEO_QUALITY_PACKET		100
#define OPT_STREAM_MODE_PACKET			101
#define OPT_FPS_MODE_PACKET				102
#define OPT_RECEIVE_INPUT_PROFILE		103
#define OPT_CHANGE_INPUT_PROFILE		104

//=================================================================================
// Each normal input have 1 byte data for
// 1 -> Press key
// 0 -> Release Key
//=================================================================================
#define INPUT_PACKET_FRAME				90


#define INPUT_PACKET_A					70
#define INPUT_PACKET_B					71
#define INPUT_PACKET_X					72
#define INPUT_PACKET_Y					73

#define INPUT_PACKET_L					74
#define INPUT_PACKET_R					75
#define INPUT_PACKET_LZ					76
#define INPUT_PACKET_RZ					77

#define INPUT_PACKET_UP_D				78
#define INPUT_PACKET_DOWN_D				79
#define INPUT_PACKET_LEFT_D				80
#define INPUT_PACKET_RIGHT_D			81

#define INPUT_PACKET_START				82
#define INPUT_PACKET_SELECT				83

//=================================================================================
// Circle pad have 2 bytes for x and y value
// each by from 0 -> 255 split to 2 part
//----------------------------------------
// 0 <-> 127 || 128 <-> 255
// can be convert to
//   -1      0       1
//=================================================================================
#define INPUT_PACKET_CIRCLE				84
#define INPUT_PACKET_CIRCLE_PRO			85



//=================================================================================
// PACKET OPTION
//=================================================================================
struct FramePiece_t
{
	u8 index = 0;
	int pieceSize = 0;
	void *pieceData = nullptr;
	void Release()
	{
		if (pieceData != nullptr) free(pieceData);
	}
};


struct Frame_t
{
	int w;
	int h;
	void *texData = nullptr;
	int frameIndex;
	int pieceCount;
	int frameSize;
	bool complete = false;
	std::map<int, FramePiece_t*> pieces;
	void Release()
	{
		if (texData != nullptr) free(texData);
	}
};

class Message
{
public:
	u8 MessageCode;
	u32 TotalSize;
	u32 CSize;
	char * Content;
	u32 Received;

public:
	Message();
	~Message();

	int ReadMessageFromData(const char* data, size_t size);

	int GetFrameIndex();
};


#endif