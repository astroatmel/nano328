//
// Atmel a328p LCD driver routines
// by pmichel
//

#define A328P_LCD_MAIN
#include "a328p_lcd.h"

#ifdef  USE_LCD 

unsigned char lcd_read_4_bits(void)
{
unsigned char pb;
unsigned char pd;
unsigned char value=0;

LCD_SET_DATA_DIR_INPUT
rt_wait_us(1);
LCD_SET_E_HIGH;
rt_wait_us(2);
pb=PINB;
pd=PIND;
LCD_SET_E_LOW;

if ( pb & 0x02 ) value |= 0x01;
if ( pb & 0x10 ) value |= 0x02;
if ( pb & 0x20 ) value |= 0x04;
if ( pd & 0x80 ) value |= 0x08;
return value;
}


void lcd_write_4_bits(unsigned char data) // use lower 4 bits of data to drive the port of the LCD
{
unsigned char pb=PORTB & ~0x32;
unsigned char pd=PORTD & ~0x80;
if ( data & 0x01 ) pb |= 0x02;
if ( data & 0x02 ) pb |= 0x10;
if ( data & 0x04 ) pb |= 0x20;
if ( data & 0x08 ) pd |= 0x80;
LCD_SET_DATA_DIR_OUTPUT
PORTB = pb;
PORTD = pd;
rt_wait_us(1);
LCD_SET_E_HIGH;
rt_wait_us(4);
LCD_SET_E_LOW;
rt_wait_us(1);
}

void lcd_wait_busy(void)
{
unsigned char HH;
volatile long timeout=10000;
if ( lcd_requires_reset ) return;
LCD_SET_DATA_DIR_INPUT;
LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_HIGH;  

do {
   timeout--;
   HH=lcd_read_4_bits();
      lcd_read_4_bits();
   } while ( (HH==0x08) && (timeout!=0) ) ;
if ( timeout==0 ) lcd_requires_reset=1;
}


void lcd_wait_send(unsigned char data, unsigned char ms)   // send comman function that does not check for the busy flag
{
rt_wait_ms(ms);
LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_LOW;

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_send(unsigned char data)
{
lcd_wait_busy();

LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_LOW;

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_text(unsigned char data)
{
lcd_wait_busy();

LCD_SET_E_LOW;
LCD_SET_RS_HIGH;  // text
LCD_SET_RW_LOW;   // write

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_reset(void)
{
lcd_requires_reset = 1;
}

void lcd_init(void)
{
LCD_SET_CTRL_DIR_OUTPUT
LCD_SET_DATA_DIR_INPUT;
LCD_SET_E_LOW;
LCD_SET_RS_HIGH;

lcd_requires_reset=0;
lcd_wait_send(0x30,20);   // Send 0x3 (last four bits ignored)
lcd_wait_send(0x30,6);    // Send 0x3 (last four bits ignored)
lcd_wait_send(0x30,2);    // Send 0x3 (last four bits ignored)
lcd_wait_send(0x20,2);    // Send 0x2 (last four bits ignored)  Sets 4-bit mode
lcd_wait_send(0x28,2);    // Send 0x28 = 4-bit, 2-line, 5x8 dots per char
// Busy Flag is now valid
lcd_send(0x08); // Send 0x08 = Display off, cursor off, blinking off
lcd_send(0x01); // Send 0x01 = Clear display
rt_wait_us(500);   // LCD executiuon time
lcd_send(0x02); // Send 0x02 = Cursor Home
rt_wait_us(500);   // LCD executiuon time
lcd_send(0x06); // Send 0x06 = Set entry mode: cursor shifts right, don't scroll
rt_wait_us(500);   // LCD executiuon time
lcd_send(0x0C); // Send 0x0C = Display on, cursor off, blinking off
}


void lcd_print_str(char *STR)
{
if ( lcd_requires_reset ) 
   {
   #ifdef A328P_RS232_AVAILABLE
      // print only if the RS232 package is included / active
      ps("\012\015LCD requires Reset...");
   #endif
   rt_wait(1);
   lcd_init();
   }

while (*STR) lcd_text(*STR++);
}

void lcd_goto(unsigned char row,unsigned char col)
{
if ( lcd_requires_reset ) return;

if ( row==0 ) lcd_send(0x80+col);
else          lcd_send(0xC0+col);
rt_wait_us(40);  // LCD executiuon time
}

#endif  // USE_LCD

