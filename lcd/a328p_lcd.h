//
// Atmel a328p LCD driver routines (see: HD44780.pdf)
// by pmichel
//

// The LCD requires the Real-Time package:
// wait_us();
//
// The LCD requires the RS232 define:
// A328P_RS232_AVAILABLE   >   to indicate if we can use the RS232 output routines
//

#ifdef  USE_LCD        // Option to enable/disable LCD module

// The next defines are useless, but errors will be triggered if two modules requires the same pin
// The LCD requires the A328P pins:
//
#define USING_A328P_PIN_14   PORT_B_BIT_0   // PORT B bit 0  (LCD E)
#define USING_A328P_PIN_04   PORT_D_BIT_2   // PORT D bit 2  (LCD RW)
#define USING_A328P_PIN_06   PORT_D_BIT_4   // PORT D bit 4  (LCD RS)

#define USING_A328P_PIN_15   PORT_B_BIT_1   // PORT B bit 1  (LCD BIT 0)
#define USING_A328P_PIN_18   PORT_B_BIT_4   // PORT B bit 4  (LCD BIT 1)
#define USING_A328P_PIN_19   PORT_B_BIT_5   // PORT B bit 5  (LCD BIT 2)
#define USING_A328P_PIN_13   PORT_D_BIT_7   // PORT D bit 7  (LCD BIT 3)


#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "a328p_rt.h"

#ifdef A328P_LCD_MAIN
   // local labels only to LCD_MAIN module
   unsigned char lcd_requires_reset=1;                // run-time reset request
   unsigned char lcd_lines[4*16];                    // LCD display
   unsigned char lcd_lines_nmi[4*16];               // LCD display
#else
   extern unsigned char lcd_requires_reset;           // run-time reset request
   extern unsigned char lcd_lines[4*16];             // LCD display
   extern unsigned char lcd_lines_nmi[4*16];        // LCD display
#endif

#define LCD_BUTTON_1  (PINB & 0x02)
#define LCD_BUTTON_2  (PINB & 0x10)
#define LCD_BUTTON_3  (PINB & 0x20)
#define LCD_BUTTON_4  (PIND & 0x80)

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
void lcd_reset();
void lcd_rt_print_next(void);  // function to print to LCD in real-time ... just fill lcd_lines[] and call lcd_rt_print_next() at 1khz

// Internal functions
unsigned char lcd_read_4_bits(void);
void lcd_write_4_bits(unsigned char data); // use lower 4 bits of data to drive the port of the LCD
void lcd_wait_busy(void);
void lcd_wait_send(unsigned char data, unsigned char ms);   // send comman function that does not check for the busy flag
void lcd_send(unsigned char data);
void lcd_text(unsigned char data);



#else   // USE_LCD

   // if USE_LCD not defined, void the usage of the user functions:
   #define lcd_init()          ;
   #define lcd_goto(row,col)   ;
   #define lcd_print_str(STR)  ;
   #define lcd_reset()         ;
   #define lcd_rt_print_next() ;

   #define LCD_SET_DATA_DIR_OUTPUT   ;
   #define LCD_SET_DATA_DIR_INPUT    ;
   #define LCD_SET_CTRL_DIR_OUTPUT   ;
   #define LCD_SET_CTRL_DIR_INPUT    ;

   #define LCD_BUTTON_1  (0)
   #define LCD_BUTTON_2  (0)
   #define LCD_BUTTON_3  (0)
   #define LCD_BUTTON_4  (0)

#endif  // USE_LCD
