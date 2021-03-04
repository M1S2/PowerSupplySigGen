/*
 * Icons.h
 *
 * Created: 02.03.2021 21:47:19
 *  Author: V17
 */ 


#ifndef ICONS_H_
#define ICONS_H_

#define icon_powerCord_width 28
#define icon_powerCord_height 25
static unsigned char icon_powerCord_bits[] U8G_PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x1c, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
	0x60, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
	0x70, 0x00, 0x00, 0x00, 0x38, 0x00, 0x0e, 0x00, 0x18, 0x00, 0x0e, 0x00,
	0x0c, 0xfc, 0x0f, 0x00, 0x0c, 0xfe, 0xff, 0x07, 0x0c, 0xff, 0xff, 0x07,
	0xd8, 0xff, 0x0f, 0x00, 0xf0, 0xff, 0x0f, 0x00, 0xc0, 0xff, 0x0f, 0x00,
	0x00, 0xff, 0x0f, 0x00, 0x00, 0xff, 0xff, 0x07, 0x00, 0xfe, 0xff, 0x07,
	0x00, 0xfc, 0x0f, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0e, 0x00,
    0x00, 0x00, 0x00, 0x00 };

#define icon_OnOff_width 15
#define icon_OnOff_height 15
static unsigned char icon_OnOff_bits[] U8G_PROGMEM = {
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x88, 0x08, 0x8c, 0x18, 0x86, 0x30,
	0x82, 0x20, 0x82, 0x20, 0x02, 0x20, 0x02, 0x20, 0x06, 0x30, 0x0c, 0x18,
    0x18, 0x0c, 0xf0, 0x07, 0xc0, 0x01 };

#endif /* ICONS_H_ */