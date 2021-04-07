/*
 * BoolIndicator.cpp
 *
 * Created: 12.03.2021 18:26:42
 *  Author: V17
 */ 

#include "BoolIndicator.h"

BoolIndicator::BoolIndicator(unsigned char locX, unsigned char locY, unsigned char width, unsigned char height, bool* valuePointer) : UIElement(locX, locY, width, height, UI_INDICATOR)
{
	_valuePointer = valuePointer;
}

void BoolIndicator::Draw(u8g_t *u8g, bool isFirstPage)
{
	if (Visible)
	{	
		if (isFirstPage) { _valueDraw = *_valuePointer; }
			
		u8g_DrawStr(u8g, LocX, LocY, (_valueDraw ? "ON" : "OFF"));
	}
}