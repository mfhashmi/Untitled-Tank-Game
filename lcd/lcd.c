/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 *
 *  
 *  - Jan 2015  Modified for LaFortuna (black edition) [KPZ]
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "font.h"
#include "ili934x.h"
#include "lcd.h"

lcd display;

void init_lcd()
{
	/* Enable extended memory interface with 10 bit addressing */
	XMCRB = _BV(XMM2) | _BV(XMM1);
	XMCRA = _BV(SRE);
	DDRC |= _BV(RESET);
	DDRB |= _BV(BLC);
	_delay_ms(1);
	PORTC &= ~_BV(RESET);
	_delay_ms(20);
	PORTC |= _BV(RESET);
	_delay_ms(120);
	write_cmd(DISPLAY_OFF);
	write_cmd(SLEEP_OUT);
	_delay_ms(60);
// ------8<-------8<------   
	write_cmd_data(INTERNAL_IC_SETTING,    		 0x01);
	write_cmd(POWER_CONTROL_1);
		write_data16(0x2608);
	write_cmd_data(POWER_CONTROL_2,		         0x10);
	write_cmd(VCOM_CONTROL_1);
		write_data16(0x353E);
//	write_cmd_data(MEMORY_ACCESS_CONTROL,	     0x48); //MX=1 , ML=1 (vertical refersh upwards) 
//	write_cmd_data(RGB_INTERFACE_SIGNAL_CONTROL, 0x4A);  // Set the DE/Hsync/Vsync/Dotclk polarity
//	write_cmd(DISPLAY_FUNCTION_CONTROL);
//		write_data16(0x0A82);
//		write_data16(0x2700);
	write_cmd_data(VCOM_CONTROL_2, 0xB5);
	write_cmd_data(INTERFACE_CONTROL, 0x01);
		write_data16(0x0000);
//	write_cmd_data(GAMMA_DISABLE, 0x00);
//	write_cmd_data(GAMMA_SET, 0x01);
//------8<-------8<------
	write_cmd_data(PIXEL_FORMAT_SET,		0x55);     // 0x66 - 18bit /pixel,  0x55 - 16bit/pixel
//------8<-------8<------
	/* write_cmd_data(POSITIVE_GAMMA_CORRECTION, 0x1F); */
	/* 	write_data16(0x1A18); */
	/* 	write_data16(0x0A0F); */
	/* 	write_data16(0x0645); */
	/* 	write_data16(0x8732); */
	/* 	write_data16(0x0A07); */
	/* 	write_data16(0x0207); */
	/* 	write_data16(0x0500);	 */
	/* write_cmd_data(NEGATIVE_GAMMA_CORRECTION, 0x00); */
	/* 	write_data16(0x2527); */
	/* 	write_data16(0x0510); */
	/* 	write_data16(0x093A); */
	/* 	write_data16(0x784D); */
	/* 	write_data16(0x0518); */
	/* 	write_data16(0x0D38); */
	/* 	write_data16(0x3A1F); */
//	write_cmd_data(POSITIVE_GAMMA_CORRECTION, 15, "\x1F \x1A\x18 \x0A\x0F \x06\x45 \x87\x32 \x0A\x07 \x02\x07 \x05\x00");
//	write_cmd_data(NEGATIVE_GAMMA_CORRECTION, 15, "\x00 \x25\x27 \x05\x10 \x09\x3A \x78\x4D \x05\x18 \x0D\x38 \x3A\x1F");
//------8<-------8<------
	/* write_cmd(COLUMN_ADDRESS_SET); */
	/* 	write_data16(0x0000); */
	/* 	write_data16(0x00EF); */
	/* write_cmd(PAGE_ADDRESS_SET); */
	/* 	write_data16(0x0000); */
	/* 	write_data16(0x013F); */
	// write_cmd_data(DISPLAY_INVERSION_CONTROL, 0x00);
	// write_cmd(DISPLAY_INVERSION_ON);  /* kpz */
//	write_cmd(DISPLAY_INVERSION_OFF);  /* kpz */
//	write_cmd(IDLE_MODE_ON);      /* kpz */
//	write_cmd(IDLE_MODE_OFF);      /* kpz */
//------8<-------8<------
	set_orientation(North);
	clear_screen();
	display.x = 0;
	display.y = 0;
	display.background = BLACK;
	display.foreground = WHITE;
	write_cmd(DISPLAY_ON);
	_delay_ms(50);
    write_cmd_data(TEARING_EFFECT_LINE_ON, 0x00);
    // write_cmd(TEARING_EFFECT_LINE_OFF);
	EICRB |= _BV(ISC61);
	PORTB |= _BV(BLC);
}

void lcd_brightness(uint8_t i)
{
	/* Configure Timer 2 Fast PWM Mode 3 */
	TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(CS20);
	OCR2A = i;
}

