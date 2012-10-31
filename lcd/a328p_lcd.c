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

// pmichel : define custom characters here, each one is 8 bytes, and only the 5 lower bits are used
PROGMEM const char my_characters[]={0x15,0x0A,0x15,0x0A,0x15,0x0A,0x15,0x0A       // Race Flag
                                   ,0x0E,0x0A,0x0E,0x00,0x00,0x00,0x00,0x00       // Deg   o
                                   ,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00       // Min   '
                                   ,0x0A,0x0A,0x0A,0x00,0x00,0x00,0x00,0x00       // Sec   "
                                   ,0x00,0x04,0x0E,0x1F,0x00,0x00,0x00,0x00       // Up
                                   ,0x00,0x00,0x00,0x00,0x00,0x1F,0x0E,0x04       // Down
                                   ,0x00,0x04,0x0E,0x1F,0x00,0x1F,0x0E,0x04       // Up/Down
                                   };

void lcd_init(void)
{
unsigned char iii,base=0x40;
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

// pmichel custom characters test:
for ( iii=0 ; iii<sizeof(my_characters) ; iii++ )
   {
   lcd_send(base++);
   lcd_text(pgm_read_byte(&my_characters[iii]));
   } 
}


void lcd_print_str(char *STR)
{
unsigned char max=16;  // print one line max 
if ( lcd_requires_reset ) 
   {
   #ifdef A328P_RS232_AVAILABLE
      // print only if the RS232 package is included / active
      ps("\012\015LCD requires Reset...");
   #endif
   rt_wait(1);
   lcd_init();
   }

while (*STR) 
   {
   lcd_text(*STR++);
   if ( max-- == 0 ) return;
   }
}


// Function to call at 1 Khz and will send the next character, no wait, no checks
// the function assumes that 1ms between writes does not require any checks
void lcd_rt_print_next(void)
{
unsigned long now;
unsigned char pb,pd,data;
static unsigned char chunk=0,ptr=0,CCC[4]; 
if ( lcd_requires_reset ) return; // dont drive the LCD if it's not ready yet

if (chunk==0)  // a new character to send
   {
   if ( (ptr & 0x0F) == 0 )  // start of a new line
      {
      chunk = 4;
      if ( ptr == 0x10 ) CCC[3] = 0x0C;  // upper chunk of command 0x80  (goto 0,0)
      else               CCC[3] = 0x08;  // upper chunk of command 0xC0  (goto 0,0)
                         CCC[2] = 0x00;  // lower chunk fisrt
      }
   else chunk = 2;
   CCC[1] = lcd_lines[ptr] >> 4; 
   CCC[0] = lcd_lines[ptr] & 0x0F;
   ptr = (ptr+1)&0x1F;  // 
   }

LCD_SET_E_LOW;
if ( chunk < 3 ) {LCD_SET_RS_HIGH; }  // text
else             {LCD_SET_RS_LOW;  }  // command
LCD_SET_RW_LOW;   // write

data = CCC[--chunk];

   pb=PORTB & ~0x32;
   pd=PORTD & ~0x80;
   if ( data & 0x01 ) pb |= 0x02;
   if ( data & 0x02 ) pb |= 0x10;
   if ( data & 0x04 ) pb |= 0x20;
   if ( data & 0x08 ) pd |= 0x80;
   LCD_SET_DATA_DIR_OUTPUT
   PORTB = pb;
   PORTD = pd;
      now = TCNT1 + 20;  // 20 clock tick is 1 us
      while ( now > TCNT1) ; // wait 1 us
   LCD_SET_E_HIGH;
      now = TCNT1 + 80;  // 20 clock tick is 1 us
      while ( now > TCNT1) ; // wait 4 us
   LCD_SET_E_LOW;
      
LCD_SET_DATA_DIR_INPUT;
}

void lcd_goto(unsigned char row,unsigned char col)
{
if ( lcd_requires_reset ) return;

if ( row==0 ) lcd_send(0x80+col);
else          lcd_send(0xC0+col);
rt_wait_us(40);  // LCD executiuon time
}

#endif  // USE_LCD

