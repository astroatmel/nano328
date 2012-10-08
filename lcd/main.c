
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "a328p_lcd.h"

#include "a328p_rt.h"

unsigned long d_ram;
unsigned char lcd_go_rt=0;

volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;


#define CDB_SIZE 10
long cdb[10];
//  0: Runnig time in seconds
//  1: RA from CGEM
//  2: DE from CGEN 
//  3: Exposure time in mili-second
//  4: Delta time in mili-second between shoots
//  5: Shot #        total: 81 on a 9x9 grid
//  6: Shot RA  id   9x9 grid
//  7: Shot DEC id   9x9 grid
//  8: Total # shot

PROGMEM const char linetxt[]="                "  //  0: 
                             "Camera Driver   "  //  1: 
                             "Version 1.0     "  //  2: 
                             "RA:@R           "  //  3: @R will display "?XXX:XX:XX.XX"   Deg/Min/Sec  ? is E/W
                             "RA:@H           "  //  4: @H will display "XXhXXmXX.XXs"   Hour/Min/Sec  
                             "DE:@D           "  //  5: @D will display "?XXX:XX:XX.XX"   Deg/Min/Sec  ? is N/S
                             "Mozaic Menu...  "  //  6: 
                             "Timelap Menu... "  //  7:
                             "Total Span:     "  //  8:    Mozaic span in deg:min:sec
                             "Exposure Time:  "  //  9:    
                             "Delta Time:     "  // 10:
                             "Current:@s      "  // 11: @s will display "XXX Sec"
                             "    Set:@s      "  // 12:
                             "Cur:@h          "  // 13: @h will display "XXhXXm"
                             "Set:@h          "  // 14:
                             "Cur:@d          "  // 15: @d will display "XXX:XX:XX.XX"  Deg/Min/Sec
                             "Set:@d          "  // 16:
                             "Start Mozaic... "  // 17:
                             "Start Timelaps.."  // 18:
                             "Really Cancel ? "  // 19:
                             "Mozaic: @o      "  // 20: @o will display "xx of yy" and use two consecutive CDB location
                             "Pos Id: @p      "  // 21: @p will display xx,yy  and use two consecutive CDB location
;

//                                              M  T  .  .  M  M  M  M  M  M  M  M
// States:                             0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20
PROGMEM const unsigned char line1[] = {1 ,3 ,4 ,6 ,7 ,0 ,0 ,9 ,9 ,8 ,8 ,17,20,19,0 ,0 ,0 ,0 ,0 ,0 ,0  };   // what string to display on line 1 for each states
PROGMEM const unsigned char line2[] = {2 ,5 ,5 ,0 ,0 ,0 ,0 ,11,12,11,12,0 ,21,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0  };   // what string to display on line 2 for each states

short d_state;   // main state machine that controls what to display
unsigned char button_down;   // check how long a button is held down
unsigned char button     ;   // Current button
unsigned char button_last;   // Prevous button

PROGMEM const char main_state_machine[]= {0,// ENTER  UNDO   UP     DOWN             // -1 means no action    -2 means edit ?
                                               1     ,1     ,1     ,1                // State:  0  welcome message, any key to go to state 1
                                              ,-1    ,-1    ,4     ,2                // State:  1  Show current telescope position {e} 
                                              ,-1    ,-1    ,1     ,3                // State:  2  Show current telescope position {e} in hour min sec
                                              ,7     ,-1    ,2     ,4                // State:  3  Mozaic Menu
                                              ,-1    ,-1    ,3     ,1                // State:  4  Timelaps pictures Menu
                                              ,-1    ,-1    ,-1    ,-1               // State:  5  Spare
                                              ,-1    ,-1    ,-1    ,-1               // State:  6  Spare
                                              ,8     ,3     ,11    ,9                // State:  7  Mozaic > Current Exposure Time  
                                              ,-1    ,7     ,-2    ,-2               // State:  8  Mozaic > Set Exposure Time  
                                              ,10    ,3     ,7     ,11               // State:  9  Mozaic > Current Total span     
                                              ,-1    ,9     ,-2    ,-2               // State: 10  Mozaic > Set Total span     
                                              ,12    ,3     ,9     ,7                // State: 11  Mozaic > Start Mosaic   
                                              ,-1    ,13    ,-1    ,-1               // State: 12  Mozaic > Mozaic in progress              (undo to cancel)
                                              ,11    ,12    ,12    ,12               // State: 13  Mozaic >>  Sure you want to cancel ?       (enter to confirm)
                                              ,-1    ,-1    ,-1    ,-1               // State: 14  Mozaic > spare
                                              ,-1    ,-1    ,-1    ,-1               // State: __  
                                              ,-1    ,-1    ,-1    ,-1               // State: __  
                                         };

 
