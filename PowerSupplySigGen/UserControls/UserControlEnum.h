/*
 * UserControlEnum.h
 *
 * Created: 30.09.2020 21:05:03
 *  Author: V17
 */ 

#ifndef USERCONTROLENUM_H_
#define USERCONTROLENUM_H_

#include "UserControlBase.h"

template <class T>
class UserControlEnum : public UserControlBase<T>
{
private:
	const char** _enumNames;
	uint8_t _numEnumValues;
public:
	UserControlEnum(uint8_t locx, uint8_t locy, T* controlValue, const char** enumNames, uint8_t numEnumValues) : UserControlBase<T>(locx, locy, controlValue)
	{
		_enumNames = enumNames;
		_numEnumValues = numEnumValues;
	}
	
	void KeyInput(Keys_t key)
	{
		if(UserControlBase<T>::IsActive)
		{
			uint8_t* value = (uint8_t*)UserControlBase<T>::_controlValue;
			
			if(key == KEYRIGHT) { EncoderInput(ENCCLOCKWISE); }
			else if(key == KEYLEFT) { EncoderInput(ENCCOUNTERCLOCKWISE); }
			else if(key == KEY0 && _numEnumValues > 0) { *value = 0; }
			else if(key == KEY1 && _numEnumValues > 1) { *value = 1; }
			else if(key == KEY2 && _numEnumValues > 2) { *value = 2; }
			else if(key == KEY3 && _numEnumValues > 3) { *value = 3; }
			else if(key == KEY4 && _numEnumValues > 4) { *value = 4; }
			else if(key == KEY5 && _numEnumValues > 5) { *value = 5; }
			else if(key == KEY6 && _numEnumValues > 6) { *value = 6; }
			else if(key == KEY7 && _numEnumValues > 7) { *value = 7; }
			else if(key == KEY8 && _numEnumValues > 8) { *value = 8; }
			else if(key == KEY9 && _numEnumValues > 9) { *value = 9; }
		}
	}
	
	void EncoderInput(EncoderDirection_t encDir)
	{
		if(UserControlBase<T>::IsActive)
		{
			uint8_t* value = (uint8_t*)UserControlBase<T>::_controlValue;
			
			if(encDir == ENCCLOCKWISE && *value < (_numEnumValues - 1)) { (*value)++; }
			else if(encDir == ENCCLOCKWISE && *value >= (_numEnumValues - 1)) { (*value) = 0; }
			else if (encDir == ENCCOUNTERCLOCKWISE && *value > 0) { (*value)--; }
			else if(encDir == ENCCOUNTERCLOCKWISE && *value == 0) { (*value) = _numEnumValues - 1; }
		}
	}
	
	void Draw(u8g_t *u8g)
	{
		UserControlBase<T>::Draw(u8g);
		u8g_DrawStr(u8g, UserControlBase<T>::_locX + 3, UserControlBase<T>::_locY + 3 + CONTROLS_FONT_HEIGHT, _enumNames[*UserControlBase<T>::_controlValue]);
	}
};

#endif /* USERCONTROLENUM_H_ */