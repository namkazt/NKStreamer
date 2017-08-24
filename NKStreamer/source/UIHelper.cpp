#include "UIHelper.h"
#include <jpeglib.h>
#include <turbojpeg.h>
#include "ConfigManager.h"
#include "ThreadManager.h"
//#include <webp/decode.h>

static UIHelper* ref;

UIHelper* UIHelper::R()
{
	if (ref == nullptr) ref = new UIHelper();
	return ref;
}

UIHelper::UIHelper()
{
}

void UIHelper::InitServices()
{
	aptInit();

	//---------------------------------------------
	// Init ROMFS
	//---------------------------------------------
	/*Result rc = romfsInit();
	if (rc) {
		printf("romfsInit: %08lX\n", rc);
		return;
	}
	else
	{
		printf("romfs Init Successful!\n");
	}*/
	//---------------------------------------------
	// Init SOCKET
	//---------------------------------------------
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	u32 ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (ret != 0)
	{
		AddLog("[Error] Init SOC services failed.");
		return;
	}
	AddLog("[Info] Init SOC service successfully.");
}

void UIHelper::EndServices()
{
	sftd_free_font(font_FreeSans);
	sftd_free_font(font_OpenSans);

	socExit();
	free(SOC_buffer);
	SOC_buffer = NULL;

	aptExit();
}

void UIHelper::LoadFonts()
{
	font_FreeSans = sftd_load_font_mem(FreeSans_ttf, FreeSans_ttf_size);
	font_OpenSans = sftd_load_font_mem(OpenSans_ttf, OpenSans_ttf_size);
	AddLog("[Info] Loaded fonts successfully.");
}

sf2d_texture* UIHelper::loadJPEGBuffer_Turbo(const void* buffer, unsigned long buffer_size, sf2d_place place)
{
	int jpegSubsamp, width, height;
	tjhandle _jpegDecompressor = tjInitDecompress();
	if (tjDecompressHeader2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, &width, &height, &jpegSubsamp) != 0)
	{
		return nullptr;
	}
	unsigned long texSize = TJBUFSIZE(width, height);
	uint8_t* texData = (uint8_t *)malloc(texSize);
	if (texData == NULL)
	{
		return nullptr;
	}
	if (tjDecompress2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, texData, width, 0/*pitch*/, height, TJPF_RGBA, TJFLAG_FASTDCT) != 0)
	{
		return nullptr;
	}
	sf2d_texture* texture = sf2d_create_texture_mem_RGBA8(texData, width, height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
	if (texData) free(texData);
	tjDestroy(_jpegDecompressor);
	return texture;
}

void UIHelper::getJPEGBuffer_Turbo(const void* buffer, Frame_t* frame, unsigned long buffer_size, sf2d_place place)
{
	if (frame == nullptr) return;
	int jpegSubsamp, width, height;
	tjhandle _jpegDecompressor = tjInitDecompress();
	
	if (tjDecompressHeader2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, &width, &height, &jpegSubsamp) != 0)
	{
		return;
	}
	unsigned long texSize = TJBUFSIZE(width, height);
	frame->texData = (uint8_t *)malloc(texSize);
	if (frame->texData == nullptr)
	{
		return;
	}
	if (tjDecompress2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, frame->texData, width, 0/*pitch*/, height, TJPF_RGBA, TJFLAG_FASTDCT) != 0)
	{
		return;
	}
	tjDestroy(_jpegDecompressor);
}

sf2d_texture* UIHelper::loadJPEGBuffer(const void* srcBuf, unsigned long buffer_size, sf2d_place place)
{
	struct jpeg_decompress_struct jinfo;
	struct jpeg_error_mgr jerr;
	jinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&jinfo);
	jpeg_mem_src(&jinfo, (void *)srcBuf, buffer_size);
	jpeg_read_header(&jinfo, 1);

	int row_bytes = jinfo.image_width * 3;
	jpeg_start_decompress(&jinfo);

	sf2d_texture* texture = sf2d_create_texture(jinfo.image_width, jinfo.image_height, TEXFMT_RGBA8, place);
	if (!texture)
	{
		AddLog("[Error] Can't create Texture [loadJPEGBuffer]");
		//printf("Err : create texture\n");
		return nullptr;
	}

	JSAMPARRAY buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW));
	buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_bytes);

	unsigned int i, color;
	const unsigned char* jpeg_ptr;
	unsigned int* row_ptr = texture->tex.data;

	while (jinfo.output_scanline < jinfo.output_height)
	{
		jpeg_read_scanlines(&jinfo, buffer, 1);
		unsigned int* tex_ptr = row_ptr;
		for (i = 0 , jpeg_ptr = buffer[0]; i < jinfo.output_width; i++ , jpeg_ptr += 3)
		{
			color = jpeg_ptr[0];
			color |= jpeg_ptr[1] << 8;
			color |= jpeg_ptr[2] << 16;
			*(tex_ptr++) = color | 0xFF000000;
		}
		// Next row.
		row_ptr += texture->tex.width;
	}

	jpeg_finish_decompress(&jinfo);

	free(buffer[0]);
	free(buffer);

	sf2d_texture_tile32(texture);

	jpeg_destroy_decompress(&jinfo);
	return texture;
}

