#include "UIHelper.h"
#include <jpeglib.h>
#include <turbojpeg.h>

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
		return;
	}
	printf("Init SOC service successfully.\n");
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
}

sf2d_texture* UIHelper::loadJPEGBuffer_Turbo(const void* buffer, unsigned long buffer_size, sf2d_place place)
{
	int jpegSubsamp, width, height;
	tjhandle _jpegDecompressor = tjInitDecompress();
	if (tjDecompressHeader2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, &width, &height, &jpegSubsamp) != 0)
	{
		printf("Err : tjDecompressHeader2 - %s\n", tjGetErrorStr());
		return nullptr;
	}
	printf("w: %d, h: %d, samp: %d\n", width, height, jpegSubsamp);

	unsigned long texSize = TJBUFSIZE(width, height);
	uint8_t* texData = (uint8_t *)malloc(texSize);
	if (texData == NULL)
	{
		printf("Err : malloc - %s\n", strerror(errno));
		return nullptr;
	}

	if (tjDecompress2(_jpegDecompressor, (unsigned char*)buffer, buffer_size, texData, width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT) != 0)
	{
		printf("Err : tjDecompress2 - %s\n", tjGetErrorStr());
		return nullptr;
	}

	printf("texSize: %d\n", texSize);
	printf("calculated Size: %d\n", width * height * 3);

	sf2d_texture* texture = sf2d_create_texture(width, height, TEXFMT_RGBA8, place);
	if (!texture)
	{
		printf("Err : create texture\n");
		return nullptr;
	}

	unsigned int rowStrice = width * 3;
	unsigned int x, y, color;
	unsigned int* row_ptr = texture->tex.data;

	for (y = 0; y < height; y++)
	{
		unsigned int* tex_ptr = row_ptr;
		for (x = 0; x < rowStrice; x += 3)
		{
			color = texData[x + (rowStrice * y)] |
				texData[x + (rowStrice * y) + 1] << 8 |
				texData[x + (rowStrice * y) + 2] << 16 | 0xFF000000;
			*(tex_ptr++) = color;
		}
		row_ptr += width;
	}

	sf2d_texture_tile32(texture);

	if (texData)
	{
		free(texData);
	}
	tjDestroy(_jpegDecompressor);

	return texture;
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
		printf("Err : create texture\n");
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

void UIHelper::StartInput()
{
	hidScanInput();
	downEvent = hidKeysDown();
	heldEvent = hidKeysHeld();
	upEvent = hidKeysUp();
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
}

bool UIHelper::PressStart()
{
	return downEvent & KEY_START;
}

bool UIHelper::PressSelect()
{
	return downEvent & KEY_SELECT;
}

bool UIHelper::PressAButton()
{
	return downEvent & KEY_A;
}

bool UIHelper::PressBButton()
{
	return downEvent & KEY_B;
}

bool UIHelper::PressXButton()
{
	return downEvent & KEY_X;
}

bool UIHelper::PressYButton()
{
	return downEvent & KEY_Y;
}

bool UIHelper::PressLButton()
{
	return downEvent & KEY_L;
}

bool UIHelper::PressRButton()
{
	return downEvent & KEY_R;
}

bool UIHelper::PressLZButton()
{
	return downEvent & KEY_ZL;
}

bool UIHelper::PressRZButton()
{
	return downEvent & KEY_ZR;
}

bool UIHelper::PressDpadUPButton()
{
	return downEvent & KEY_DUP;
}

bool UIHelper::PressDpadDOWNButton()
{
	return downEvent & KEY_DDOWN;
}

bool UIHelper::PressDpadLEFTButton()
{
	return downEvent & KEY_DLEFT;
}

bool UIHelper::PressDpadRIGHTButton()
{
	return downEvent & KEY_DRIGHT;
}

void UIHelper::StartGUI(u32 w, u32 h)
{
	guiArea.w = w;
	guiArea.h = h;
}

void UIHelper::EndGUI()
{
}

