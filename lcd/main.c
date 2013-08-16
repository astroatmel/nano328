/*
Info:
Lazer rouge : 15mA a 4.5v   ( 500 ohms at 12v)
Lazer vert 5 miles: 150mA a 3v  ( 60 ohms at 12V)
Toshiba Step motor driver: 64 pulses required for a full cycle and to get the init pulse
Toshiba steo motor works well with 22pf which gives 400kHz
TODO
- I think pause mode has a bug, TODO: set tracking off, and always goto the same position to see if we goto the same place or if we advance

-Easy goto catalog position (planned destinations)
-Easy snapshot of current position

*/
#include <avr/pgmspace.h> 
#include <avr/io.h>
#include <avr/interrupt.h>

#include "a328p_lcd.h"

#include "a328p_rt.h"

#define bit(BBB) (0x01<<BBB)
#define PORT_BIT_HIGH(P0RT,B1T) { P0RT |= bit(B1T); }

#define CANON_FOCUS_HIGH     PORTB |=  bit(7)          // PORT B bit 7
#define CANON_FOCUS_LOW      PORTB &= ~bit(7)

#define CANON_SHOOT_HIGH     PORTD |=  bit(5)          // PORT D bit 5
#define CANON_SHOOT_LOW      PORTD &= ~bit(5)

// Photo delays:
#define SEQ_1 2500   // was 500
#define SEQ_2 4000   // was 1500
#define SEQ_3 6000   // was 2000
// CGEM: 
// on 24h,     one sec = 0xC22E
// on 360 deg, one sec = 0xCF2
//
// Recheck with Stellarium
// Newton              750mm ~ 1.4 FOV and about 10min in time from left to right
// CGEM should be ~   2400mm ~ 0.5 FOV and about 3min in time
// I'm not sure the values below are good looke like ONE_TENTH_OF_NEWTON_FOV is 2 minutes
// I have the feeling that the values should be:
// #define ONE_TENTH_OF_NEWTON_FOV (3314*60)   that is 1 minutes which should be 1/10 of Newton which is 10 min wide
// #define ONE_TENTH_OF_EDGEHD_FOV 
// But it kinda works with the values below:
#define ONE_TENTH_OF_NEWTON_FOV (0x61172*3)  // that would be 360 seconds (6 min) for an object to go from one edge to the other ?
#define ONE_TENTH_OF_EDGEHD_FOV 0x61172      // that would be 120 seconds (2 min) for an object to go from one edge to the other ?    so 1/10, 12 shoots, it means mozzaic_cgem would need to pause one second

#define ONE_DEG_CGEM 0xB60B61
#define ONE_MIN_CGEM 0x308B9
#define ONE_SEC_CGEM 0xCF2
#include "menu.h"
unsigned char menu_stack_ptr=0;
char menu_stack[10];
char set_tracking_mode=-1;  // -1 , no change 
char tracking_mode_p;       // previous tracking mode

unsigned short d_ram;
unsigned char lcd_go_rt=0;
unsigned short beep_time_ms;

unsigned short histo=0;
unsigned short histo_min=0;
unsigned short histo_max=0;
unsigned char  cgem_fb=0;

unsigned char m_minor,m_major;  // Mozaic sequence
PROGMEM const char m_seq_x[] = { 0, 0, 1,    1, 1,-1,   -1,-1, 0 };   // RA
PROGMEM const char m_seq_y[] = { 0, 1,-1,    0, 1,-1,    0, 1,-1 };   // DEC

// Print strinng
#ifdef TEST_P_VAL
PROGMEM const char p_testing_p_val[] = "Testing p_val()\015\012";
PROGMEM const char p_size[]          = "\015\012Size:";
PROGMEM const char p_case1[]         = "dont show +/-:\015\012";
PROGMEM const char p_case2[]         = "only show -:\015\012";
PROGMEM const char p_case3[]         = "show +/-:\015\012";
PROGMEM const char p_no_lead[]       = "No leading Zeros:\015\012";
PROGMEM const char p_lead[]          = "Leading Zeros:\015\012";
PROGMEM const char p_value[]         = "value:\015\012";
PROGMEM const long p_values[]        = {123,-123,-1,1,10,-10};
#endif

unsigned char  rs232_tx_cmd[32];  // null terminated
unsigned char  rs232_rx_cmd[32];  
volatile char  rs232_tx_ptr = -1;   // -1 means last tx complete and >0 means in progress of transmitting...
volatile short rs232_tx_cnt = 0;   // because in the CGEM communication, all characters are valid, even the NULL, I cant use 0x00 as the end, I have to use a counter [[ cnt points on the null ]]
volatile char  rs232_rx_ptr = 0;   // -1 means a command was received, -2 means overflow
short         rs232_com_to    = -1;  // Time-out
char rs232_com_state = 0;  // Odd values are states where we are waiting for an answer from CGEM
char mozaic_mode  = 0;    // goto or pause
char mozaic_state = 0;          // to help with the handshake...
char goto_state = 0;            // to help with the handshake...
short mozaic_wait = 0;    // dont wait for ever...
signed char rs232_edit=-1;
long          cdb_add_value=0;  // value to add to the current cdb label... 
long          cdb_initial_value=0;  // value to restore in case of UNDO
unsigned char cdb_initial_offset=0;  // value to restore in case of UNDO
unsigned char stack_before_edit=0;  // 
short dbg;

// States are:
// 0: Is aligned
// 2: Is in goto 
// 4: get current pos
// 6: if in mozaic mode and aligned, and the time is right, do a goto
                      
char timelap_state = 0;    // to help with the handshake...

volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;

volatile short pop_delay=0;

#define CDB_SIZE 50
volatile long cdb[CDB_SIZE];
//  0: Runnig time in seconds
//  1: RA from CGEM
//  2: DE from CGEN 
//  3: Exposure time in mili second
//  4: Delta time in mili-second between shoots
//  5: Shot #        total: 81 on a 9x9 grid
//  6: Total # shot
//  7: Shot RA  id   9x9 grid
//  8: Shot DEC id   9x9 grid
//  9: Communication Time-Out RS232 (no response from CGEM)
// 10: Aligned ?
// 11: In goto ?
// 12: Mozaic goto error
// 13: get position error {e}
// 14: Time since last position update
// 15: Mozaic span in degrees
// 16: Mozaic Running time in ms ( 1000-2000:focus    2000-> cdb[3]*1000: expose    then wait 4 second to store )
// 17: Mozaic Base position RA    ( also used by RECALL goto)
// 18: Mozaic Base position DEC   ( also used by RECALL goto)
// 19: Histogram Min
// 20: Histogram Cur
// 21: Histogram Max
// 22: Tracking Mode ? 0 1 2 3  (2 = EQ north  0 = off)  
// 23: Mozaic State
// 24: Pause Mode Counter
// 25: Free Mem RAM
// 26: Timelap Shot no
// 27: Timelap Period counter
// 28: Nb pictures on RA axis
// 29: Nb pictures on REC axis
// 30: Debug 1
// 31: Debug 2
// 32: Debug 3
// 33: Debug 4
// 34: Debug 5
// 35: Debug 6
// 36: Stored RA #1
// 37: Stored RA #2
// 38: Stored RA #3
// 39: Stored RA #4
// 40: Stored RA #5
// 41: Stored RA MozStartPos
// 42: Stored DE #1
// 43: Stored DE #2
// 44: Stored DE #3
// 45: Stored DE #4
// 46: Stored DE #5
// 47: Stored DE MozStartPos


