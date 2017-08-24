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
#include "ThreadManager.h"
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
#include "ConfigManager.h"

int main()
{
	//---------------------------------------------
	// Variables
	//---------------------------------------------
	
	//---------------------------------------------
	// Init UI Helper for services
	//---------------------------------------------
	UIHelper* uiHelper = UIHelper::R();
	uiHelper->InitServices();
	//---------------------------------------------
	// Init SF2D
	//---------------------------------------------
	sf2d_init();
	sf2d_set_clear_color(UIHelper::Col_Text_Light);
	sf2d_set_vblank_wait(1);
	// Font loading
	sftd_init();
	uiHelper->LoadFonts();
	// Init config
	ConfigManager::R()->InitConfig();
	//---------------------------------------------
	//gfxSetDoubleBuffering(GFX_TOP, true);
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
	static sf2d_texture* frameTexture = nullptr;
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
		// && uiHelper->MainSock()->isServerReceivedInput
		if (ThreadManager::R()->MainState() >= 3)
		{
			if (uiHelper->downEvent != uiHelper->downEventOld || uiHelper->upEvent != uiHelper->upEventOld || uiHelper->IsChangeCirclePad()) {
				ThreadManager::R()->MainWorker->SendInputPacketMesssage(uiHelper->downEvent, uiHelper->upEvent, uiHelper->GetCirclePosition());
			}
		}
		//-------------------------------------------------------
		// draw
		//-------------------------------------------------------

#ifndef _CONSOLE_DEBUGING
		if (ThreadManager::R()->MainState() >= 3 && uiHelper->isStreaming)
		{
			if(ThreadManager::R()->LastFrame != ThreadManager::R()->WorkingFrame)
			{
				if(frameTexture != nullptr)
				{
					sf2d_free_texture(frameTexture);
					frameTexture = nullptr;
				}
				frameTexture = ThreadManager::R()->GetNewestFrameThatCompleted();
			}
				
			if (frameTexture != nullptr) {
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
				sf2d_draw_texture(frameTexture, 400 / 2 - frameTexture->width / 2, 240 / 2 - frameTexture->height / 2);
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
		if (uiHelper->MainSockState() >= 3 && uiHelper->isStreaming)
		{

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
			int uiSceneMain = uiHelper->SceneMain(prio);
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

	if(frameTexture != nullptr)
	{
		sf2d_free_texture(frameTexture);
		frameTexture = nullptr;
	}

	//----------------------------------
	// close sock if inited
	ThreadManager::R()->Stop();

	//----------------------------------
	uiHelper->EndServices();
	ConfigManager::R()->Destroy();
	sf2d_fini();
	//----------------------------------
	return 0;
}