u32 morton_interleave_test(u32 x, u32 y)
{
	u32 i = (x & 7) | ((y & 7) << 8); // ---- -210
	i = (i ^ (i << 2)) & 0x1313;      // ---2 --10
	i = (i ^ (i << 1)) & 0x1515;      // ---2 -1-0
	i = (i | (i >> 7)) & 0x3F;
	return i;
}

u32 get_morton_offset_test(u32 x, u32 y, u32 bytes_per_pixel)
{
	u32 i = morton_interleave_test(x, y);
	unsigned int offset = (x & ~7) * 8;
	return (i + offset) * bytes_per_pixel;
}


void UIHelper::fillJPEGBuffer(sf2d_texture* frameTexture, const void* srcBuf, unsigned long buffer_size)
{
	int jpegSubsamp, width, height;
	tjhandle _jpegDecompressor = tjInitDecompress();
	if (tjDecompressHeader2(_jpegDecompressor, (unsigned char*)srcBuf, buffer_size, &width, &height, &jpegSubsamp) != 0)
	{
		return;
	}
	unsigned long texSize = TJBUFSIZE(width, height);
	uint8_t* texData = (uint8_t *)malloc(texSize);
	if (texData == NULL)
	{
		return;
	}
	if (tjDecompress2(_jpegDecompressor, (unsigned char*)srcBuf, buffer_size, texData, width, 0/*pitch*/, height, TJPF_RGBA, TJFLAG_FASTDCT) != 0)
	{
		return;
	}
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			u32 coarse_y = y & ~7;
			u32 offset = get_morton_offset_test(x, y, 4) + coarse_y * frameTexture->tex.width * 4;
			*(u32 *)(frameTexture->tex.data + offset) = __builtin_bswap32(((u32 *)texData)[offset]);
		}
	}
	if (texData) free(texData);
	tjDestroy(_jpegDecompressor);
}

/*
sf2d_texture* UIHelper::loadWEBPBuffer(const void* buffer, unsigned long buffer_size, sf2d_place place)
{
	int w, h;
	WebPGetInfo((const uint8_t*)buffer, buffer_size, &w, &h);

	sf2d_texture* texture = sf2d_create_texture(400, 240, TEXFMT_RGB8, place);
	if (!texture)
	{
		AddLog("[Error] Can't create Texture [loadWEBPBuffer]");
		return nullptr;
	}

	int outSize, outStride;
	uint8_t * ret = WebPDecodeRGBInto((const uint8_t*)buffer, buffer_size, (uint8_t*)texture->tex.data, outSize, outStride);
	if(!ret)
	{
		AddLog("[Error] Fail to decode webp.");
		return nullptr;
	}
	return texture;
}
*/

void UIHelper::StartInput()
{
	hidScanInput();
	downEvent = hidKeysDown();
	heldEvent = hidKeysHeld();
	upEvent = hidKeysUp();
	//-------------------------------------------------
	// touch press control
	circlePos = circlePosition();
	hidCircleRead(&circlePos);
	//-------------------------------------------------
	// touch press control
	touch = touchPosition();
	hidTouchRead(&touch);
	if (touch.px == 0 && touch.py == 0)
	{
		_isTouchEnd = false;
		if (_isTouchStart)
		{
			_isTouchStart = false;
			if (_lastTouch.px == 0 && _lastTouch.py == 0)
			{
				_isTouchEnd = false;
			}
			else
			{
				_isTouchEnd = true;
			}
		}
	}
	else
	{
		_isTouchStart = true;
		_isTouchEnd = false;
	}
}

