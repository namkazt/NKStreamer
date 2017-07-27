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

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

#define SCENE_EXIT		-999
#define SCENE_DISMISS	-1
#define SCENE_OK		0	


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
	UIHelper();

	u32 *SOC_buffer;

	u32 downEvent;
	u32 heldEvent;
	u32 upEvent;

	bool isUsingKeyboard;
	SwkbdButton keyboardButton;
	SwkbdState keyboardState;
	char keyboardBuffer[60];

	touchPosition touch;
	touchPosition _lastTouch;
	bool _isTouchEnd = false;
	bool _isTouchStart = false;

	void InitServices();
	void EndServices();

	void LoadFonts();


	sf2d_texture* loadJPEGBuffer_Turbo(const void *buffer, unsigned long buffer_size, sf2d_place place);
	sf2d_texture* loadJPEGBuffer(const void *buffer, unsigned long buffer_size, sf2d_place place);
	//---------------------------------
	// Input
	void StartInput();
	void EndInput();

	static SwkbdCallbackResult IPInputValidate(void* user, const char** ppMessage, const char* text, size_t textlen);
	//---------------------------------
	// Input Button
	bool PressStart();
	bool PressSelect();
	bool PressAButton();
	bool PressBButton();
	bool PressXButton();
	bool PressYButton();

	bool PressLButton();
	bool PressRButton();
	bool PressLZButton();
	bool PressRZButton();

	bool PressDpadUPButton();
	bool PressDpadDOWNButton();
	bool PressDpadLEFTButton();
	bool PressDpadRIGHTButton();


	bool ReleaseStart() const { return upEvent & KEY_START; };
	bool ReleaseSelect() const { return upEvent & KEY_SELECT; };
	bool ReleaseAButton() const { return upEvent & KEY_A; };
	bool ReleaseBButton()const { return upEvent & KEY_B; };
	bool ReleaseXButton() const { return upEvent & KEY_X; };
	bool ReleaseYButton() const { return upEvent & KEY_Y; };

	bool ReleaseLButton()const { return upEvent & KEY_L; };
	bool ReleaseRButton()const { return upEvent & KEY_R; };
	bool ReleaseLZButton()const { return upEvent & KEY_ZL; };
	bool ReleaseRZButton() const { return upEvent & KEY_ZR; };

	bool ReleaseDpadUPButton() const { return upEvent & KEY_DUP; };
	bool ReleaseDpadDOWNButton() const { return upEvent & KEY_DDOWN; };
	bool ReleaseDpadLEFTButton() const { return upEvent & KEY_DLEFT; };
	bool ReleaseDpadRIGHTButton() const { return upEvent & KEY_DRIGHT; };

	//---------------------------------
	// GUI Draw
	void StartGUI( u32 w, u32 h);
	void EndGUI();

	void GUI_Panel(u32 x, u32 y, u32 w, u32 h, const char * text, int size = 13, u32 color = UIHelper::Col_Primary);

	bool GUI_Button(u32 x, u32 y, u32 w, u32 h, const char * text, int size = 13);
	bool GUI_TextInput(u32 x, u32 y, u32 w, u32 h, const char * text, const char* placeHolder, int size = 13, CallbackVoid callback = nullptr);

	//---------------------------------
	// GUI Event
	bool IsContainTouch(u32 x, u32 y, u32 w, u32 h);

	//---------------------------------
	// GUI Variables
	char _logInformation[256] = "";
	std::string _ipInput = "192.168.31.222";
	std::string _portInput = "1234";

	std::string _inputTextBackupValue = "";

	//---------------------------------
	// GUI Scene
	bool isStreaming = false;
	int SceneMain(SocketManager* sockMng, int THREADNUM, u32 prio, std::vector<SocketManager*> *socketThreads);
	int SceneInputNumber(const char* label, std::string *inputvalue, CallbackVoid_1 onCancel = nullptr, CallbackVoid_1 onOk = nullptr);

	bool HasPopup() { return popupQueue.size() > 0; }
	CallbackVoid GetPopup() { return popupQueue[popupQueue.size() - 1]; }
	void ClosePopup() { popupQueue.erase(popupQueue.end() - 1); }
	u32 GetRGBA(char r, char g, char b, char a);
};


#endif