void set_orientation(orientation o)
{
	display.orient = o;
	write_cmd(MEMORY_ACCESS_CONTROL);
	if (o==North) { 
		display.width = LCDWIDTH;
		display.height = LCDHEIGHT;
		write_data(0x48);
	}
	else if (o==West) {
		display.width = LCDHEIGHT;
		display.height = LCDWIDTH;
		write_data(0xE8);
	}
	else if (o==South) {
		display.width = LCDWIDTH;
		display.height = LCDHEIGHT;
		write_data(0x88);
	}
	else if (o==East) {
		display.width = LCDHEIGHT;
		display.height = LCDWIDTH;
		write_data(0x28);
	}
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(0);
	write_data16(display.width-1);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(0);
	write_data16(display.height-1);
}



void set_frame_rate_hz(uint8_t f)
{
	uint8_t diva, rtna, period;
	if (f>118)
		f = 118;
	if (f<8)
		f = 8;
	if (f>60) {
		diva = 0x00;
	} else if (f>30) {
		diva = 0x01;
	} else if (f>15) {
		diva = 0x02;
	} else {
		diva = 0x03;
	}
	/*   !!! FIXME !!!  [KPZ-30.01.2015] */
	/*   Check whether this works for diva > 0  */
	/*   See ILI9341 datasheet, page 155  */
	period = 1920.0/f;
	rtna = period >> diva;
	write_cmd(FRAME_CONTROL_IN_NORMAL_MODE);
	write_data(diva);
	write_data(rtna);
}

void fill_rectangle(rectangle r, uint16_t col)
{
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(r.left);
	write_data16(r.right);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(r.top);
	write_data16(r.bottom);
	write_cmd(MEMORY_WRITE);
/*	uint16_t x, y;
  	for(x=r.left; x<=r.right; x++)
		for(y=r.top; y<=r.bottom; y++)
			write_data16(col);
*/
	uint16_t wpixels = r.right - r.left + 1;
	uint16_t hpixels = r.bottom - r.top + 1;
	uint8_t mod8, div8;
	uint16_t odm8, odd8;
	if (hpixels > wpixels) {
		mod8 = hpixels & 0x07;
		div8 = hpixels >> 3;
		odm8 = wpixels*mod8;
		odd8 = wpixels*div8;
	} else {
		mod8 = wpixels & 0x07;
		div8 = wpixels >> 3;
		odm8 = hpixels*mod8;
		odd8 = hpixels*div8;
	}
	uint8_t pix1 = odm8 & 0x07;
	while(pix1--)
		write_data16(col);

	uint16_t pix8 = odd8 + (odm8 >> 3);
	while(pix8--) {
		write_data16(col);
		write_data16(col);
		write_data16(col);
		write_data16(col);
		write_data16(col);
		write_data16(col);
		write_data16(col);
		write_data16(col);
	}
}

void fill_rectangle_indexed(rectangle r, uint16_t* col)
{
	uint16_t x, y;
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(r.left);
	write_data16(r.right);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(r.top);
	write_data16(r.bottom);
	write_cmd(MEMORY_WRITE);
	for(x=r.left; x<=r.right; x++)
		for(y=r.top; y<=r.bottom; y++)
			write_data16(*col++);
}

void clear_screen()
{
	display.x = 0;
	display.y = 0;
	rectangle r = {0, display.width-1, 0, display.height-1};
	fill_rectangle(r, display.background);
}

void display_char(char c)
{
	uint16_t x, y;
	PGM_P fdata; 
	uint8_t bits, mask;
	uint16_t sc=display.x, ec=display.x + 4, sp=display.y, ep=display.y + 7;
	if (c < 32 || c > 126) return;
	fdata = (c - ' ')*5 + font5x7;
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(sp);
	write_data16(ep);
	for(x=sc; x<=ec; x++) {
		write_cmd(COLUMN_ADDRESS_SET);
		write_data16(x);
		write_data16(x);
		write_cmd(MEMORY_WRITE);
		bits = pgm_read_byte(fdata++);
		for(y=sp, mask=0x01; y<=ep; y++, mask<<=1)
			write_data16((bits & mask) ? display.foreground : display.background);
	}
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(x);
	write_data16(x);
	write_cmd(MEMORY_WRITE);
	for(y=sp; y<=ep; y++)
		write_data16(display.background);

	display.x += 6;
	if (display.x >= display.width) { display.x=0; display.y+=8; }
}

void display_string(char *str)
{
	uint8_t i;
	for(i=0; str[i]; i++) 
		display_char(str[i]);
}

void display_string_xy(char *str, uint16_t x, uint16_t y)
{
	uint8_t i;
	display.x = x;
	display.y = y;
	for(i=0; str[i]; i++)
		display_char(str[i]);
}