void UIHelper::EndInput()
{
	_lastTouch = touch;

	downEventOld = downEvent;
	upEventOld = upEvent;
	heldEventOld = heldEvent;

	circlePosOld = circlePos;
}

void UIHelper::StartGUI(u32 w, u32 h)
{
	guiArea.w = w;
	guiArea.h = h;
}

void UIHelper::EndGUI()
{
}

bool UIHelper::GUI_Panel(u32 x, u32 y, u32 w, u32 h, const char* text, int size, u32 color)
{
	sf2d_draw_rectangle(x, y, w, h, color);
	sf2d_draw_rectangle(x, y + h, w, 2, UIHelper::Col_Background_Light);
	//-----------------------
	int tw = sftd_get_text_width(font_FreeSans, size, text);
	int th = size;
	int offx = (w - tw) / 2;
	int offy = (h - th) / 2 - 2;
	sftd_draw_text(font_FreeSans, x + offx, y + offy, UIHelper::Col_Text_Dark, size, text);
	//-----------------------
	if (IsContainTouch(x, y, w, h))
	{
		return true;
	}
	return false;
}

bool UIHelper::GUI_Button(u32 x, u32 y, u32 w, u32 h, const char* text, int size)
{
	sf2d_draw_rectangle(x, y, w, h, UIHelper::Col_Secondary);
	sf2d_draw_rectangle(x, y + h, w, 3, UIHelper::Col_Background_Light);
	//-----------------------
	int tw = sftd_get_text_width(font_OpenSans, size, text);
	int th = size;
	int offx = (w - tw) / 2;
	int offy = (h - th) / 2 - 2;
	sftd_draw_text(font_OpenSans, x + offx, y + offy, UIHelper::Col_Text_Dark, size, text);
	//-----------------------
	if (IsContainTouch(x, y, w, h))
	{
		sf2d_draw_rectangle(x, y, w, h, UIHelper::Col_Secondary_Dark);
		sf2d_draw_rectangle(x, y + h, w, 3, UIHelper::Col_Background_Dark);
		return true;
	}
	return false;
}

bool UIHelper::GUI_Label(u32 x, u32 y, const char* text, int size)
{
	sftd_draw_text(font_OpenSans, x, y, UIHelper::Col_Text_Dark, size, text);
	return false;
}

bool UIHelper::GUI_TextInput(u32 x, u32 y, u32 w, u32 h, const char* text, const char* placeHolder, int size, CallbackVoid callback)
{
	sf2d_draw_rectangle(x, y, w, h, UIHelper::Col_Primary_Light);
	sf2d_draw_rectangle(x, y + h, w, 3, UIHelper::Col_Background_Light);
	//-----------------------
	if (strcmp(text, "") == 0)
	{
		int tw = sftd_get_text_width(font_OpenSans, size, placeHolder);
		int th = size;
		int offx = (w - tw) / 2;
		int offy = (h - th) / 2 - 2;
		sftd_draw_text(font_OpenSans, x + offx, y + offy, UIHelper::Col_Text_Dark, size, placeHolder);
	}
	else
	{
		int tw = sftd_get_text_width(font_OpenSans, size, text);
		int th = size;
		int offx = (w - tw) / 2;
		int offy = (h - th) / 2 - 2;
		sftd_draw_text(font_OpenSans, x + offx, y + offy, UIHelper::Col_Text_Dark, size, text);
	}
	//-----------------------
	if (IsContainTouch(x, y, w, h))
	{
		if (callback)
		{
			popupQueue.push_back(callback);
			_inputTextBackupValue = text;
		}
		return true;
	}
	return false;
}

bool UIHelper::IsContainTouch(u32 x, u32 y, u32 w, u32 h)
{
	if (x < _lastTouch.px && x + w > _lastTouch.px &&
		y < _lastTouch.py && y + h > _lastTouch.py && _isTouchEnd)
	{
		return true;
	}
	return false;
}