////////////////////////////////// DEFINES /////////////////////////////////////   
#define baud             9600

char rs232_to=0;
void wait_rs232(void)
{
long timeout=10000;
while((UCSR0A & (1 <<UDRE0)) == 0)
   {
   if ( --timeout==0 ) 
      {
      rs232_to=1;
      return;
      }
   }
}

void pc(char ccc)
{
//while((UCSR0A & (1 <<UDRE0)) == 0);  // data register is empty...
wait_rs232();
UDR0 = ccc;
//while((UCSR0A & (1 <<UDRE0)) == 0);  // data register is empty...
}

void ps(char *ssz)   // super simple blocking print string
{
while ( *ssz ) pc(*ssz++);
}


void pgm_ps(const char *ssz)   // super simple blocking print string
{
char tc = pgm_read_byte(ssz++);
while ( tc )
   {
   pc(tc);
   tc = pgm_read_byte(ssz++);
   }
}

// digit is the demanded precision
char *pxxd(char *buf,long val,short digit)
{
short iii,nnn;
long  max=1;
if ( digit>12 ) digit = 12;
if ( digit<1  ) digit = 1;
nnn = val < 0;

for ( iii=0 ; iii<digit ; iii++ ) max=max*10;
//if(val<-(max-1))   // 10000000000  (10^digit)-1
//   {
//   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '-';   // below max negative displayable
//   }
//else if (val>(max*10))
//   {
//   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '+';   // above max positive displayable
//   }
//else
//   {
   if ( nnn ) val = -val;
   for ( iii=digit-1 ; iii>=0 ; iii-- , val /= 10 )
      {
      buf[iii] = val%10 + '0';
      }
//   if ( nnn ) buf[0] = '-';
//   else       buf[0] = ' ';  // was buf[0] = '+';
//   }
buf[digit] = 0;
return &buf[digit];
}

void p02x(char *buf,char val)
{
unsigned short v0,v1;
v0 = (val>>4)&0xF;
v1 =  val    &0xF;

if ( v0 >=0  && v0<=9  ) buf[0] = '0' + v0;
else                     buf[0] = 'A' + v0 - 10;
if ( v1 >=0  && v1<=9  ) buf[1] = '0' + v1;
else                     buf[1] = 'A' + v1 - 10;
}

void p08x(char *bof,long value)
{
unsigned char *p_it = (unsigned char*) &value;
p_it = (unsigned char*) &value;
p02x(&bof[6],*p_it++);
p02x(&bof[4],*p_it++);
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
bof[8]=0;
}

void p04x(char *bof,long value)
{
unsigned char *p_it = (unsigned char*) &value;
p_it = (unsigned char*) &value;
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
bof[4]=0;
}




////////////////////////////////////////// INTERRUPT SECTION ////////////////////////////////////////////
////////////////////////////////////////// INTERRUPT SECTION ////////////////////////////////////////////
////////////////////////////////////////// INTERRUPT SECTION ////////////////////////////////////////////
//-   
//-            Interrupt priority goes from high to low, from 1 to 20
//-  
//-  1  0x0000 RESET External Pin, Power-on Reset, Brown-out Reset and Watchdog System Reset
//-  2  0x0002 INT0 External Interrupt Request 0
//-  3  0x0004 INT1 External Interrupt Request 1
//-  4  0x0006 PCINT0 Pin Change Interrupt Request 0
//-  5  0x0008 PCINT1 Pin Change Interrupt Request 1
//-  6  0x000A PCINT2 Pin Change Interrupt Request 2
//-  7  0x000C WDT Watchdog Time-out Interrupt
//-  8  0x000E TIMER2 COMPA Timer/Counter2 Compare Match A
//-  9  0x0010 TIMER2 COMPB Timer/Counter2 Compare Match B
//-  10 0x0012 TIMER2 OVF Timer/Counter2 Overflow
//-  11 0x0014 TIMER1 CAPT Timer/Counter1 Capture Event                          | sp0c0 : handle proper telescope movement and IR decoding
//-  12 0x0016 TIMER1 COMPA Timer/Counter1 Compare Match A                       |     
//-  13 0x0018 TIMER1 COMPB Timer/Coutner1 Compare Match B    TIMER1_COMPB_vect  |      
//-  14 0x001A TIMER1 OVF Timer/Counter1 Overflow             TIMER1_OVF_vect    |
//-  15 0x001C TIMER0 COMPA Timer/Counter0 Compare Match A                               | ap0c0 : handle async operation like rs232
//-  16 0x001E TIMER0 COMPB Timer/Counter0 Compare Match B                               |
//-  17 0x0020 TIMER0 OVF Timer/Counter0 Overflow             TIMER0_OVF_vect            |
//-  18 0x0022 SPI, STC SPI Serial Transfer Complete
//-  19 0x0024 USART, RX USART Rx Complete                    USART_RX_vect
//-  20 0x0026 USART, UDRE USART, Data Register Empty
//-  21 0x0028 USART, TX USART, Tx Complete                   USART_TX_vect
//-  22 0x002A ADC ADC Conversion Complete
//-  23 0x002C EE READY EEPROM Ready
//-  24 0x002E ANALOG COMP Analog Comparator
//-  25 0x0030 TWI 2-wire Serial Interface
//-  26 0x0032 SPM READY Store Program Memory Ready
//-  

