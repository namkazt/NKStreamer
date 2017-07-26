#include "UIHelper.h"


UIHelper::UIHelper()
{

}

void UIHelper::InitServices()
{
	//---------------------------------------------
	// Init ROMFS
	//---------------------------------------------
	Result rc = romfsInit();
	if (rc) {
		printf("romfsInit: %08lX\n", rc);
		return;
	}
	else
	{
		printf("romfs Init Successful!\n");
	}
	//---------------------------------------------
	// Init SOCKET
	//---------------------------------------------
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	u32 ret = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (ret != 0) {
		printf("romfsInit: %08lX\n", rc);
		return;
	}
	printf("Init SOC service successfully.\n");
}

void UIHelper::EndServices()
{
	socExit();
	free(SOC_buffer);
	SOC_buffer = NULL;

	romfsExit();
}

void UIHelper::LoadFonts()
{
	font_FreeSans = sftd_load_font_mem(FreeSans_ttf, FreeSans_ttf_size);
	font_OpenSans = sftd_load_font_mem(OpenSans_ttf, OpenSans_ttf_size);
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
	if(touch.px == 0 && touch.py == 0)
	{
		_isTouchEnd = false;
		if(_isTouchStart)
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
	}else
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

	guiPadding = Rect(0, 0, 0, 0);
	guiMargin = Rect(0, 0, 0, 0);
}

void UIHelper::EndGUI()
{
	
}

void UIHelper::setPadding(u32 left, u32 top, u32 right, u32 bottom)
{
	guiPadding.x = left;
	guiPadding.y = top;
	guiPadding.w = right;
	guiPadding.h = bottom;
}

void UIHelper::setMargin(u32 left, u32 top, u32 right, u32 bottom)
{
	guiMargin.x = left;
	guiMargin.y = top;
	guiMargin.w = right;
	guiMargin.h = bottom;
}

void UIHelper::GUI_Panel(u32 x, u32 y, u32 w, u32 h, const char * text, int size)
{
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y, w - guiMargin.w - guiMargin.x, h - guiMargin.h , UIHelper::Col_Primary);
	sftd_draw_text(font_FreeSans, x + guiMargin.x + 10, 3 +  y + guiMargin.y, UIHelper::Col_Text_Dark, size, text);
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y + h, w - guiMargin.w - guiMargin.x, 2, UIHelper::Col_Background_Light);
}

bool UIHelper::GUI_Button(u32 x, u32 y, u32 w, u32 h, const char* text)
{
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y, w - guiMargin.w - guiMargin.x, h - guiMargin.h, UIHelper::Col_Secondary);
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y + h, w - guiMargin.w - guiMargin.x, 3, UIHelper::Col_Background_Light);
	sftd_draw_text(font_OpenSans, 10 + x + guiMargin.x, 3 + y + guiMargin.y, UIHelper::Col_Text_Dark, 13, text);
	//-----------------------
	if (IsContainTouch(x + guiMargin.x, y + guiMargin.y, w - guiMargin.w - guiMargin.x, h - guiMargin.h))
	{
		return true;
	}
	return false;
}

const char* UIHelper::GUI_TextInput(u32 x, u32 y, u32 w, u32 h,const char* text, const char* placeHolder, int size)
{
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y, w - guiMargin.w - guiMargin.x, h - guiMargin.h, UIHelper::Col_Primary_Light);
	sf2d_draw_rectangle(x + guiMargin.x, y + guiMargin.y + h, w - guiMargin.w - guiMargin.x, 3, UIHelper::Col_Background_Light);
	if (strcmp(text, "") == 0)
		sftd_draw_text(font_OpenSans, 10 + x + guiMargin.x, 3 + y + guiMargin.y, UIHelper::Col_Background_Dark, size, placeHolder);
	else
		sftd_draw_text(font_OpenSans, 10 + x + guiMargin.x, 3 + y + guiMargin.y, UIHelper::Col_Text_Dark, size, text);
	//-----------------------
	if (IsContainTouch(x + guiMargin.x,y + guiMargin.y, w - guiMargin.w - guiMargin.x, h - guiMargin.h))
	{
		StartUpdateKeyboard();
		ShowInputKeyboard(nullptr, text);
		const char* output = EndUpdateKeyboard();
		return output;
	}
	return nullptr;
}


bool UIHelper::IsContainTouch(u32 x, u32 y, u32 w, u32 h)
{

	if(x < _lastTouch.px && x + w > _lastTouch.px &&
		y < _lastTouch.py && y + h > _lastTouch.py && _isTouchEnd)
	{
		return true;
	}
	return false;
}

void UIHelper::StartUpdateKeyboard()
{
	keyboardButton = SWKBD_BUTTON_NONE;
	isUsingKeyboard = false;
}

void UIHelper::ShowInputKeyboard(SwkbdCallbackFn callback, const char* initText)
{
	isUsingKeyboard = true;
	swkbdInit(&keyboardState, SWKBD_TYPE_WESTERN, 2, -1);
	swkbdSetInitialText(&keyboardState, initText);
	swkbdSetValidation(&keyboardState, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	if(callback != nullptr) swkbdSetFilterCallback(&keyboardState, callback, NULL);
	keyboardButton = swkbdInputText(&keyboardState, keyboardBuffer, sizeof(keyboardBuffer));
}

void UIHelper::ShowNumberKeyboard(SwkbdCallbackFn callback, size_t count, const char* initText)
{
	isUsingKeyboard = true;
	swkbdInit(&keyboardState, SWKBD_TYPE_NUMPAD, 1, count);
	swkbdSetInitialText(&keyboardState, initText);
	swkbdSetValidation(&keyboardState, SWKBD_ANYTHING, SWKBD_FILTER_DIGITS, 0);
	swkbdSetFeatures(&keyboardState, SWKBD_FIXED_WIDTH);
	if (callback != nullptr) swkbdSetFilterCallback(&keyboardState, callback, NULL);
	keyboardButton = swkbdInputText(&keyboardState, keyboardBuffer, sizeof(keyboardBuffer));
}
void UIHelper::ShowNumberKeyboardWithDot(SwkbdCallbackFn callback, size_t count, const char* initText)
{
	isUsingKeyboard = true;
	swkbdInit(&keyboardState, SWKBD_TYPE_WESTERN, 2, -1);
	swkbdSetInitialText(&keyboardState, initText);
	swkbdSetValidation(&keyboardState, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS, 0);
	if (callback != nullptr) swkbdSetFilterCallback(&keyboardState, callback, NULL);
	keyboardButton = swkbdInputText(&keyboardState, keyboardBuffer, sizeof(keyboardBuffer));
}

const char* UIHelper::EndUpdateKeyboard()
{
	if (isUsingKeyboard)
	{
		return &keyboardBuffer[0];
	}
	return nullptr;
}


SwkbdCallbackResult UIHelper::IPInputValidate(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	/*if (strstr(text, "lenny"))
	{
		*ppMessage = "Nice try but I'm not letting you use that meme right now";
		return SWKBD_CALLBACK_CONTINUE;
	}

	if (strstr(text, "brick"))
	{
		*ppMessage = "~Time to visit Brick City~";
		return SWKBD_CALLBACK_CLOSE;
	}
*/
	return SWKBD_CALLBACK_OK;
}