int UIHelper::SceneMain(u32 prio)
{
	if(isTurnOffBtmScreen)
	{
		int tw = sftd_get_text_width(font_FreeSans, 13, "Touch on screen to show");
		int th = 13;
		int offx = (320 - tw) / 2;
		int offy = (30 - th) / 2 - 2;
		sftd_draw_text(font_FreeSans, offx, 90 + offy, UIHelper::Col_Text_Dark, 13, "Touch on screen to show");

		float fps = sf2d_get_fps();
		char fpsStr[16];
		sprintf(fpsStr, "FPS: %.2f", fps);
		sftd_draw_text(font_FreeSans, 4, 4, Col_Text_Dark, 12, fpsStr);

		if(downEvent != downEventOld && downEvent & KEY_TOUCH)
		{
			//--------------------------
			// reset touch event
			touch = touchPosition();
			_lastTouch = touch;
			_isTouchStart = false;
			_isTouchEnd = false;
			//--------------------------
			cdTimeToOffScreen = time(NULL);
			//--------------------------
			isTurnOffBtmScreen = false;
		}
		return;
	}

	//--------------------------------------------------
	struct tm* lastTime = gmtime((const time_t *)&cdTimeToOffScreen);
	time_t cdCurrTime = time(NULL);
	struct tm* currTime = gmtime((const time_t *)&cdCurrTime);
	int difTime = currTime->tm_sec - lastTime->tm_sec;
	if(difTime > 10)
	{
		isTurnOffBtmScreen = true;
		return;
	}
	//--------------------------------------------------
	// Top panel
	this->GUI_Panel(0, 0, 320, 25, "NKStreamer - v0.5.7", 13);
	float fps = sf2d_get_fps();
	char fpsStr[16];
	sprintf(fpsStr, "FPS: %.2f", fps);
	sftd_draw_text(font_FreeSans, 4, 4, Col_Text_Dark, 12, fpsStr);
	//--------------------------------------------------
	// IP input
	if (_ipInput.length() == 0)
	{
		_ipInput = std::string(ConfigManager::R()->_cfg_ip);
		ConfigManager::R()->_cfg_ip = _ipInput.c_str();
	}
	if (this->GUI_Button(10, 30, 140, 30, ConfigManager::R()->_cfg_ip, 13))
	{
		_templateInputText = std::string(ConfigManager::R()->_cfg_ip);
		popupQueue.push_back([&]()
			{
				SceneInputNumber("Server IP", &_templateInputText,
				                 [&](void* arg)
				                 {
					                 _templateInputText = "";
					                 ClosePopup();
				                 }, [&](void* arg)
				                 {
					                 _ipInput = std::string(_templateInputText.c_str());
					                 ConfigManager::R()->_cfg_ip = _ipInput.c_str();
					                 ConfigManager::R()->Save();
					                 _templateInputText = "";
					                 ClosePopup();
				                 });
			});
	}
	//--------------------------------------------------
	// Port input
	std::shared_ptr<char> portBuff(new char[10]);
	itoa(ConfigManager::R()->_cfg_port, portBuff.get(), 10);
	if (this->GUI_Button(160, 30, 60, 30, portBuff.get(), 13))
	{
		_templateInputText = std::string(portBuff.get());
		popupQueue.push_back([&]()
			{
				SceneInputNumber("Server Port", &_templateInputText,
				                 [&](void* arg)
				                 {
					                 _templateInputText = "";
					                 ClosePopup();
				                 }, [&](void* arg)
				                 {
					                 ConfigManager::R()->_cfg_port = atoi(_templateInputText.c_str());
					                 _templateInputText = "";
					                 ConfigManager::R()->Save();
					                 ClosePopup();
				                 });
			});
	}
	//--------------------------------------------------
	// Connect button
	int mainSockState = ThreadManager::R()->MainState();
	if (mainSockState == 0)
	{
		if (this->GUI_Button(240, 30, 70, 30, "Connect", 14))
		{
			std::shared_ptr<char> portBuff(new char[10]);
			itoa(ConfigManager::R()->_cfg_port, portBuff.get(), 10);
			ThreadManager::R()->InitWithThreads(ConfigManager::R()->_cfg_ip, portBuff.get(), prio);
		}
	}
	else if (mainSockState == 1 || mainSockState == 2)
	{
		this->GUI_Button(240, 30, 70, 30, "Wait..");
	}
	else if (mainSockState >= 3)
	{
		if (this->GUI_Button(240, 30, 70, 30, "Stop"))
		{
			ThreadManager::R()->Stop();
		}
	}

	//--------------------------------------------------
	// options

	this->GUI_Panel(0, 70, 320, 3, "", 10);
	this->GUI_Panel(200, 75, 3, 175, "", 10, GetRGBA(150, 150, 150, 100));
	if (mainSockState >= 3)
	{
		//--------------------------------------------------
		// start stream button
		if (this->GUI_Button(210, 80, 100, 30, !isStreaming ? "Begin Stream" : "End Stream", 15))
		{
			if (!isStreaming)
			{
				AddLog("[CMD] Start streaming");
				//------------------------------------------------------------
				// send config
				ThreadManager::R()->MainWorker->SendOptMessage(OPT_VIDEO_QUALITY_PACKET, ConfigManager::R()->_cfg_video_quality);
				ThreadManager::R()->MainWorker->SendOptMessage(OPT_STREAM_MODE_PACKET, ConfigManager::R()->_cfg_video_mode);
				ThreadManager::R()->MainWorker->SendMessageWithCode(START_STREAM_PACKET);
				isStreaming = true;
				//isTurnOffBtmScreen = true;
			}
			else
			{
				AddLog("[CMD] Stop streaming");
				ThreadManager::R()->MainWorker->SendMessageWithCode(STOP_STREAM_PACKET);
				isStreaming = false;
			}
		}

		//--------------------------------------------------
		// Option that display on connected
		int _groupY = 80;

		// Video quality
		std::shared_ptr<char> videoQualityBuf(new char[4]);
		itoa(ConfigManager::R()->_cfg_video_quality, videoQualityBuf.get(), 10);
		this->GUI_Label(10, _groupY + 5, "Quality", 13);
		if (this->GUI_Button(95, _groupY, 100, 24, videoQualityBuf.get(), 13))
		{
			_templateInputText = std::string(videoQualityBuf.get());
			popupQueue.push_back([&]()
				{
					SceneInputNumber("Thread Number", &_templateInputText,
					                 [&](void* arg)
					                 {
						                 _templateInputText = "";
						                 ClosePopup();
					                 }, [&](void* arg)
					                 {
						                 ConfigManager::R()->_cfg_video_quality = atoi(_templateInputText.c_str());
						                 if (ConfigManager::R()->_cfg_video_quality < 0) ConfigManager::R()->_cfg_video_quality = 0;
						                 if (ConfigManager::R()->_cfg_video_quality > 100) ConfigManager::R()->_cfg_video_quality = 100;

						                 //TODO: send request change video quality
										 ThreadManager::R()->MainWorker->SendOptMessage(OPT_VIDEO_QUALITY_PACKET, ConfigManager::R()->_cfg_video_quality);
						                 ConfigManager::R()->Save();
						                 _templateInputText = "";
						                 ClosePopup();
					                 });
				});
		}

		// Streaming mode
		_groupY += 30;
		this->GUI_Label(10, _groupY + 5, "Stream Mode", 13);
		if (this->GUI_Button(95, _groupY, 100, 24, STREAMING_MODE[ConfigManager::R()->_cfg_video_mode], 13))
		{
			// reset scroll btn pos to default
			lastLogYPos = 0;
			lastLogYDisplay = 0;
			popupQueue.push_back([&]()
				{
					std::vector<std::string> options;
					options.push_back(STREAMING_MODE[0]);
					options.push_back(STREAMING_MODE[1]);
					SceneInputOptions("Stream Mode", &ConfigManager::R()->_cfg_video_mode, options, [&](void* arg)
					                  {
						                  //if (ConfigManager::R()->_cfg_video_mode == 1)
						                  //{
							                 // // Clean video frames before start using it.

							                 // for (int i = 0; i < ConfigManager::R()->_cfg_thread_num; i++)
							                 // {
								                //  //TODO: need pop all out and delete all message?
								                //  for (int j = 0; j != socketThreads[i]->videoStreamFrames.size(); ++j)
								                //  {
									               //   Message* _m = socketThreads[i]->videoStreamFrames[j];
									               //   delete _m;
								                //  }
								                //  std::vector<Message*>().swap(socketThreads[i]->videoStreamFrames);
							                 // }
							                 // _videoCurrentFrame = -1;
						                  //}
						                  //else
						                  //{
							                 // // Clean all queue frames before using it
							                 // for (int i = 0; i < ConfigManager::R()->_cfg_thread_num; i++)
							                 // {
								                //  //TODO: need pop all out and delete all message?
								                //  while (!socketThreads[i]->queueFrames.empty())
								                //  {
									               //   Frame_t* _m = socketThreads[i]->queueFrames.front();
									               //   socketThreads[i]->queueFrames.pop();
									               //   delete _m;
								                //  }
								                //  std::queue<Frame_t*>().swap(socketThreads[i]->queueFrames);
							                 // }
						                  //}

						                  ////------------------------------------------------------------
						                  //MainSock()->SendOptMessage(OPT_STREAM_MODE_PACKET, ConfigManager::R()->_cfg_video_mode);
						                  //ConfigManager::R()->Save();
						                  ClosePopup();
					                  });
				});
		}

		// Input profile
		_groupY += 30;
		this->GUI_Label(10, _groupY + 5, "Input", 13);
		if (this->GUI_Button(95, _groupY, 100, 24, _inputProfileName.size() > 0 ? _inputProfileName[_inputProfile].c_str() : "Loading...", 13))
		{
			if (_inputProfileName.size() > 0) {
				// reset scroll btn pos to default
				lastLogYPos = 0;
				lastLogYDisplay = 0;
				popupQueue.push_back([&]()
				{
					SceneInputOptions("Input Profile", &_inputProfile, _inputProfileName, [&](void* arg)
					{
						ThreadManager::R()->MainWorker->SendOptMessage(OPT_CHANGE_INPUT_PROFILE, _inputProfile);
						ClosePopup();
					});
				});
			}
		}
	}
	else
	{
		//--------------------------------------------------
		// Option that only display when not connect.
		int _groupY = 80;

		// threads number
		std::shared_ptr<char> threadBuf(new char[4]);
		itoa(ConfigManager::R()->_cfg_thread_num, threadBuf.get(), 10);
		this->GUI_Label(10, _groupY + 5, "Threads", 13);
		if (this->GUI_Button(95, _groupY, 100, 24, threadBuf.get(), 13))
		{
			_templateInputText = std::string(threadBuf.get());
			popupQueue.push_back([&]()
				{
					SceneInputNumber("Thread Number", &_templateInputText,
					                 [&](void* arg)
					                 {
						                 _templateInputText = "";
						                 ClosePopup();
					                 }, [&](void* arg)
					                 {
						                 ConfigManager::R()->_cfg_thread_num = atoi(_templateInputText.c_str());
						                 if (ConfigManager::R()->_cfg_thread_num < MIN_THREAD_NUM) ConfigManager::R()->_cfg_thread_num = MIN_THREAD_NUM;
						                 if (ConfigManager::R()->_cfg_thread_num > MAX_THREAD_NUM) ConfigManager::R()->_cfg_thread_num = MAX_THREAD_NUM;
						                 ConfigManager::R()->Save();
						                 _templateInputText = "";
						                 ClosePopup();
					                 });
				});
		}
	}

	//--------------------------------------------------
	// Exit button
	if (this->GUI_Button(250, 180, 60, 30, "Exit", 16))
	{
		return SCENE_EXIT;
	}
	if (this->GUI_Panel(0, 240 - 20, 320, 20, LastLog(), 10))
	{
		// reset scroll btn pos to default
		lastLogYPos = 0;
		lastLogYDisplay = 0;
		popupQueue.push_back([&]()
			{
				SceneLogInformation("Log Viewer");
			});
	}
	return SCENE_OK;
}