ISR(TIMER0_OVF_vect)     // 
{                       
rt_TIMER0++;
return;
}

ISR(TIMER1_COMPB_vect)   // 
{
return;
}

//  IR CODE:
//  The code is in the delay between "1"
//  __-----__-__-_-_-__-__-__-   
//  At 10 Khz sampling with the Proscan remote, with an average of 3 iteration 
//  The Signal stays at 1 for 3~4 iterations ; only the start bit is longer than 3~4 iterations
//  Then goes to 0 for either 0x0B~0x0C : Logic 0    or either 0x15~0x16 : Logic 1
//  The  Start bit is a "1" for ~ 0x2A then a "0" for !0x2A
//  The code with Proscan is about 25 bits of information

volatile unsigned short msms;
volatile unsigned char SS,MM,HH;

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
rt_TIMER1++;

if ( lcd_go_rt ) lcd_rt_print_next();

// static char tog=0;
// tog++;
// if ( rs232_to==0 )
//    {
//    if ( tog & 0x01 ) PORTD |=  0x40;    // Driving PORTD pin6
//    else              PORTD &= ~0x40;
//    }
// else
//    {
//    if ( tog & 0x02 ) PORTD |=  0x40; 
//    else              PORTD &= ~0x40;
//    }

msms++;
if ( msms >= 1000 ) 
   { 
   msms = 0;
   SS++;
   if ( SS >= 60 )
      {
      SS = 0;
      MM++;
      if ( MM >= 60 )
         {
         MM = 0;
         HH ++;
         if ( HH >= 24 ) HH = 0;
         }
      }
   }
}

ISR(USART_RX_vect)
{
d_USART_RX++;
}

ISR(USART_TX_vect)
{
d_USART_TX++;
}

// - used by timer routines - ISR(TIMER2_OVF_vect)
// - used by timer routines - {
// - used by timer routines - dd_v[DDS_DEBUG + 0x03]++;
// - used by timer routines - }

void init_rs232(void)
{
long temp = RT_CPU_K_FRAME*1000UL/(16UL*baud)-1;  // set uart baud rate register

UBRR0H = (temp >> 8);
UBRR0L = (temp & 0xFF);
UCSR0B= (1 <<RXEN0 | 1 << TXEN0 );  // enable RX and TX
//UCSR0B|= (1 <<RXCIE0);              // enable receive complete interrupt
//UCSR0B|= (1 <<TXCIE0);              // enable transmit complete interrupt
UCSR0C = (3 <<UCSZ00);              // 8 bits, no parity, 1 stop
}



extern void __bss_end;
extern void *__brkval;

int get_free_memory()
{
int free_memory;

if((int)__brkval == 0) free_memory = ((int)&free_memory) - ((int)&__bss_end);
else                   free_memory = ((int)&free_memory) - ((int)__brkval);

return free_memory;
}

/////////////////// 
// function to set l1 and l2 based on the current state
void set_line(void)
{
unsigned char id;
short i1,i2;
i1 = pgm_read_byte(&line1[d_state]) * 16;
i2 = pgm_read_byte(&line2[d_state]) * 16;
for ( id=0;id<16;id++ ) lcd_lines[id+0x00] = pgm_read_byte ( &linetxt[i1]+id);
for ( id=0;id<16;id++ ) lcd_lines[id+0x10] = pgm_read_byte ( &linetxt[i2]+id);
}