short d_state;          // main state machine that controls what to display
short d_debug=0;        // Secondary state machine for debug info
short d_debug_page=M_DEBUG_FIRST;   // Secondary state machine for debug info
volatile unsigned char  button_rt_sp,button_rt_lp;   // set by foreground
unsigned char  button_sp,button_lp;                  // local usage
unsigned char  button_last_lp;                       // long bush transition detection
unsigned char  button_hold_sp,button_hold_lp;        // just to see what's the last detected button input


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

// Copy a null terminated string from PGM space
short pgm_to_mem(const char *src,unsigned char *dst)
{
short count=0;
unsigned char CCC;
while ( (CCC = pgm_read_byte( src++ )) ) 
   {
   *dst++ = CCC;
   count++;
   }
*dst++ = 0;
return count;
}

void rt_p_pgm(const char *src)
{
unsigned char iii;
rs232_tx_cnt = pgm_to_mem(src,rs232_tx_cmd);
rs232_tx_ptr    = 0;     // FG... pls tx the command
while ( rs232_tx_ptr >=0 ) ;  // wait...
for ( iii=0 ; iii < sizeof(rs232_tx_cmd) ; iii++ ) rs232_tx_cmd[iii]=0;  // clear the string
}

// digit is the demanded precision
short p_val(unsigned char *buf,long val,unsigned char option_digit)
{
unsigned char nnn;
unsigned char digit = 1 + (option_digit & 0x07);  // 3 lsb = nb digits
unsigned char sign = (option_digit & 0x30);  // this bit means...  0x00: dont show +/-  0x10: show only "-" if negative   0x30: show +/-
unsigned char lead = (option_digit & 0x40);  // this bit means we want to display leaging zeros
short iii;
long  max;
nnn = val < 0;
if ( (sign == 0x10) && (nnn==0) ) sign=0;  // no sign snown if only displaying "-" and the value is positive
if ( sign ) max = 1;
else        max = 10; // if not displaying signs, then we have one more character

for ( iii=1 ; iii<digit ; iii++ ) max=max*10;
if(val<=-max)   // 10000000000  (10^digit)-1
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '-';   // below max negative displayable
   }
else if (val>=max)
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '+';   // above max positive displayable
   }
else
   {
   if ( nnn ) val = -val;
   iii=digit-1;
   if  ( val == 0 )            buf[iii--] = '0';  
   for ( ; val>0 ; val /= 10 ) buf[iii--] = val%10 + '0';
// if ( iii < 0 ) return; // no space left
   if ( (lead!=0) && ( iii >= 0 ))
      {
      while (iii>0) buf[iii--] = '0';
      if ( sign == 0 ) buf[iii--] = '0';
      }
   if ( sign ) 
      {
      if ( iii<0 ) iii=0;
      if ( nnn )               buf[iii--] = '-';
      else if ( sign == 0x30 ) buf[iii--] = '+';
      }
   while (iii>=0) buf[iii--] = ' ';
   }
return digit;
}

long p_cgem_coord_sub(long *m128,long div)
{
long rrr;
 rrr  = *m128 / div;
*m128 = *m128 % div; 
return rrr;
}
// Convert long to deg,min,sec
void p_cgem_coord(unsigned char *bof,long *p_value,unsigned char options)
{
char  opt = options & 0x30;  // 0x00 = No orientation, just degrees // 0x01 = North/South // 0x02 = hour/min/sec // 0x03 = East/West
long  val = *p_value;
char  neg = val<0;
short q128;  // lowest common denominator
long  m128;
long  hour=0,deg=0,min=0,sec=0,cen=0;

if ( (neg!=0) && (opt!=0x20)) val = -val;
q128 = val >> 25;        // div by 2^7 lowest common denominator between 360*60*60 and 0x1000000   // 2^3*3^2*5 * 2^2*3*5 * 2^2*3*5  and 2^24
q128 = q128 & 0x7F;      // get rid of any signh extend
m128 = val & 0x1FFFFFF;  // (modulo) value between 0 and 10125 

if ( opt == 0x20 )
   { // in hour/min/sec, q128 = 24h/128 = 0.1875h = 11.25m = 0 hour + 11 min + 15 sec (flush)   which is:  0x2000000 
   sec = q128 * 15;
   min = q128 * 11;
   min += 5  * p_cgem_coord_sub(&m128,0xE38E39); // 5 min = 0xE38E39 = 14913980.89
   min += 2  * p_cgem_coord_sub(&m128,0x5B05B0); 
   min += 1  * p_cgem_coord_sub(&m128,0x2D82D8); 
   sec += 30 * p_cgem_coord_sub(&m128,0x16C16C);
   sec += 15 * p_cgem_coord_sub(&m128,0xB60B6);
   sec += 5  * p_cgem_coord_sub(&m128,0x3CAE7);
   sec += 1  * p_cgem_coord_sub(&m128,0xC22E);
   cen += 50 * p_cgem_coord_sub(&m128,0x6117);
   cen += 25 * p_cgem_coord_sub(&m128,0x308C);
   cen += 5  * p_cgem_coord_sub(&m128,0x9B6);
   cen += 1  * p_cgem_coord_sub(&m128,0x1F1);
   }
else  
   { // in deg/min/sec, q128 = 360/128 = 2.8125deg = 2 deg + 48 min + 45 sec   (flush)   which is 0x2000000
   deg = q128 * 2;
   min = q128 * 48;
   sec = q128 * 45;
   deg += 1  * p_cgem_coord_sub(&m128,0xB60B61); // 1 deg = 0xB60B61 = 11930464.71
   min += 30 * p_cgem_coord_sub(&m128,0x5B05B0); 
   min += 10 * p_cgem_coord_sub(&m128,0x1E573B); 
   min += 5  * p_cgem_coord_sub(&m128,0xF2B9D); 
   min += 1  * p_cgem_coord_sub(&m128,0x308B9); 
   sec += 30 * p_cgem_coord_sub(&m128,0x1845C); 
   sec += 10 * p_cgem_coord_sub(&m128,0x8174); 
   sec += 5  * p_cgem_coord_sub(&m128,0x40BA); 
   sec += 1  * p_cgem_coord_sub(&m128,0xCF2); 
   cen += 50 * p_cgem_coord_sub(&m128,0x679); 
   cen += 10 * p_cgem_coord_sub(&m128,0x14B); 
   cen += 1  * p_cgem_coord_sub(&m128,0x21); 
   }

// add-up
if ( cen>99 ) cen = 99;  // centime cant go over 99
min  += sec / 60;
sec  %= 60;
deg  += min / 60;
hour += min / 60;
min  %= 60;
// 0123456789012
// W123'32m32.4s
//  23h32m32.12s
if ( opt == 0x20 )
   {
   bof[0] = ' ';
   p_val(&bof[1],hour,0x00 + 0x01);
   bof[3] = 'h';
   p_val(&bof[4],min,0x40 + 0x01);
   bof[6] = 'm';
   p_val(&bof[7],sec,0x40 + 0x01);
   bof[9] = '.';
   p_val(&bof[10],cen,0x40 + 0x01);
   bof[12] = 's';
   }
else    // ex:  40000000,20000000#   20000000,10000000#   10000000,22222200#    11111100,00000000#    58009F3A,A7FF60C6#   A7FF60C6,00000000#   40000000,00000000#
   {    //      E 90 deg             E 45 deg             E 22deg 30 min        E 23 deg 59m 59.9s    E 123d 45" 12.3'
   if ( opt == 0x10 )
      {
      if ( neg ) bof[0] = 'S';
      else       bof[0] = 'N';
      }
   else if ( opt == 0x30 )
      {
      if ( neg ) bof[0] = 'W';
      else       bof[0] = 'E';
      } 
   else          bof[0] = ' ';

   p_val(&bof[1],deg,0x00 + 0x02);
   bof[4] = 1;  // deg
   p_val(&bof[5],min,0x40 + 0x01);
   bof[7] = 2;  // min
   p_val(&bof[8],sec,0x40 + 0x01);
   bof[10] = '.';
   p_val(&bof[11],cen/10,0x40 + 0x00);
   bof[12] = 3; // sec
   }
}