bool InRange(int min, int max, int v) { return v > min && v < max; }

int UIHelper::SceneLogInformation(const char* label, CallbackVoid_1 onOk)
{
	int minYContent = 30;
	int maxYContent = 210;
	//------------------------------------------------
	// Log information
	//TODO: use it lastLogHeight

	int cx = 5;
	int cy = minYContent - lastLogYDisplay;
	int logSpace = 5;
	//------------------------------------------------
	int allLogHeight = 0;
	for (int i = inlineLogs.size() - 1; i >= 0; i--)
	{
		int lW, lH;
		sftd_calc_bounding_box(&lW, &lH, font_FreeSans, 14, 300, inlineLogs[i].c_str());
		sftd_draw_text_wrap(font_FreeSans, cx, cy, Col_Text_Dark, 14, 300, inlineLogs[i].c_str());
		cy += lH + logSpace;
		//------------------------------------------------
		allLogHeight += lH + logSpace;
	}
	// need to show scroll bar
	if (allLogHeight > 170)
	{
		this->GUI_Panel(305, 35, 4, 170, "", 10, Col_Secondary_Dark);
		int controlS = 10;
		//------------------------------
		if (_isTouchStart)
		{
			int min = 0;
			int max = minYContent + allLogHeight - maxYContent + logSpace;
			//------------------------------
			// swipe on content region
			if (InRange(0, 300, _lastTouch.px) && InRange(30, 210, _lastTouch.py))
			{
				int dy = touch.py - _lastTouch.py;
				lastLogYDisplay -= dy;

				if (minYContent - lastLogYDisplay + allLogHeight <= maxYContent + logSpace)
				{
					lastLogYDisplay = max;
				}
				if (minYContent - lastLogYDisplay > minYContent)
				{
					lastLogYDisplay = min;
				}
			}
			//------------------------------
			// scroll bar control
			float percent = (float)lastLogYDisplay / (float)max;
			lastLogYPos = percent * 170.0f;

			if (lastLogYPos < logSpace) lastLogYPos = logSpace;
			else if (lastLogYPos > maxYContent - logSpace) lastLogYPos = maxYContent - logSpace;
		}
		//------------------------------
		sf2d_draw_fill_circle(312 - controlS / 2, minYContent + lastLogYPos, controlS, Col_Secondary_Light);
	}
	//------------------------------------------------
	// Border
	this->GUI_Panel(0, 0, 320, 30, label, 13);
	this->GUI_Panel(0, 210, 320, 30, "", 13);

	if (this->GUI_Button(10, 215, 80, 20, "Close", 16))
	{
		ClosePopup();
	}
}