////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
int main(void)
{
///////////////////////////////// Init /////////////////////////////////////////

init_rs232();
rt_init_disp();

// init buzzer: T2 output B  on PD3 
   // TIMSK2 |= 1 <<  TOIE2;    // timer2 interrupt when Overflow                    ///////////////// SP0C0
   // TCCR2A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
   TCCR2A  = 0x23;           // FAST PWM, Clear OC2B on counter match, SET on BOTTOM
   TCCR2B  = 0x08 + 0x07;           // FAST PWM,  + divide by 1024 
   OCR2A   = 0;                   // Clock divider set to 1024, so 20 should give a wave at 1Khz
   OCR2B   = 0;                  // By default, set the PWM to 50%



ps("\033[2JLCD Tests...");

//set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();

PRR   = 0; // Enable all systems

DDRD |= 0x40;  // PD6 test

sei();         //enable global interrupts

lcd_init();

#define bit(BBB) (0x01<<BBB)
#define PORT_BIT_HIGH(P0RT,B1T) { P0RT |= bit(B1T); }

#define CANON_FOCUS_HIGH     PORTB |=  bit(7)          // PORT B bit 7
#define CANON_FOCUS_LOW      PORTB &= ~bit(7)

#define CANON_SHOOT_HIGH     PORTD |=  bit(5)          // PORT D bit 5
#define CANON_SHOOT_LOW      PORTD &= ~bit(5)

DDRB |= bit(7);  
DDRD |= bit(5);  
DDRD |= bit(3);   // Buzzer

#define BT_ENTER 1
#define BT_UNDO  2
#define BT_UP    3
#define BT_DOWN  4
#define BT_LONG  100

d_state=0;
lcd_go_rt = 1;

while(1)
   {
   static unsigned char tog;
   tog = !tog;

   if ( tog ) { CANON_FOCUS_HIGH; }
   else       { CANON_FOCUS_LOW;  }

   rt_wait_ms(10);
   if      (LCD_BUTTON_1 == 0 ) {button = BT_ENTER; }
   else if (LCD_BUTTON_2 == 0 ) {button = BT_UP;    }
   else if (LCD_BUTTON_3 == 0 ) {button = BT_UNDO;  }
   else if (LCD_BUTTON_4 == 0 ) {button = BT_DOWN;  }
   else                         {button = 0;        }

   if ( (button>0) && (button<=BT_LONG) ) button_down++;

   if      ( button_down == 1 )        { OCR2A   = 20;  OCR2B   = 10; }    // Beep
   else if ( button_down == BT_LONG )  { OCR2A   =  8;  OCR2B   =  4; }    // Beep high pitch on long push
   else                                { OCR2A   =  5;  OCR2B   = 10; }    // Sound off

   if ( button != button_last )
      {
      if ( button == 0 ) //  action on key-off
         {               // Check next state...
         unsigned char next = pgm_read_byte( &main_state_machine[d_state*4 + button_last]);
         //if ( (next > 0) && (next< sizeof(line1)) ) d_state = next;
         if ( (next > 0) && (next<25 ) ) d_state = next;
         }

      button_down=0;
      button_last = button;
      } 

   set_line();  // display the thing
   }

#ifdef ASDF
char buf[16];
long val=0;
long lon=0;
while(1)
   {
   val++;
   lon+=16;
   lcd_goto(0,0);

   lcd_print_str("Time:");
   pxxd(buf,HH,2);
   lcd_print_str(buf);
   lcd_print_str(":");
   pxxd(buf,MM,2);
   lcd_print_str(buf);
   lcd_print_str(":");
   pxxd(buf,SS,2);
   lcd_print_str(buf);

   lcd_goto(1,0);
   lcd_print_str("Long:  ");
   p04x(buf,lon);
   lcd_print_str(buf);

   LCD_SET_DATA_DIR_INPUT;
   rt_wait_ms(100);

   if ( SS < 5  ) CANON_FOCUS_HIGH;
   else           CANON_FOCUS_LOW; 

   if ( SS == 4 ) CANON_SHOOT_HIGH;
   else           CANON_SHOOT_LOW; 

   lon = lon & 0xFFF0;
   if (LCD_BUTTON_1 == 0 ) lon |= 0x01;
   if (LCD_BUTTON_2 == 0 ) lon |= 0x02;
   if (LCD_BUTTON_3 == 0 ) lon |= 0x04;
   if (LCD_BUTTON_4 == 0 ) lon |= 0x08;

   if      (LCD_BUTTON_1 == 0 ) { OCR2A   = 20;  OCR2B   = 10; }    // Clock divider set to 1024, so 20 should give a wave at 1Khz
   else if (LCD_BUTTON_2 == 0 ) { OCR2A   = 14;  OCR2B   =  7; }    // Clock divider set to 1024, so 20 should give a wave at 1Khz
   else if (LCD_BUTTON_3 == 0 ) { OCR2A   = 10;  OCR2B   =  5; }    // Clock divider set to 1024, so 20 should give a wave at 1Khz
   else if (LCD_BUTTON_4 == 0 ) { OCR2A   =  8;  OCR2B   =  4; }    // Clock divider set to 1024, so 20 should give a wave at 1Khz
   else                         { OCR2A   =  5;  OCR2B   = 10; }    // Clock divider set to 1024, so 20 should give a wave at 1Khz
   ps("\012\015...");

//   if (LCD_BUTTON_3 == 0 ) lcd_reset();

   }
#endif

}

// #include "a328p_lcd.c"   <-- I need the makefile to compile with -DUSE_LCD
//
//
