/*
 * ButtonControl.cpp
 *
 * Created: 31.03.2021 20:42:49
 *  Author: V17
 */ 

#include "ButtonControl.h"
#include <string.h>

ButtonControl::ButtonControl(unsigned char locX, unsigned char locY, unsigned char width, unsigned char height, const char* buttonText, void* controlContext, void(*onClick)(void* controlContext)) : UIElement(locX, locY, width, height, UI_CONTROL)
{
	strcpy(_buttonText, buttonText);
	_controlContext = controlContext;
	_onClick = onClick;
}

void ButtonControl::Draw(u8g_t *u8g, bool isFirstPage)
{
	if (Visible)
	{
		u8g_DrawHLine(u8g, LocX - 1, LocY - 1, 3);					// Upper left corner
		u8g_DrawVLine(u8g, LocX - 1, LocY - 1, 3);
		u8g_DrawHLine(u8g, LocX + Width - 2, LocY - 1, 3);			// Upper right corner
		u8g_DrawVLine(u8g, LocX + Width, LocY - 1, 3);
		u8g_DrawHLine(u8g, LocX - 1, LocY + Height, 3);				// Lower left corner
		u8g_DrawVLine(u8g, LocX - 1, LocY + Height - 2, 3);
		u8g_DrawHLine(u8g, LocX + Width - 2, LocY + Height, 3);		// Lower right corner		
		u8g_DrawVLine(u8g, LocX + Width, LocY + Height - 2, 3);
		u8g_DrawStr(u8g, LocX, LocY, _buttonText);
	}
}

bool ButtonControl::KeyInput(Keys_t key)
{
	switch (key)
	{
		case KEYOK:
			if (_onClick != NULL) { _onClick(_controlContext); return true; }
			else { return false; }
		default:
			return false;
	}
}