void p02x(unsigned char *buf,char val)
{
unsigned short v0,v1;
v0 = (val>>4)&0xF;
v1 =  val    &0xF;

if ( v0 >=0  && v0<=9  ) buf[0] = '0' + v0;
else                     buf[0] = 'A' + v0 - 10;
if ( v1 >=0  && v1<=9  ) buf[1] = '0' + v1;
else                     buf[1] = 'A' + v1 - 10;
}

void p08x(unsigned char *bof,long *p_value)
{
unsigned char *p_it = (unsigned char*) p_value;
p02x(&bof[6],*p_it++);
p02x(&bof[4],*p_it++);
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
}

void p04x(unsigned char *bof,long *p_value)
{
unsigned char *p_it = (unsigned char*) p_value;
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
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

#define BT_UNDO  1
#define BT_ENTER 2
#define BT_UP    3
#define BT_DOWN  4
#define BT 10
#define BT_LONG  1000            // a long push is 1 sec (1000 ticks)

volatile unsigned short msms;
volatile unsigned char TT,SS,MM,HH;

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 1 KHz
{
static unsigned char  button_rt,button_last_rt, button_sp_sp;
static unsigned short button_down_ms;   // check how long a button is held down

rt_TIMER1++;

///////////////////////////// Edit Logic  /////////////////////////////
if ( rs232_edit ) lcd_lines[rs232_edit] = 0;   // in edit mode, show the up/down character where required
if ( lcd_go_rt )  lcd_rt_print_next();
///////////////////////////// Edit Logic  /////////////////////////////
if ( beep_time_ms )
   {
   OCR2B = 10;
   OCR2A = 20;
   if ( 0 == --beep_time_ms ) OCR2B = 254;  // stop the beep
   }

button_last_rt = button_rt;
if      (LCD_BUTTON_1 == 0 ) {button_rt = BT_ENTER; }
else if (LCD_BUTTON_2 == 0 ) {button_rt = BT_UP;    }
else if (LCD_BUTTON_3 == 0 ) {button_rt = BT_UNDO;  }
else if (LCD_BUTTON_4 == 0 ) {button_rt = BT_DOWN;  }
else                         {button_rt = 0;        } // _sp gets set when button is at least 10ms down

if ( button_rt )  
   {
   if      ( button_down_ms <= BT_LONG)   { button_down_ms++;  }                               // check how long the button is down;
   if      ( button_down_ms == 10 )       { beep_time_ms = 10; button_sp_sp = button_rt; }    // Beep
   else if ( button_down_ms == BT_LONG )  { beep_time_ms = 10; button_rt_lp = button_rt; }    // Beep high pitch on long push, set button_rt_lp
   }
else
   {
   if ( (button_sp_sp!=0) && (button_rt_lp==0)) button_rt_sp = button_sp_sp;  // if there was a beep for short push, then set button_rt_sp
   button_sp_sp   = 0;
   button_down_ms = 0;
   button_rt_lp   = 0;
   }

msms++;
if ( msms >= 100 ) 
   { 
   msms = 0;
   TT++;                              // Tenths

   if ( pop_delay   > 1 ) pop_delay--;
   if ( mozaic_wait > 0 ) mozaic_wait--;

   if ( TT >= 10 )
      {
      TT = 0;
      SS ++;                          // Seconds
      cdb[0]++; // running time
      cdb[27]+=1000;  // Period count
// THIS THE MY BUG IN THE PAUSE MOZAIC      if ( cdb[22] == 0 ) cdb[17] += 0xC22E;  // when tracking is off, move the mozaic origin with time to be able to debug

      if ( SS >= 60 )
         {
         SS = 0;
         histo_min = histo_max = histo; // reset min max every minute
         MM ++;                       // Minutes
         if ( MM >= 60 )
            {
            MM = 0;
            HH ++;                    // Hours
            if ( HH >= 24 ) HH = 0;
            }
         }
      }
   }

if ( rs232_tx_ptr >= 0 )
   {
   if (UCSR0A & (1 <<UDRE0)) // TX register empty
      {
      UDR0 = rs232_tx_cmd[(unsigned char)rs232_tx_ptr++];
//      if ( rs232_tx_cmd[(unsigned char)rs232_tx_ptr] == 0 ) rs232_tx_ptr = -1;  // we are done
      if ( rs232_tx_ptr == rs232_tx_cnt ) rs232_tx_ptr = -1;  // we are done
      }
   if ( rs232_tx_ptr > sizeof(rs232_tx_cmd) ) rs232_tx_ptr = -1;  // we are done
   }
if ( rs232_rx_ptr>=0 )   // real-time has control on buffer
   {
   if ( (UCSR0A & 0x80) != 0 )    // ********** CA CA LIT BIEN...     // RX register full
      {
      unsigned char CCC = UDR0;
      rs232_rx_cmd[(unsigned char)rs232_rx_ptr++] = CCC;
      if ( CCC == '#' ) rs232_rx_ptr = -1; // CGEM command complete...
      }
   }
if ( rs232_rx_ptr >= sizeof(rs232_rx_cmd) ) rs232_rx_ptr = -2; // overflow...

if ( rs232_com_to>=0 ) rs232_com_to++;  // monitor how long it takes to get the response

cdb[14]++;  // monitor how old the position from CGEM is

//WAS if ( (d_state == 12) || (d_state == 13) || (d_state == 19) || (d_state == 26) || (d_state == 36) || (d_state == 37) || (d_state == 38) || (d_state == 44))
if ( (d_state >= M__MOZ_RUN_FIRST) && (d_state <= M_CANCELING_FIRST))  // Shutter controlled group
   {
   if ( cdb[24] > 0 ) cdb[24]--;  // pause counter in miliseconds

   if      ( cdb[16]<SEQ_1 ) // initial delay
      {
      cdb[16] ++;
      CANON_FOCUS_LOW;
      CANON_SHOOT_LOW;
      }
   else if ( cdb[16]<SEQ_2 ) // Focus
      {
      cdb[16] ++;
      CANON_FOCUS_HIGH;   // Led on
      CANON_SHOOT_LOW;
      }
   else if ( cdb[16]<cdb[3]+SEQ_2 ) // Expose
      {
      cdb[16] ++;
      CANON_FOCUS_HIGH;   // Led on
      CANON_SHOOT_HIGH;   // Led on
      }
   else if ( cdb[16]<cdb[3]+SEQ_3 ) // Store image (the move is so long with CGEM, no need to wait for image storage...
      {
      cdb[16] ++;
      CANON_FOCUS_LOW;
      CANON_SHOOT_LOW;
      }
   }
else // not in mozaic mode
   {
   cdb[16] = d_state;
   CANON_FOCUS_LOW;
   CANON_SHOOT_LOW;
   }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   This section terminates the interrupt routine and will help estimate how much CPU time we use in the real-time interrupt
//   This SP0 uses soooo little cpu time that I do the histogram math here  (currently using 5% of 1ms)
if ( histo < histo_min ) histo_min = histo;
if ( histo > histo_max ) histo_max = histo;
// This assumes that RT_CPU_K_FRAME is 20000 (we are running at 1Khz) 
///cdb[19] = ((long)histo_min*13421)>>24;  // value will go from 0 to 15
///cdb[20] = ((long)histo    *13421)>>24;  // value will go from 0 to 15
///cdb[21] = ((long)histo_max*13421)>>24;  // value will go from 0 to 15
cdb[19] = ((long)histo_min*13421)>>21;  // value will go from 0 to 15*8
cdb[20] = ((long)histo    *13421)>>21;  // value will go from 0 to 15*8
cdb[21] = ((long)histo_max*13421)>>21;  // value will go from 0 to 15*8
if ( cdb[19] < 1 ) cdb[19]=1;  
if ( cdb[20] < 2 ) cdb[20]=2;
if ( cdb[21] < 3 ) cdb[21]=3;
if ( cdb[19] == cdb[20] ) cdb[19] = cdb[20] - 1;
if ( cdb[21] == cdb[20] ) cdb[21] = cdb[20] + 1;
histo = TCNT1;  // the division takes a lot of cycles ... about 40% of one iteration !  so lets do the rest outside sp0
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

// function that convert "cnt" characters from rs232_rx_cmd starting at "where"
// used to decode feedback from CGEM
unsigned char rs232_rx_hex(unsigned char where, unsigned char cnt,long *tmp)
{
unsigned char iii,jjj,kkk;
for ( *tmp=iii=kkk=0 ; iii<cnt ; iii++) 
   {
   jjj = rs232_rx_cmd[iii+where];
   if ( (jjj >= '0') && (jjj<= '9') ) kkk = jjj-'0';
   else if ( (jjj >= 'A') && (jjj<= 'F') ) kkk = jjj-'A' + 10;
   else if ( (jjj >= 'a') && (jjj<= 'f') ) kkk = jjj-'a' + 10;
   else return -1;  // problem
   *tmp = *tmp*16 + kkk;
   }
return 0;
}


void pop(char next)
{
if ( ( (next+menu_stack_ptr) >= 0 ) && (next<0) )
   {
   menu_stack_ptr += next;
   d_state = menu_stack[menu_stack_ptr];
   }
}
void push(char next)
{
if ( menu_stack_ptr < sizeof(menu_stack) ) menu_stack[menu_stack_ptr++] = next;
}
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
int main(void)
{
unsigned char iii,jjj,kkk,nb_display=0;
unsigned char l_TT=0;
long lll,mmm;
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

//set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
cdb[25] = d_ram = get_free_memory();

PRR   = 0; // Enable all systems

DDRD |= 0x40;  // PD6 test

sei();         //enable global interrupts

lcd_init();

//////////////////////////////////// p_val() tests ////////////////////////////////////
#ifdef TEST_P_VAL
rt_p_pgm(p_testing_p_val);
lll = 123;
for ( kkk=0 ; kkk<6 ; kkk++ )   // Some values...
   {
   lll = pgm_read_dword(&p_values[kkk]);
   rt_p_pgm(p_value);
   p08x(rs232_tx_cmd,&lll); 
   rs232_tx_cnt    = 8;     // 8 char to tx
   rs232_tx_ptr    = 0;     // FG... pls tx the command
   while ( rs232_tx_ptr >=0 ) ;  // wait...

   for ( jjj=0 ; jjj<=0x40 ; jjj+=0x40 )   // Leading zeros
      {
      if ( jjj==0 ) rt_p_pgm(p_no_lead);
      else          rt_p_pgm(p_lead);
      for ( iii=0 ; iii<7 ; iii++ )   // for all the sizes
         {
         rt_p_pgm(p_size);
         rs232_tx_cmd[0] = iii + '0';
         rs232_tx_cmd[1] = 0x0D;
         rs232_tx_cmd[2] = 0x0A;
         rs232_tx_cnt    = 3;     // 3 ch to transmit
         rs232_tx_ptr    = 0;     // FG... pls tx the command
         while ( rs232_tx_ptr >=0 ) ;  // wait...
         rs232_tx_cnt = p_val(rs232_tx_cmd,lll,0x00 + jjj + iii); 
         rs232_tx_ptr    = 0;     // FG... pls tx the command
         while ( rs232_tx_ptr >=0 ) ;  // wait...
         }
       
      }
   }
#endif
///////////////////////////////////////////////////////////////////////////////////////
DDRB |= bit(7);  
DDRD |= bit(5);  
DDRD |= bit(3);   // Buzzer

d_state=1;
lcd_go_rt = 1;

cdb[3]  = 2*1000;    // default exposure time: 30 mili seconds  (mozaic)
cdb[4]  = 60;        // default delta time: 60 seconds (time laps)
cdb[15] = ONE_TENTH_OF_EDGEHD_FOV;   // default span: 1/10 of EDGE HD  (0.333 deg / 10 = 0.03 deg = 2 min)   { thats 1/30 of Newton)
cdb[28] = 9;         // Pause method, nb of pictures on RA axis (paused ones)
cdb[29] = 9;         // Pause method, nb of pictures on DEC axis (goto ones)

while(1)
   {
   static signed char c_edt=-1,l_edt=-1;  // current and last edit mode
   static unsigned char id;
   static unsigned char refresh=1;
   static unsigned char set[10],fmt[10],pos[10],off[10]; // show/edit format position cdb-offset    <- values used to set/show values on LCD
   short i1,i2;
   static char next;

   c_edt = (signed char) pgm_read_byte( &menu_state_machine[d_state*MENU_TABLE_WIDTH + 6]);

   //////////////////////////////////////////////////
   if ( button_sp ) button_hold_sp = button_sp;    // This section isolates the foreground from the background
   if ( button_lp ) button_hold_lp = button_lp;    // sp0 can set button_rt_lp and button_rt_sp at any time
   button_last_lp = button_lp;                     // but we set our local copy
   button_lp = button_rt_lp;                       // only at the begining of the background loop
   button_sp = 0;                                  // this is important to make sure
   if ( button_rt_sp )                             // that the changes occurs at the start of the loop
      {                                            // 
      button_sp = button_rt_sp;                    // this waym any logic below will always catch the changes
      button_rt_sp = 0;                            //
      }                                            //
   //////////////////////////////////////////////////

   if ( pop_delay == 1 )
      {
      pop_delay = 0;
      refresh=1;
      if ( d_state == M_EXECUTE_FIRST ) menu_stack_ptr = stack_before_edit;  // ugly... I know...
      pop(-1);
      }
//   lcd_lines_nmi[0x0A] = '0' + menu_stack[menu_stack_ptr-2]/10;
//   lcd_lines_nmi[0x0B] = '0' + menu_stack[menu_stack_ptr-2]%10;
//   lcd_lines_nmi[0x0A] = '0' + dbg                         /10;
//   lcd_lines_nmi[0x0B] = '0' + dbg                         %10;
//   lcd_lines_nmi[0x0C] = '0' + menu_stack_ptr              /10;
//   lcd_lines_nmi[0x0D] = '0' + menu_stack_ptr              %10;
//   lcd_lines_nmi[0x0E] = '0' + d_state                     /10;
//   lcd_lines_nmi[0x0F] = '0' + d_state                     %10;
//
//   lcd_lines_nmi[0x1E] = '0' + button_lp;
//   lcd_lines_nmi[0x1F] = '0' + button_sp;

   if ( d_debug )
      {
      if ( (button_sp==BT_UP) || (button_sp==BT_DOWN) )    // on key press
         {
         d_debug_page = pgm_read_byte( &menu_state_machine[d_debug_page*MENU_TABLE_WIDTH + button_sp]);   // Check next state...
         refresh = 1;
         }
      }
   else
      {
      if ( button_sp )    // on key press
         {
         refresh = 1;
         next = pgm_read_byte( &menu_state_machine[d_state*MENU_TABLE_WIDTH + button_sp]);   // Check next state...
         if ( (next>0) && (next<= MENU_TABLE_LEN) )
            { 
            if ( button_sp == BT_UNDO )
               {
               push(d_state);
               }
            if ( button_sp == BT_ENTER)  // push on the stack
               {
               push(d_state);
               if ( next == M_PRESET_FIRST )
                  {
                  short menu_caller = menu_stack[menu_stack_ptr-1];   // who called us ?

                  pop_delay = 10; // display "done" for one second to give feedback
                  if (  menu_caller == M_MOZAIC_FIRST + 1 ) cdb[15] = ONE_TENTH_OF_EDGEHD_FOV;
                  if (  menu_caller == M_MOZAIC_FIRST + 2 ) cdb[15] = ONE_TENTH_OF_NEWTON_FOV;
                  if (  menu_caller == M_MOZAIC_FIRST + 3 ) cdb[3] = 1000;
                  if (  menu_caller == M_MOZAIC_FIRST + 4 ) cdb[3] = 30000;
                  if (  menu_caller == M_MOZAIC_FIRST + 5 ) cdb[3] = 60000;
   
                  if (  menu_caller == M_MOZAIC_PAUSE_FIRST + 1 ) cdb[15] = ONE_TENTH_OF_EDGEHD_FOV;
                  if (  menu_caller == M_MOZAIC_PAUSE_FIRST + 2 ) cdb[15] = ONE_TENTH_OF_NEWTON_FOV;
                  if (  menu_caller == M_MOZAIC_PAUSE_FIRST + 3 ) cdb[3] = 1000;
                  if (  menu_caller == M_MOZAIC_PAUSE_FIRST + 4 ) cdb[3] = 30000;
                  if (  menu_caller == M_MOZAIC_PAUSE_FIRST + 5 ) cdb[3] = 60000;
                  if ( (menu_caller >= M_STORE_FIRST) &&
                       (menu_caller <= M_STORE_LAST)    )
                     {
cdb[34] =            cdb[36+menu_caller-M_STORE_FIRST] = cdb[1];  // Store current Pos
cdb[35] =            cdb[42+menu_caller-M_STORE_FIRST] = cdb[2];  // Store current Pos
                     }
                  if ( (menu_caller >= M_RECALL_FIRST) &&
                       (menu_caller <= M_RECALL_LAST)    )
                     {
                     cdb[15]=cdb[8]=cdb[7]=0;
                     cdb[17] = cdb[36+menu_caller-M_RECALL_FIRST];
                     cdb[18] = cdb[42+menu_caller-M_RECALL_FIRST];
                     mozaic_state = 1; // GOTO requested
                     }
                  }
               if ( next == M_EXECUTE_FIRST )
                  {              // display "Executing..." for one 0.5 sec to give feedback ( I wanted to use this state to "execute" the change )
                  pop_delay = 5; // but I cant because if I use a constant cdb locatio to edit, then I cant know where the value goes ):
                  }
               if ( (next == M__MOZ_RUN_FIRST) || (next == M__MOZ_PAUSE_RUN_FIRST) )  // When Starting Moz, store start pos
                  {
                  cdb[41] = cdb[17] = cdb[1];  // Save start RA      (base position for mozaic)
                  cdb[47] = cdb[18] = cdb[2];  // Save start DEC     (base position for mozaic)
                  }
               }
            d_state = next;
            }
         else if ( next<0 )  // pop the stack...
            {
            pop(next);
            }
   
         if ( button_sp == BT_UNDO) 
            {
            signed char t_edt = (signed char)pgm_read_byte( &menu_state_machine[d_state*MENU_TABLE_WIDTH + 5]);
            if ( (c_edt!=-1) && (t_edt==-1) ) cdb[cdb_initial_offset] = cdb_initial_value;  // UNDO in EDIT mode : restore initial value
            }
   
         } 
      if ( button_lp || button_sp) 
         {
         short inc_id = (signed char) pgm_read_byte( &menu_state_machine[d_state*MENU_TABLE_WIDTH + 5]);
         static char ll_TT;  // last tenth
         if ( inc_id >= 0 ) // we are in edit mode...
            {
            long value = pgm_read_dword(&edit_increment[inc_id]);
            if ( button_sp )
               {
               if ( button_sp == BT_UP)   cdb_add_value =  value;
               if ( button_sp == BT_DOWN) cdb_add_value = -value;
               }
            else if ( ll_TT != TT )
               {
               if ( button_lp == BT_UP)   cdb_add_value =  value;
               if ( button_lp == BT_DOWN) cdb_add_value = -value;
               }
            }
         ll_TT = TT;
         }
      } // end of the else d_debug
   if ( refresh )
      {
      refresh = 0;
      if ( d_debug == 0 )
         {
         i1 = pgm_read_byte(&menu_state_machine[d_state*MENU_TABLE_WIDTH + 7]) * 16;
         i2 = pgm_read_byte(&menu_state_machine[d_state*MENU_TABLE_WIDTH + 8]) * 16;
         }
      else
         {
         i1 = pgm_read_byte(&menu_state_machine[d_debug_page*MENU_TABLE_WIDTH + 7]) * 16;
         i2 = pgm_read_byte(&menu_state_machine[d_debug_page*MENU_TABLE_WIDTH + 8]) * 16;
         }

      for ( id=0;id<16;id++ ) 
         {
         lcd_lines[id+0x00] = pgm_read_byte ( &linetxt[i1]+id);
         lcd_lines[id+0x10] = pgm_read_byte ( &linetxt[i2]+id);
         } 

      nb_display = 0; // assume nothing to do
      for ( id=0;id<32;id++ ) // check for printable / editable strings
         {
         if ( (lcd_lines[id] == 0xA2) || (lcd_lines[id] == 0xA3) )    // we have something to do...   \242 \243
            {
            set[nb_display] = lcd_lines[id]==0xA2;   // show ?
            fmt[nb_display] = lcd_lines[id+1]; // format
            off[nb_display] = lcd_lines[id+2]; // CDB label offset
            pos[nb_display] = id;
            nb_display++;
            }
         } 
      }
         
   if ( (button_lp != 0) && (button_last_lp == 0) )  // on lp rising edge
      {
      if ( button_lp == BT_UNDO ) // long undo forces a reset of the state (almost the same as a reset, except most values are kept
         {
         d_state=1; // long undo 
         d_debug=0; 
         menu_stack_ptr = 0;
         }
      if ( button_lp == BT_ENTER ) // long undo forces a reset of the state (almost the same as a reset, except most values are kept
         {
         if ( d_debug == 0 ) d_debug = 1;  // go to first debug page
         else                d_debug = 0;              // go to normal operation
         refresh ++ ;
         }
      }
   
   for ( iii=0 ; iii < nb_display ; iii++ )  // display required values
      {
      lll = cdb[off[iii]+0];
      mmm = cdb[off[iii]+1];
      jjj = pos[iii];
      if ( cdb_add_value !=0 )  // edit mode requests add/remove..
         {
         cdb[off[iii]] += cdb_add_value;
         cdb_add_value = 0;
         }

      if ( fmt[iii] == 'n' )  // @nX format  (normal number)
         {
         p_val(&lcd_lines[jjj],lll,0x10 + 0x02);
         }
      else if ( fmt[iii] == 's' )  // @sX format  (seconds)
         {
         p_val(&lcd_lines[jjj],lll/1000,0x10 + 0x02);
         }
      else if ( fmt[iii] == 'y' )  // @yX format  (yes/no)
         {
         if ( (lll==1) || (lll=='1') ) 
            {
            lcd_lines[jjj+0] = 'y';
            lcd_lines[jjj+1] = 'e';
            lcd_lines[jjj+2] = 's';
            }
         else
            {
            lcd_lines[jjj+0] = 'n';
            lcd_lines[jjj+1] = 'o';
            lcd_lines[jjj+2] = ' ';
            }
         }
//      else if ( fmt[iii] == 'R' || fmt[iii] == 'D' || fmt[iii] == 'H' )  // temp test becasue they seem to jam  ... see p_cgem_coord()
//         {
//         p08x(&lcd_lines[jjj],&lll);
//         }
      else if ( fmt[iii] == 'R' )  // @RX format  (RA)      
         {
         p_cgem_coord(&lcd_lines[jjj],&lll,0x10);
         }
      else if ( fmt[iii] == 'D' )  // @DX format  (DEC)
         {
         p_cgem_coord(&lcd_lines[jjj],&lll,0x30);
         }
      else if ( fmt[iii] == 'H' )  // @HX format  (Hour)
         {
         p_cgem_coord(&lcd_lines[jjj],&lll,0x20);
         }
      else if ( fmt[iii] == 'd' )  // @dX format  (span)
         {
         p_cgem_coord(&lcd_lines[jjj],&lll,0x00);
         }
      else if ( fmt[iii] == 'E' )  // @EX format  (8 Char leading zeros)
         {
         p_val(&lcd_lines[jjj],lll,0x00 + 0x06);
         }
      else if ( fmt[iii] == 'e' )  // @eX format  (4 Char leading zeros)
         {
         p_val(&lcd_lines[jjj],lll,0x00 + 0x03);
         }
      else if ( fmt[iii] == 'o' )  // @oX format  x of y
         {
         p_val(&lcd_lines[jjj+0],lll,0x00 + 0x02);
         lcd_lines[jjj+4] = 'o';
         lcd_lines[jjj+5] = 'f';
         p_val(&lcd_lines[jjj+6],mmm,0x00 + 0x02);
         }
      else if ( fmt[iii] == 'p' )  // @oX format  x of y
         {
         p_val(&lcd_lines[jjj+0],lll,0x30 + 0x01);
         lcd_lines[jjj+2] = ',';
         p_val(&lcd_lines[jjj+3],mmm,0x30 + 0x01);
         }
      else if ( fmt[iii] == 'q' )  // CGEM tracking mode: off / eq-north / eq-south / 
         {
         if ( lll == 0 )
            {
            lcd_lines[jjj+0] = 'o';
            lcd_lines[jjj+1] = 'f';
            lcd_lines[jjj+2] = 'f';
            } 
         else if ( lll == 1 )
            {
            lcd_lines[jjj+0] = 'a';
            lcd_lines[jjj+1] = 'z';
            lcd_lines[jjj+2] = 'i';
            } 
         else if ( lll == 2 )
            {
            lcd_lines[jjj+0] = 'E';
            lcd_lines[jjj+1] = 'q';
            lcd_lines[jjj+2] = 'N';
            } 
         else if ( lll == 3 )
            {
            lcd_lines[jjj+0] = 'E';
            lcd_lines[jjj+1] = 'q';
            lcd_lines[jjj+2] = 'S';
            } 
         }
      else if ( fmt[iii] == 'x' )  // @xX format  (0x????????)
         {
         p08x(&lcd_lines[jjj],&lll);
         }
      else if ( fmt[iii] == 'X' )  // 
         {
         p04x(&lcd_lines[jjj],&lll);
         }
      else if ( fmt[iii] == 'm' )  // format  xxx.xxx ms   to display exposition time
         {
         long  sec,msec;
         if ( lll < SEQ_2 ) sec = msec = 0;
         else if ( (lll - SEQ_2) < cdb[3] ) 
            {
            sec  = (lll-SEQ_2)/1000;
            msec = (lll-SEQ_2)%1000;
            }
         else  // reached final exposure time
            {
            sec  = cdb[3]/1000;
            msec = cdb[3]%1000;
            }
         p_val(&lcd_lines[jjj+0],sec,0x00 + 0x02);
         lcd_lines[jjj+3] = '.';
         p_val(&lcd_lines[jjj+4],msec/10,0x40 + 0x01);
         
         }
      else if ( fmt[iii] == 'h' )  
         {
         kkk=0;
         lll = cdb[off[iii]+0];
         while((kkk<lll) && (kkk<16)) lcd_lines[jjj+kkk++] = 6;    // full block
         lll = cdb[off[iii]+1];
         while((kkk<lll) && (kkk<16)) lcd_lines[jjj+kkk++] = 7;    // half block
         lll = cdb[off[iii]+2];
         while((kkk<lll) && (kkk<16)) lcd_lines[jjj+kkk++] = '_';  // quart block
         }
      else if ( (fmt[iii] == 't') )
         {
         mmm = lll%3600;
         p_val(&lcd_lines[jjj+0],lll/3600,0x10 + 0x01);
         lcd_lines[jjj+2] = 'h';
         p_val(&lcd_lines[jjj+3],mmm/60  ,0x50 + 0x01);
         lcd_lines[jjj+5] = 'm';
         }
      else if ( (fmt[iii] == 'T') )  
         {
         lll = lll/1000;
         mmm = lll%3600;
         p_val(&lcd_lines[jjj+0],lll/3600,0x10 + 0x01);
         lcd_lines[jjj+2] = 'h';
         p_val(&lcd_lines[jjj+3],mmm/60  ,0x50 + 0x01);
         lcd_lines[jjj+5] = 'm';
         p_val(&lcd_lines[jjj+6],mmm%60  ,0x50 + 0x01);
         lcd_lines[jjj+8] = 's';
         }
      }
///////////////////////////// Edit Logic  /////////////////////////////
   if ( (l_edt != c_edt) && ( c_edt != -1) && (l_edt == -1) )   // Entering Edit mode, remember initial value and CDB offset
      {
      cdb_initial_value   = cdb[off[0]];  
      cdb_initial_offset  = off[0];  
      stack_before_edit   = menu_stack_ptr;
      } 
   l_edt = c_edt;

   if ( TT < 3 ) rs232_edit = c_edt;   // for cursor display
   else          rs232_edit=-1;        // for cursor display
///////////////////////////// Mozaic Startup Logic  /////////////////////////////
   if ( (d_state == M_MOZAIC_FIRST) ||
        (d_state == M_MOZAIC_PAUSE_FIRST) )  // in case we use the long push on the UNDO button, these states acts as reset
      {
      mozaic_state = 0;      // nothing to do...
      cdb[5] = 0;  // shot no
      cdb[6] = 82; // Total # shot
      cdb[7] = 0;  // RA id
      cdb[8] = 0;  // DE id;
      cdb[16] = 0; // Mozaic Running time in ms ( 1000-2000:focus    2000-> cdb[3]*1000: expose    then wait 4 second to store )
//      cdb[41] = cdb[17] = cdb[1];  // Save start RA      (base position for mozaic)
//      cdb[47] = cdb[18] = cdb[2];  // Save start DEC     (base position for mozaic)
      m_minor = m_major = 0;

      if ( d_state == M_MOZAIC_FIRST )       { mozaic_mode = 1;  // GOTO mode  -> do a goto for before each picture and do a 9x9 matrix
                                               cdb[6] = 82; // Total # shot
                                             }
      if ( d_state == M_MOZAIC_PAUSE_FIRST ) { mozaic_mode = 2;  // Pause mode -> do a goto only after a line on the RA is complete, and on RA, each picture is separated by a slew (no goto)
                                               cdb[6] = cdb[28] * cdb[29] + 1;
                                             }
      if ( d_state == M_MOZAIC_PAUSE_FIRST )   m_major = cdb[28]; // because 0 and 1 divided by 2 = 0, I set major to the last, result: first pic is center, second is 
      }

///////////////////////////// Mozaic Logic (pause) /////////////////////////////
   cdb[30] = mozaic_state + 0x1000*d_state; // DEBUG D1
   cdb[31] = (cdb[11] == 0) || (cdb[11] == '0');
   if ( ((d_state >= M__MOZ_PAUSE_RUN_FIRST) && (d_state <= M__MOZ_PAUSE_RUN_LAST)) || (d_state == M_PAUSE_CANCEL_FIRST) || (d_state == M_PAUSE_CANCELING_FIRST))
      {
//    cdb[31] = mozaic_wait; // DEBUG D2
      if      ( mozaic_state == 1 ) // command requested 
         { if ( mozaic_wait  == 0 ) mozaic_state++; }  // wait...but not forever
      else if ( mozaic_state == 2 ) // waiting for first "in goto" status
         { if ( mozaic_wait  == 0 ) mozaic_state++; }  // wait...but not forever
      else if ( mozaic_state == 3 ) // command send and "in goto" state feedback received
         {
         if ( (cdb[11] == 0) || (cdb[11] == '0') ) // if not in goto
            {
            if ( ((d_state==M_PAUSE_CANCELING_FIRST)|| (m_minor>cdb[29])) && (cdb[7]==0) && (cdb[8]==0) ) // cancel Mozaic or finished  goto 0,0 complete
               {
               set_tracking_mode = 2;  // screw that, hardcode North EQ
               if ( d_state==M_PAUSE_CANCELING_FIRST) pop(-3);
               else                                   pop(-1);
               refresh = 1;        // force a first pass
               beep_time_ms = 5000;
               }
            else
               {
               mozaic_state=0;
               cdb[16] = 0;   // tell foreground to start taking the picture
               }
            }
         }
      else if ( mozaic_state == 10 )  // Wait for pause to complete
         {
//beep_time_ms = 10;
         if ( cdb[24] <= 0 ) 
            {
            cdb[16] = 0;       // tell foreground to start taking the picture
            mozaic_state = 0;  // we are done waiting
//          set_tracking_mode = tracking_mode_p;  // Restore tracking mode
            set_tracking_mode = 2;  // screw that, hardcode North EQ
            }
         }
      else   // the above is exact same as normal goto mozaic and it simply asks for a goto and wait for it to complete
         {
         cdb[32] = d_state; // DEBUG D3
         cdb[33] = M_PAUSE_CANCEL_FIRST; // DEBUG D4
         if ( d_state==M_PAUSE_CANCEL_FIRST ) {;}   // use cancel to pause the Mozaic
         else if ( cdb[16] >= cdb[3]+SEQ_3 ) // Picture taken, ready to proceed...
            {
            cdb[5] ++;                              // Shot #
            m_major ++ ;
            if ( m_major >= cdb[28]  )              // RA axis
               {
               m_major = 0;
               m_minor ++;

               cdb[7] = -(cdb[28]/2) ;   // on RA Axis  always go to right most position
               cdb[8] = m_minor/2;      // on DEC Axiz
               if ( m_minor & 1 )  cdb[8] = -cdb[8];  // alternate each sides

//cdb[7] = cdb[7]*10; // debug
//cdb[8] = cdb[8]*10; // debug

               mozaic_state = 1;  // Ask the CGEM communication to issue a goto command
               mozaic_wait  = 2500; // wait 15 sec max 
               }
            else
               {
               mozaic_state = 10; // State where we wait x ms
                                              // cdb[15] = total span for cdb[28] pictures on RA axis
                                              // ONE_TENTH_OF_NEWTON_FOV = 1193046   we know a full frame on newton is 480sec, so 1/20 is 24 seconds
                                              // ONE_TENTH_OF_NEWTON_FOV / 1000 / 24  = 51
//             cdb[24] = cdb[15]/cdb[28]/51;  // Time in miliseconds for one picture  ... so with this, if we take 100 picture, the span will always be 1/20 of the image
               cdb[24] = cdb[15]/510;         // Time in miliseconds for one picture  assuming 10 pictures ... so with this, the delta between each picture will be constant, and probably easyer to post-process
//cdb[24] = cdb[15]/51;         // Debug, wait about 30 sec each pic
               tracking_mode_p = cdb[22];     // remember active tracking mode
               set_tracking_mode = 0;         // request tracking off
               } 

            if ( (d_state==M_PAUSE_CANCELING_FIRST)  || ( m_minor > cdb[29] ) ) // cancel Mozaic  or Finished
               {
               cdb[7] = 0;
               cdb[8] = 0;
               mozaic_state =1; // Ask the CGEM communication to issue a goto command
               mozaic_wait  =150; // wait 15 sec max 
               }
            }
         }
      }
///////////////////////////// Mozaic Logic (goto) /////////////////////////////
   if ( ((d_state >= M__MOZ_RUN_FIRST) && (d_state <= M__MOZ_RUN_LAST)) || (d_state == M_CANCEL_FIRST) || (d_state == M_CANCELING_FIRST))
      {
      if      ( mozaic_state == 1 ) // command requested 
         { if ( mozaic_wait  == 0 ) mozaic_state++; }  // wait...but not forever
      else if ( mozaic_state == 2 ) // waiting for first "in goto" status
         { if ( mozaic_wait  == 0 ) mozaic_state++; }  // wait...but not forever
      else if ( mozaic_state == 3 ) // command send and "in goto" state feedback received
         {
         if ( (d_state==M_CANCELING_FIRST) && (cdb[7]==0) && (cdb[8]==0) ) // cancel Mozaic and goto 0,0 complete
            {
            pop(-3);
            refresh = 1;        // force a first pass
            }
         else if ( (cdb[11] == 0) || (cdb[11] == '0') ) // if not in goto
            {
            mozaic_state=0;
            cdb[16] = 0;   // tell foreground to start taking the picture
            }
         }
      else if ( cdb[16] >= cdb[3]+SEQ_3 ) // Picture taken, ready to proceed... 
         {       // 16: Mozaic Running time in ms ( 1000-2000:focus    2000-> cdb[3]*1000: expose    then wait 4 second to store )
         if ( cdb[5] == cdb[6] )   // finished, too all the pictures
            {
            pop(-1);
            refresh = 1;     // force a first pass
            beep_time_ms = 5000;
            }
         else                                     // Take new picture
            {
            cdb[5] ++;
            m_major ++ ;
            if ( m_major >=9 ) 
               { 
               m_major = 0; 
               m_minor ++;
               if ( m_minor >=9 ) m_minor=0;
               }
            cdb[7] = (signed char)pgm_read_byte( &m_seq_x[m_minor]) + 3 * (signed char)pgm_read_byte( &m_seq_x[m_major]);  
            cdb[8] = (signed char)pgm_read_byte( &m_seq_y[m_minor]) + 3 * (signed char)pgm_read_byte( &m_seq_y[m_major]);  

            if ( d_state==M_CANCELING_FIRST ) // cancel Mozaic 
               {
               cdb[7] = 0;
               cdb[8] = 0;
               }
            mozaic_state=1; // Ask the CGEM communication to issue a goto command
            mozaic_wait=150; // wait 15 sec max 
            }
         }
      }
///////////////////////////// Timelap Logic  /////////////////////////////
   if ( ((d_state >= M__TIMELAP_RUN_FIRST) && (d_state <= M__TIMELAP_RUN_LAST)) || (d_state == M_CANCEL_FIRST) || (d_state == M_CANCELING_FIRST))
      {
      if ( (cdb[16] >= cdb[3]+SEQ_3) && (timelap_state==0) ) // Picture taken, ready to proceed...
         {
         cdb[26] +=1000;   // shot counter
         timelap_state = 1; // wait for period to complete
         }
      if ( (timelap_state == 1) && (cdb[27]>=(cdb[4]*1000)))
         {
         cdb[27]  = (SEQ_3-SEQ_2);  // Period count                                            // little hack to get back the extra time from the mozaic sequence
         cdb[16] = 0;   // tell foreground to start taking the picture                   // little hack to get back the extra time from the mozaic sequence
         timelap_state = 0; 
         }
      }
   else
      {
      cdb[26]  = 1000;  // Shot x of
      cdb[27]  = 0;  // Period count
      timelap_state = 0 ;
      }
///////////////////////////// CGEM communication /////////////////////////////
   if ( 0x01 == (rs232_com_state & 0x01) )   // we are waiting for a response
      {
      if ( rs232_com_to > 3000) // outch... 3 seconds without a response...
         {
         cdb[9]++;   // Time-out
         rs232_rx_ptr    = 0;
         rs232_com_state = 0;   // restart
         }
      else if ( rs232_rx_ptr < 0 )  // something to do... we just got a complete message
         {
         if ( rs232_com_state == 1 ) // response from aligned {J} or Tracking {t}
            {
//          cdb[34] = 0x1000+cdb[22];   // DEBUG D5
//          cdb[35] = 0x2000+set_tracking_mode; // DEBUG D6
            if ( rs232_rx_cmd[1]=='#' ) 
               {
               if ( cgem_fb == 'J' ) cdb[10] = rs232_rx_cmd[0];
               if ( cgem_fb == 't' ) cdb[22] = rs232_rx_cmd[0];
               if ( set_tracking_mode == cdb[22] ) set_tracking_mode=-1;  // request completed
               }
            }
         else if ( rs232_com_state == 3 ) // response from in goto  {L}
            {
            if ( rs232_rx_cmd[1]=='#' ) 
               {
               cdb[11] = rs232_rx_cmd[0];
               if ( mozaic_state==2 ) mozaic_state++; // after any goto command, this is the first valid feedback, tell the mozaic logic that it can wait for goto finished...
               }
            }
         else if ( rs232_com_state == 5 ) // response from get pos {e}
            {
            long          AAA,BBB;     // {e} : C691D800,1B947B00#
            unsigned char aaa,bbb;     //       01234567890123456789
            aaa = rs232_rx_hex(0,8,&AAA);
            bbb = rs232_rx_hex(9,8,&BBB);
            if ( (rs232_rx_cmd[8]==',') && (rs232_rx_cmd[17]=='#') && (aaa == 0) && (bbb == 0))
               {
               cdb[1]  = AAA; 
               cdb[2]  = BBB; 
               cdb[14] = 0;   // reset update time
               l_TT = -1;  // Send goto right away if in mozaic mode
               }
            else cdb[13]++;   // get position error
            }
         else if ( rs232_com_state == 7 ) // response from  goto command (mozaic)  {r}
            {
            if ( rs232_rx_cmd[0]=='#' ) cdb[12]++;   // goto error
            if ( (mozaic_state==1) && (cdb[11]=='1') ) mozaic_state=2;   // Got a resopnse from the goto request, plus we are moving ...
            }

         rs232_com_state++; // go to next state
         if ( rs232_com_state == 8 ) rs232_com_state = 0;

         for ( iii=0 ; iii<sizeof(rs232_rx_cmd) ; iii++ ) rs232_rx_cmd[iii]=0; // clear the buffer
         rs232_rx_ptr = 0;  // we are done processing the message, give back the control to foreground
         }
      }
   if ( 0x00 == (rs232_com_state & 0x01) )   // we are ready to send a command
      {
      if ( (l_TT != TT) && ( rs232_tx_ptr < 0 ))  // 10 times a second, send a new command  (as long as the last transmit is complete
         {
         rs232_tx_cnt = 1;    // most commands below are 1 character long

         l_TT = TT;
         for ( kkk=0 ; kkk<sizeof(rs232_tx_cmd) ; kkk++ ) rs232_tx_cmd[kkk]=0; // clear the buffer

         rs232_com_to = 0;   // reset the time-out monitor
         if ( rs232_com_state == 0 ) // send: aligned {J}
            {  // Sequence: t,J,T : get tracking mode, alignment complete ? , set tracking mode
            if ( set_tracking_mode >= 0  ) // when a new request is made, alternate Request and Status
               {
               if ( cgem_fb == 'T' )      {  cgem_fb = 't';  // t : get tracking mode
                                          }
               else                       {  cgem_fb = 'T';  // T : set tracking mode
                                             rs232_tx_cmd[1] = (set_tracking_mode & 0x3);
                                             rs232_tx_cnt = 2;
                                          }
               }
            else if ( cgem_fb == 'J' )       cgem_fb = 't';  // t : get tracking mode
            else                             cgem_fb = 'J';  // J : is align complete ?

/* This works and forces the controler in Eq-North
            else        
               {
               cgem_fb = 'T';
               rs232_tx_cmd[1] = 2; // try to set the tracking mode...
               }   */
            rs232_tx_cmd[0]=cgem_fb;  
            }
         else if ( rs232_com_state == 2 ) // Send: in goto  {L}
            {
            rs232_tx_cmd[0]='L';  
            }
         else if ( rs232_com_state == 4 ) // Send:  get pos {e}
            {
            rs232_tx_cmd[0]='e';  
            }
         else if ( rs232_com_state == 6 ) // Send:  goto pos {rC691D800,1B947B00}
            {                             //                  01234567890123456789
            if ( mozaic_state==1 ) // goto request 
               {
               if ( mozaic_mode == 2 )   // pause method not always 9x9
                  {
                  rs232_tx_cmd[0]='r';  
//                lll = (cdb[15]*cdb[7])*6  + cdb[17];  // (Span * displacement)*2 + Start RA    when tracking off (inhouse tests) multiply the span by 18
                  lll = (cdb[15]*cdb[7])/9  + cdb[17];  // (Span * displacement)/9 + Start RA
                  p08x(&rs232_tx_cmd[1],&lll);
                  rs232_tx_cmd[9]=',';  
//                lll = (cdb[15]*cdb[8])*6  + cdb[18];  // (Span * displacement)*2 + Start DEC    when tracking off (inhouse tests) multiply the span by 18
                  lll = (cdb[15]*cdb[8])/9  + cdb[18];  // (Span * displacement)/9 + Start DEC
                  p08x(&rs232_tx_cmd[10],&lll);
                  }
               else
                  {
                  rs232_tx_cmd[0]='r';  
                  lll = (cdb[15]*cdb[7])/cdb[28]  + cdb[17];  // (Span * displacement)/(nb pic on RA axis) + Start RA
                  p08x(&rs232_tx_cmd[1],&lll);
                  rs232_tx_cmd[9]=',';  
                  lll = (cdb[15]*cdb[8])/cdb[29]  + cdb[18];  // (Span * displacement)/(nb pic on DE axis) + Start DEC
                  p08x(&rs232_tx_cmd[10],&lll);
                  }
               rs232_tx_cnt = 18;
               beep_time_ms = 100;
               }
            else rs232_com_state = -1;   // Jumpt to state 0
            }
         if ( rs232_tx_cmd[0] ) rs232_tx_ptr    = 0;     // FG... pls tx the command  (unless there is no commands)
         rs232_com_state++;       // wait response
         }
      } // if ( rs232_com_state & 0x01 )
// ok:                   LeJLetLerE1333F54,1DEEA400JLerE1333F54,1DEEA400tLeJLetLeJLeA
// bad: eTLeTLeTLeTLeTLeTLeTLeTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CACTLerE1333F54,1E130CA
///////////////////////////////////////////////////////////////////////////////////////
   } // while(1)
}

// #include "a328p_lcd.c"   <-- I need the makefile to compile with -DUSE_LCD
//
//
