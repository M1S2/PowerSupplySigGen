/*
 * EnumIndicator.h
 *
 * Created: 15.03.2021 19:42:12
 *  Author: V17
 */ 


#ifndef ENUMINDICATOR_H_
#define ENUMINDICATOR_H_

#include "../Core/UIElement.h"

template <class T>
class EnumIndicator : public UIElement
{
	protected:
		const char** _enumNames;
		uint8_t _numEnumValues;
		T* _valuePointer;
		T _valueDraw;				// This variable is updated from the _valuePointer on each draw of the first page.
	
	public:
		EnumIndicator(unsigned char locX, unsigned char locY, T* valuePointer, const char** enumNames, uint8_t numEnumValues);
		virtual void Draw(u8g_t *u8g, bool isFirstPage) override;
};

#endif /* ENUMINDICATOR_H_ */