void UIHelper::GUI_Panel(u32 x, u32 y, u32 w, u32 h, const char* text, int size, u32 color)
{
	sf2d_draw_rectangle(x, y, w, h, color);
	sf2d_draw_rectangle(x, y + h, w, 2, UIHelper::Col_Background_Light);
	//-----------------------
	int tw = sftd_get_text_width(font_OpenSans, size, text);
	int th = size;
	int offx = (w - tw) / 2;
	int offy = (h - th) / 2 - 2;
	sftd_draw_text(font_OpenSans, x + offx, y + offy, UIHelper::Col_Text_Dark, size, text);
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

int UIHelper::SceneMain(SocketManager* sockMng, int THREADNUM, u32 prio, std::vector<SocketManager*>* socketThreads)
{
	//--------------------------------------------------
	// Top panel
	this->GUI_Panel(0, 0, 320, 25, "NKStreamer - v0.5.5 - alpha", 13);
	//--------------------------------------------------
	// IP input
	this->GUI_TextInput(10, 30, 140, 30, _ipInput.c_str(), "0.0.0.0", 16, [&]()
	                    {
		                    SceneInputNumber("Server IP", &_ipInput,
		                                     [&](void* arg)
		                                     {
								_ipInput = _inputTextBackupValue;
			                                     ClosePopup();
		                                     }, [&](void* arg)
		                                     {
			                                     ClosePopup();
		                                     });
	                    });
	//--------------------------------------------------
	// Port input
	this->GUI_TextInput(160, 30, 60, 30, _portInput.c_str(), "1234", 16, [&]()
	{
		SceneInputNumber("Server Port", &_portInput,
			[&](void* arg)
		{
			_portInput = _inputTextBackupValue;
			ClosePopup();
		}, [&](void* arg)
		{
			ClosePopup();
		});
	});
	//--------------------------------------------------
	// Connect button
	if (sockMng->sharedConnectionState == 0)
	{
		if (this->GUI_Button(240, 30, 70, 30, "Connect", 14))
		{
			short port = atoi(_portInput.c_str());
			for (int i = 0; i < THREADNUM; i++)
			{
				SocketManager* sockThread = nullptr;
				if (i > 0)
				{
					sockThread = new SocketManager();
				}
				else
				{
					sockThread = sockMng;
				}

				if (!sockThread->SetServer(_ipInput.c_str(), port))
				{
					bzero(_logInformation, 256);
					sprintf(_logInformation, "[Error] Can't parse server infor");
					continue;
				}
				//-------------------------------------------
				sockThread->StartThread(prio, i);
				sockThread->Connect();
				//-------------------------------------------
				socketThreads->push_back(sockThread);
			}
		}
	}
	else if (sockMng->sharedConnectionState == 1 || sockMng->sharedConnectionState == 2)
	{
		if (this->GUI_Button(240, 30, 70, 30, "Wait.."))
		{
			//printf("Connection:  %d?\n", sockMng->sharedConnectionState);
		}
	}
	else if (sockMng->sharedConnectionState >= 3)
	{
		if (this->GUI_Button(240, 30, 70, 30, "Stop"))
		{
			for (int i = 0; i < THREADNUM; i++)
			{
				socketThreads->at(i)->Close();
				socketThreads->at(i)->EndThread();
			}
		}
	}

	//--------------------------------------------------
	// options

	this->GUI_Panel(0, 70, 320, 3, "", 10);
	this->GUI_Panel(200, 75, 3, 175, "", 10, GetRGBA(150, 150, 150, 100));
	if (sockMng->sharedConnectionState >= 3 && sockMng->isConnected)
	{
		//--------------------------------------------------
		// start stream button
		if (this->GUI_Button(210, 80, 100, 30, !isStreaming ? "Begin Stream" : "End Stream", 15))
		{
			if (!isStreaming)
			{
				sockMng->SendMessageWithCode(START_STREAM_PACKET);
				isStreaming = true;
			}
			else
			{
				sockMng->SendMessageWithCode(STOP_STREAM_PACKET);
				isStreaming = false;
			}
		}
	}

	//--------------------------------------------------
	// Exit button
	if (this->GUI_Button(250, 180, 60, 30, "Exit", 16))
	{
		return SCENE_EXIT;
	}
	this->GUI_Panel(0, 240 - 20, 320, 20, _logInformation, 10);
	return SCENE_OK;
}

static const char* const UI_INPUT_VALUE[] = {"0", "1","2", "3","4","5", "6","7","8", "9", ".", ":"};

int UIHelper::SceneInputNumber(const char* label, std::string* inputvalue, CallbackVoid_1 onCancel, CallbackVoid_1 onOk)
{
	if (!inputvalue) return;
	printf("input: %s \n", inputvalue->c_str());
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

u32 UIHelper::GetRGBA(char r, char g, char b, char a)
{
	return a << 24 | b << 16 | g << 8 | r;
}