static const char* const UI_INPUT_VALUE[] = {"0", "1","2", "3","4","5", "6","7","8", "9", ".", ":"};

int UIHelper::SceneInputNumber(const char* label, std::string* inputvalue, CallbackVoid_1 onCancel, CallbackVoid_1 onOk)
{
	if (!inputvalue) return;
	//printf("input: %s \n", inputvalue->c_str());
	//------------------------------------------------
	// input lable
	this->GUI_Panel(0, 0, 320, 30, label, 13);
	//------------------------------------------------
	// input display box
	this->GUI_Panel(20, 40, 200, 30, inputvalue->c_str(), 17);
	if (this->GUI_Button(230, 40, 60, 30, "< DEL", 16))
	{
		if (inputvalue->size() > 0)
		{
			inputvalue->erase(inputvalue->end() - 1);
		}
	}
	//------------------------------------------------
	// draw number input
	int offx = 30;
	int offy = 50;
	int fSize = 16;
	int col = 4;
	int row = 3;
	int x, y = 0;
	for (x = 0; x < col; x++)
	{
		for (y = 0; y < row; y++)
		{
			if (this->GUI_Button(offx + 10 + (x * 60), offy + 40 + (y * 35), 50, 25, UI_INPUT_VALUE[x + y * col], fSize))
			{
				char v = *UI_INPUT_VALUE[x + y * col];
				inputvalue->push_back(v);
			}
		}
	}
	//------------------------------------------------
	// input action
	if (this->GUI_Button(10, 200, 70, 30, "Cancel", 16))
	{
		if (onCancel) onCancel(nullptr);
		return SCENE_DISMISS;
	}
	if (this->GUI_Button(240, 200, 70, 30, "Done", 16))
	{
		if (onOk) onOk(inputvalue);
	}

	return SCENE_OK;
}

