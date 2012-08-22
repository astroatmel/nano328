//
// Atmel a328p LCD driver routines
// by pmichel
//

// The LCD requires the Real-Time package:
// wait_us();
//
// The LCD requires the RS232 define:
// A328P_RS232_AVAILABLE   >   to indicate if we can use the RS232 output routines
//
// The LCD requires the A328P pins:
// PORT B bit 0  (LCD E)
// PORT D bit 2  (LCD RW)
// PORT D bit 4  (LCD RS)
//
// PORT B bit 1  (LCD BIT 0)
// PORT B bit 4  (LCD BIT 1)
// PORT B bit 5  (LCD BIT 3)
// PORT D bit 7  (LCD BIT 4)

#ifdef  USE_LCD        // Option to enable/disable LCD module

#ifndef A328P_LCD
#define A328P_LCD

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "a328p_realtime.h"

#ifdef A328P_LCD_MAIN
   // local labels only to LCD_MAIN module
   unsigned char lcd_requires_reset=1;                // run-time reset request
#else
   extern unsigned char lcd_requires_reset;           // run-time reset request
#endif

#define LCD_SET_E_HIGH     PORTD |=  0x10          // PORT D bit 4
#define LCD_SET_E_LOW      PORTD &= ~0x10

#define LCD_SET_RW_HIGH    PORTB |=  0x01          // PORT B bit 0
#define LCD_SET_RW_LOW     PORTB &= ~0x01

#define LCD_SET_RS_HIGH    PORTD |=  0x04          // PORT D bit 2
#define LCD_SET_RS_LOW     PORTD &= ~0x04

#define LCD_SET_DATA_DIR_OUTPUT           \
{                                         \
DDRB |= 0x32;                             \
DDRD |= 0x80;                             \
}

#define LCD_SET_DATA_DIR_INPUT            \
{                                         \
DDRB &= ~0x32;                            \
DDRD &= ~0x80;                            \
}

#define LCD_SET_CTRL_DIR_OUTPUT           \
{                                         \
DDRB |= 0x01;                             \
DDRD |= 0x14;                             \
}

#define LCD_SET_CTRL_DIR_INPUT            \
{                                         \
DDRB &= ~0x01;                            \
DDRD &= ~0x14;                            \
}


// User functions
void lcd_init(void);
void lcd_goto(unsigned char row,unsigned char col);
void lcd_print_str(char *STR);

// Internal functions
unsigned char lcd_read_4_bits(void);
void lcd_write_4_bits(unsigned char data); // use lower 4 bits of data to drive the port of the LCD
void lcd_wait_busy(void);
void lcd_wait_send(unsigned char data, unsigned char ms);   // send comman function that does not check for the busy flag
void lcd_send(unsigned char data);
void lcd_text(unsigned char data);


#endif  // A328P_LCD

#else   // USE_LCD

   // if USE_LCD no defined, void the usage of the user functions:
   #define lcd_init()  ;
   #define lcd_goto(row,col)  ;
   #define lcd_print_str(STR)  ;

   #define LCD_SET_DATA_DIR_OUTPUT   ;
   #define LCD_SET_DATA_DIR_INPUT    ;
   #define LCD_SET_CTRL_DIR_OUTPUT   ;
   #define LCD_SET_CTRL_DIR_INPUT    ;

#endif  // USE_LCD
