#if __INTELLISENSE__
typedef unsigned int __SIZE_TYPE__;
typedef unsigned long __PTRDIFF_TYPE__;
#define __attribute__(q)
#define __builtin_strcmp(a,b) 0
#define __builtin_strlen(a) 0
#define __builtin_memcpy(a,b) 0
#define __builtin_va_list void*
#define __builtin_va_start(a,b)
#define __extension__
#endif

// enable this for debug log console on top
//#define _CONSOLE_DEBUGING


#include <functional>
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <3ds.h>
#include <sf2d.h>
#include <sftd.h>
#include <csignal>

#include "UIHelper.h"
#include "SocketManager.h"

int main()
{
	std::string ipInput = "192.168.31.222";
	//---------------------------------------------
	// Variables
	//---------------------------------------------
	int _maxIfd = -1;
	//---------------------------------------------
	// Init UI Helper for services
	//---------------------------------------------
	UIHelper* uiHelper = new UIHelper();
	uiHelper->InitServices();
	//---------------------------------------------
	// Init SF2D
	//---------------------------------------------
	sf2d_init();
	sf2d_set_clear_color(UIHelper::Col_Text_Light);
	sf2d_set_vblank_wait(0);
	// Font loading
	sftd_init();
	uiHelper->LoadFonts();
	//---------------------------------------------
	gfxSetDoubleBuffering(GFX_TOP, true);

#ifdef _CONSOLE_DEBUGING
	PrintConsole consoleTOp;
	consoleInit(GFX_TOP, &consoleTOp);
#endif
	//---------------------------------------------
	//  Set thread priority
	//---------------------------------------------
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	//---------------------------------------------
	// Init Streaming thread
	//---------------------------------------------
	std::vector<SocketManager*> socketThreads;
	int THREADNUM = 4;
	//------------------
	SocketManager *sockMng = new SocketManager();
	sf2d_texture* frameTexture = nullptr;
	//---------------------------------------------
	//Main loop
	//---------------------------------------------
	while (aptMainLoop())
	{

		uiHelper->StartInput();
		//--------------------------------------------------
		// Note: sockMng is main socket connect to server
		// as defined first sock connect to server is main.
		//--------------------------------------------------
		// INPUT REQUEST
		//-------------------------------------------------------
		if (sockMng->sharedConnectionState >= 3 && sockMng->isConnected)
		{
			//--------------------------------------------------
			// Press
			if (uiHelper->PressAButton()) sockMng->SendInputMesssage(INPUT_PACKET_A, false);
			if (uiHelper->PressBButton()) sockMng->SendInputMesssage(INPUT_PACKET_B, false);
			if (uiHelper->PressXButton()) sockMng->SendInputMesssage(INPUT_PACKET_X, false);
			if (uiHelper->PressYButton()) sockMng->SendInputMesssage(INPUT_PACKET_Y, false);

			if (uiHelper->PressLButton()) sockMng->SendInputMesssage(INPUT_PACKET_L, false);
			if (uiHelper->PressRButton()) sockMng->SendInputMesssage(INPUT_PACKET_R, false);
			if (uiHelper->PressLZButton()) sockMng->SendInputMesssage(INPUT_PACKET_LZ, false);
			if (uiHelper->PressRZButton()) sockMng->SendInputMesssage(INPUT_PACKET_RZ, false);

			if (uiHelper->PressStart()) sockMng->SendInputMesssage(INPUT_PACKET_START, false);
			if (uiHelper->PressSelect()) sockMng->SendInputMesssage(INPUT_PACKET_SELECT, false);

			if (uiHelper->PressDpadUPButton()) sockMng->SendInputMesssage(INPUT_PACKET_UP_D, false);
			if (uiHelper->PressDpadDOWNButton()) sockMng->SendInputMesssage(INPUT_PACKET_DOWN_D, false);
			if (uiHelper->PressDpadLEFTButton()) sockMng->SendInputMesssage(INPUT_PACKET_LEFT_D, false);
			if (uiHelper->PressDpadRIGHTButton()) sockMng->SendInputMesssage(INPUT_PACKET_RIGHT_D, false);

			//--------------------------------------------------
			// Release
			if (uiHelper->ReleaseAButton()) sockMng->SendInputMesssage(INPUT_PACKET_A, true);
			if (uiHelper->ReleaseBButton()) sockMng->SendInputMesssage(INPUT_PACKET_B, true);
			if (uiHelper->ReleaseXButton()) sockMng->SendInputMesssage(INPUT_PACKET_X, true);
			if (uiHelper->ReleaseYButton()) sockMng->SendInputMesssage(INPUT_PACKET_Y, true);

			if (uiHelper->ReleaseLButton()) sockMng->SendInputMesssage(INPUT_PACKET_L, true);
			if (uiHelper->ReleaseRButton()) sockMng->SendInputMesssage(INPUT_PACKET_R, true);
			if (uiHelper->ReleaseLZButton()) sockMng->SendInputMesssage(INPUT_PACKET_LZ, true);
			if (uiHelper->ReleaseRZButton()) sockMng->SendInputMesssage(INPUT_PACKET_RZ, true);

			if (uiHelper->ReleaseStart()) sockMng->SendInputMesssage(INPUT_PACKET_START, true);
			if (uiHelper->ReleaseSelect()) sockMng->SendInputMesssage(INPUT_PACKET_SELECT, true);

			if (uiHelper->ReleaseDpadUPButton()) sockMng->SendInputMesssage(INPUT_PACKET_UP_D, true);
			if (uiHelper->ReleaseDpadDOWNButton()) sockMng->SendInputMesssage(INPUT_PACKET_DOWN_D, true);
			if (uiHelper->ReleaseDpadLEFTButton()) sockMng->SendInputMesssage(INPUT_PACKET_LEFT_D, true);
			if (uiHelper->ReleaseDpadRIGHTButton()) sockMng->SendInputMesssage(INPUT_PACKET_RIGHT_D, true);
		}


		//-------------------------------------------------------
		// draw
		//-------------------------------------------------------

#ifndef _CONSOLE_DEBUGING
		if (sockMng->sharedConnectionState >= 3 && uiHelper->isStreaming)
		{
			int maxFrame = -1;
			Message* msg = nullptr;
			for (int i = 0; i < THREADNUM; i++)
			{
				if (!socketThreads[i]->queueFrames.empty()) {
					printf("[Fc] T:%d.\n", maxFrame);
					if (socketThreads[i]->mutex != nullptr) sockMng->mutex->waitIfLock();
					if (socketThreads[i]->FrameIndex > maxFrame)
					{
						printf(">> [Fc] T:%d | MAX.\n", maxFrame);
						maxFrame = socketThreads[i]->FrameIndex;
						msg = socketThreads[i]->queueFrames.front();
						socketThreads[i]->queueFrames.pop();
					}
				}
			}
			//-------------------------------------
			// only can happen when circle the frame happen
			if (maxFrame < _maxIfd - 10000) _maxIfd = maxFrame;
			//-------------------------------------
			// parse new message
			if (msg != nullptr && maxFrame >= _maxIfd)
			{
				_maxIfd = maxFrame;
				printf(">>Draw frame.\n");
				if (frameTexture != nullptr) sf2d_free_texture(frameTexture);
				frameTexture = uiHelper->loadJPEGBuffer(msg->Content + 4, msg->TotalSize - 9, SF2D_PLACE_RAM);
			}
			//-------------------------------------
			if (frameTexture != nullptr) {
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
				try {
					sf2d_draw_texture(frameTexture, 400 / 2 - frameTexture->width / 2, 240 / 2 - frameTexture->height / 2);
				}
				catch (...)
				{
					printf("Can't draw texture.\n");
				}
				sf2d_end_frame();
			}
		}
		else
		{
			sf2d_start_frame(GFX_TOP, GFX_LEFT);
			sf2d_draw_rectangle(0, 0, 400, 240, UIHelper::Col_Secondary);

			sftd_draw_text_wrap(uiHelper->font_OpenSans, 110, 240 / 2 - 50, UIHelper::Col_Secondary_Dark, 30, 300, "NKStreamer");
			sftd_draw_text_wrap(uiHelper->font_OpenSans, 120, 240 / 2 - 10, UIHelper::Col_Secondary_Light, 15, 400, "PC Desktop Streaming");

			sf2d_end_frame();
		}
#else
		if (sockMng->sharedConnectionState >= 3 && uiHelper->isStreaming)
		{
			int maxFrame = -1;
			Message* msg = nullptr;
			for (int i = 0; i < THREADNUM; i++)
			{
				if (!socketThreads[i]->queueFrames.empty()) {
					printf("[Fc] T:%d.\n", maxFrame);
					if (socketThreads[i]->mutex != nullptr) sockMng->mutex->waitIfLock();
					if (socketThreads[i]->FrameIndex > maxFrame)
					{
						printf(">> [Fc] T:%d | MAX.\n", maxFrame);
						maxFrame = socketThreads[i]->FrameIndex;
						msg = socketThreads[i]->queueFrames.front();
						socketThreads[i]->queueFrames.pop();
					}
				}
			}
			//-------------------------------------
			// only can happen when circle the frame happen
			if (maxFrame < _maxIfd - 10000) _maxIfd = maxFrame;
			//-------------------------------------
			// parse new message
			if (msg != nullptr && maxFrame >= _maxIfd)
			{
				_maxIfd = maxFrame;
				printf(">>Draw frame.\n");
				if (frameTexture != nullptr) sf2d_free_texture(frameTexture);
				frameTexture = uiHelper->loadJPEGBuffer(msg->Content + 4, msg->TotalSize - 9, SF2D_PLACE_RAM);
			}
			//-------------------------------------
			/*if (frameTexture != nullptr) {
			sf2d_start_frame(GFX_TOP, GFX_LEFT);
			try {
			sf2d_draw_texture(frameTexture, 400 / 2 - frameTexture->width / 2, 240 / 2 - frameTexture->height / 2);
			}
			catch (...)
			{
			printf("Can't draw texture.\n");
			}
			sf2d_end_frame();
			}*/
		}
#endif

		//--------------------------------------------------
		// Draw UI
		//-------------------------------------------------------
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);

		uiHelper->StartGUI(320, 240);


		if(uiHelper->HasPopup())
		{
			auto func = uiHelper->GetPopup();
			func();

		}else
		{
			int uiSceneMain = uiHelper->SceneMain(sockMng, THREADNUM, prio, &socketThreads);
			if (uiSceneMain == SCENE_EXIT) break;
		}

		uiHelper->EndGUI();

		sf2d_end_frame();
		//--------------------------------------------------
		// end input
		//--------------------------------------------------
		uiHelper->EndInput();
		//-------------------------------------------------------
		sf2d_swapbuffers();
	}

	//----------------------------------
	// close sock if inited
	if(socketThreads.size() > 0)
	{
		for (int i = 0; i < THREADNUM; i++)
		{
			socketThreads[i]->Close();
			socketThreads[i]->EndThread();
		}
	}
	//----------------------------------
	uiHelper->EndServices();
	sf2d_fini();
	//----------------------------------
	return 0;
}