int UIHelper::SceneInputNumber(const char* label, std::string inputvalue, CallbackVoid_1 onCancel, CallbackVoid_1 onOk)
{
	//------------------------------------------------
	// input lable
	this->GUI_Panel(0, 0, 320, 30, label, 13);
	//------------------------------------------------
	// input display box
	this->GUI_Panel(20, 40, 200, 30, inputvalue.c_str(), 17);
	if (this->GUI_Button(230, 40, 60, 30, "< DEL", 16))
	{
		if (inputvalue.size() > 0)
		{
			inputvalue.erase(inputvalue.end() - 1);
		}
	}
	//------------------------------------------------
	// draw number input
	int offx = 30;
	int offy = 50;
	int fSize = 16;
	int col = 4;
	int row = 3;
	int x, y = 0;
	for (x = 0; x < col; x++)
	{
		for (y = 0; y < row; y++)
		{
			if (this->GUI_Button(offx + 10 + (x * 60), offy + 40 + (y * 35), 50, 25, UI_INPUT_VALUE[x + y * col], fSize))
			{
				char v = *UI_INPUT_VALUE[x + y * col];
				inputvalue.push_back(v);
			}
		}
	}
	//------------------------------------------------
	// input action
	if (this->GUI_Button(10, 200, 70, 30, "Cancel", 16))
	{
		if (onCancel) onCancel(nullptr);
		return SCENE_DISMISS;
	}
	if (this->GUI_Button(240, 200, 70, 30, "Done", 16))
	{
		if (onOk) onOk((void *)inputvalue.c_str());
	}

	return SCENE_OK;
}

