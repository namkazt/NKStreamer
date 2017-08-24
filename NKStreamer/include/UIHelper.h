#pragma once
#ifndef _UI_HELPER_H_
#define _UI_HELPER_H_

#include <3ds.h>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sf2d.h>
#include <string>

#include "FreeSans_ttf.h"
#include "OpenSans_ttf.h"
#include <sftd.h>
#include "SocketManager.h"
#include <cstdarg>
#include <map>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

#define SCENE_EXIT		-999
#define SCENE_DISMISS	-1
#define SCENE_OK		0	

#define MIN_THREAD_NUM  1
#define MAX_THREAD_NUM  10

#define CIRCLE_PAD_DEATH_ZONE 15


typedef std::function<void()> CallbackVoid;
typedef std::function<void(void* arg)> CallbackVoid_1;
typedef std::function<void(void* arg, void* arg2)> CallbackVoid_2;
typedef std::function<void(void* arg, void* arg2, void* arg3)> CallbackVoid_3;

struct Rect
{
	u32 x = 0;
	u32 y = 0;
	u32 w = 0;
	u32 h = 0;
	Rect() {};
	Rect(u32 x, u32 y, u32 w, u32 h)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	}
};

class UIHelper
{
private:
	Rect guiArea;
public:
	std::vector<CallbackVoid> popupQueue;

	//---------------------------------
	// Themes color define
	static const u32 Col_Primary = RGBA8(0x29, 0xb6, 0xf6, 0xff);
	static const u32 Col_Primary_Light = RGBA8(0x73, 0xe8, 0xff, 0xff);
	static const u32 Col_Primary_Dark = RGBA8(0x00, 0x86, 0xc3, 0xff);

	static const u32 Col_Background_Light = RGBA8(0xe0, 0xe0, 0xe0, 0xff);
	static const u32 Col_Background_Dark = RGBA8(0xae, 0xae, 0xae, 0xff);

	static const u32 Col_Secondary = RGBA8(0xe5, 0x73, 0x73, 0xff);
	static const u32 Col_Secondary_Light = RGBA8(0xff, 0xa4, 0xa2, 0xff);
	static const u32 Col_Secondary_Dark = RGBA8(0xaf, 0x44, 0x48, 0xff);

	static const u32 Col_Text_Dark = RGBA8(0x00, 0x00, 0x00, 0xff);
	static const u32 Col_Text_Light = RGBA8(0xff, 0xff, 0xff, 0xff);

	sftd_font *font_FreeSans;
	sftd_font *font_OpenSans;

public:
	static UIHelper* R();
	UIHelper();

	u32 *SOC_buffer;

	u32 downEvent = 0;
	u32 heldEvent = 0;
	u32 upEvent = 0;

	u32 downEventOld = 0;
	u32 heldEventOld = 0;
	u32 upEventOld = 0;

	circlePosition circlePos;
	circlePosition circlePosOld;

	touchPosition touch;
	touchPosition _lastTouch;
	bool _isTouchEnd = false;
	bool _isTouchStart = false;

	void InitServices();
	void EndServices();

	void LoadFonts();


	sf2d_texture* loadJPEGBuffer_Turbo(const void *buffer, unsigned long buffer_size, sf2d_place place);
	void getJPEGBuffer_Turbo(const void *buffer, Frame_t* frame, unsigned long buffer_size, sf2d_place place);
	sf2d_texture* loadJPEGBuffer(const void *buffer, unsigned long buffer_size, sf2d_place place);
	void fillJPEGBuffer(sf2d_texture *frameTexture, const void *buffer, unsigned long buffer_size);
	//sf2d_texture* loadWEBPBuffer(const void *buffer, unsigned long buffer_size, sf2d_place place);
	//---------------------------------
	// Input
	void StartInput();
	void EndInput();

	static SwkbdCallbackResult IPInputValidate(void* user, const char** ppMessage, const char* text, size_t textlen);

	bool IsChangeCirclePad() const { return abs(circlePos.dx) > CIRCLE_PAD_DEATH_ZONE || abs(circlePos.dy) > CIRCLE_PAD_DEATH_ZONE; };
	circlePosition GetCirclePosition() { return circlePos; };
	//---------------------------------
	// GUI Draw
	void StartGUI( u32 w, u32 h);
	void EndGUI();

	bool GUI_Panel(u32 x, u32 y, u32 w, u32 h, const char * text, int size = 13, u32 color = UIHelper::Col_Primary);
	bool GUI_Button(u32 x, u32 y, u32 w, u32 h, const char * text, int size = 13);
	bool GUI_Label(u32 x, u32 y, const char * text, int size = 13);
	bool GUI_TextInput(u32 x, u32 y, u32 w, u32 h, const char * text, const char* placeHolder, int size = 13, CallbackVoid callback = nullptr);
	std::string _inputTextBackupValue = "";
	std::string _templateInputText = "";
	//---------------------------------
	// GUI Event
	bool IsContainTouch(u32 x, u32 y, u32 w, u32 h);

	//---------------------------------
	// GUI Variables
	std::vector<std::string> inlineLogs;
	const int maxLogSize = 200;
	std::unique_ptr<NKMutex> logMutex;
	void AddLog(std::string log)
	{
		if (inlineLogs.size() > maxLogSize) inlineLogs.erase(inlineLogs.begin());
		inlineLogs.push_back(log);
	}
	const char* LastLog()
	{
		if (inlineLogs.size() > 0) 
			return inlineLogs[inlineLogs.size() - 1].c_str();
		else 
			return "";
	}
	//---------------------------------
	std::string _ipInput = "";
	bool _splitFrameMode = false;
	int _inputProfile = 0;
	std::vector<std::string> _inputProfileName;
	const char* const STREAMING_MODE[2] = { "Game Stream", "Video Stream"};
	int _videoCurrentFrame = -1;
	//---------------------------------
	volatile bool IsMainBusy = false;
	bool isTurnOffBtmScreen = false;
	time_t cdTimeToOffScreen;
	//---------------------------------
	// GUI Scene
	bool isStreaming = false;
	int SceneMain(u32 prio);
	int lastLogYPos = 35;
	int lastLogYDisplay= 0;
	int SceneLogInformation(const char* label, CallbackVoid_1 onOk = nullptr);
	int SceneInputNumber(const char* label, std::string *inputvalue, CallbackVoid_1 onCancel = nullptr, CallbackVoid_1 onOk = nullptr);
	int SceneInputNumber(const char* label, std::string inputvalue, CallbackVoid_1 onCancel = nullptr, CallbackVoid_1 onOk = nullptr);
	int SceneInputOptions(const char* label, int *selectIndex, std::vector<std::string> options, CallbackVoid_1 onOk = nullptr);
	int SceneInputYesNo(const char* label, bool *select, CallbackVoid_1 onOk = nullptr);
	//---------------------------------
	// GUI Popup
	bool HasPopup() { return popupQueue.size() > 0; }
	CallbackVoid GetPopup() { return popupQueue[popupQueue.size() - 1]; }
	void ClosePopup() { popupQueue.erase(popupQueue.end() - 1); }
	u32 GetRGBA(char r, char g, char b, char a);
};


#endif