int UIHelper::SceneInputOptions(const char* label, int* selectIndex, std::vector<std::string> options, CallbackVoid_1 onOk)
{
	int minYContent = 30;
	int maxYContent = 210;
	//------------------------------------------------
	// Log information
	int cx = 5;
	int cy = minYContent - lastLogYDisplay + 5;
	int logSpace = 5;
	int allLogHeight = options.size() * (30 + logSpace);
	//------------------------------------------------
	// Draw content here
	for (int i = 0; i < options.size(); i++)
	{
		if (i != *selectIndex)
		{
			if (this->GUI_Panel(15, cy, 295, 30, options[i].c_str(), 12, Col_Secondary_Light))
			{
				*selectIndex = i;
			}
		}
		else
		{
			if (this->GUI_Panel(10, cy, 300, 30, options[i].c_str(), 15, Col_Secondary_Dark))
			{
				*selectIndex = i;
			}
		}
		cy += 30 + logSpace;
	}

	//------------------------------------------------
	// need to show scroll bar
	if (allLogHeight > 170)
	{
		this->GUI_Panel(305, 35, 4, 170, "", 10, Col_Secondary_Dark);
		int controlS = 10;
		//------------------------------
		if (_isTouchStart)
		{
			int min = 0;
			int max = minYContent + allLogHeight - maxYContent + logSpace;
			//------------------------------
			// swipe on content region
			if (InRange(0, 300, _lastTouch.px) && InRange(30, 210, _lastTouch.py))
			{
				int dy = touch.py - _lastTouch.py;
				lastLogYDisplay -= dy;

				if (minYContent - lastLogYDisplay + allLogHeight <= maxYContent + logSpace)
				{
					lastLogYDisplay = max;
				}
				if (minYContent - lastLogYDisplay > minYContent)
				{
					lastLogYDisplay = min;
				}
			}
			//------------------------------
			// scroll bar control
			float percent = (float)lastLogYDisplay / (float)max;
			lastLogYPos = percent * 170.0f;

			if (lastLogYPos < logSpace) lastLogYPos = logSpace;
			else if (lastLogYPos > maxYContent - logSpace) lastLogYPos = maxYContent - logSpace;
		}
		//------------------------------
		sf2d_draw_fill_circle(312 - controlS / 2, minYContent + lastLogYPos, controlS, Col_Secondary_Light);
	}
	//------------------------------------------------
	// Border
	this->GUI_Panel(0, 0, 320, 30, label, 13);
	this->GUI_Panel(0, 210, 320, 30, "", 13);

	//------------------------------------------------
	// input action
	if (this->GUI_Button(240, 200, 70, 30, "Done", 16))
	{
		if (onOk) onOk(nullptr);
	}

	return SCENE_OK;
}

int UIHelper::SceneInputYesNo(const char* label, bool* select, CallbackVoid_1 onOk)
{
	int minYContent = 30;
	int maxYContent = 210;
	int selection = (int)(*select);
	//------------------------------------------------
	// Log information
	int cx = 5;
	int cy = minYContent + 5;
	int logSpace = 5;
	//------------------------------------------------
	// Draw content here
	if (*select)
	{
		if (this->GUI_Panel(15, cy, 295, 30, "NO", 12, Col_Secondary_Light))
		{
			*select = false;
		}
		this->GUI_Panel(10, cy + 30 + logSpace, 300, 30, "YES", 15, Col_Secondary_Dark);
	}
	else
	{
		this->GUI_Panel(10, cy, 300, 30, "NO", 15, Col_Secondary_Dark);
		if (this->GUI_Panel(15, cy + 30 + logSpace, 295, 30, "YES", 12, Col_Secondary_Light))
		{
			*select = true;
		}
	}
	//------------------------------------------------
	// Border
	this->GUI_Panel(0, 0, 320, 30, label, 13);
	this->GUI_Panel(0, 210, 320, 30, "", 13);

	//------------------------------------------------
	// input action
	if (this->GUI_Button(240, 200, 70, 30, "Done", 16))
	{
		if (onOk) onOk(nullptr);
	}

	return SCENE_OK;
}

u32 UIHelper::GetRGBA(char r, char g, char b, char a)
{
	return a << 24 | b << 16 | g << 8 | r;
}
