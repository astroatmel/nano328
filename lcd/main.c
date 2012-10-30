
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "a328p_lcd.h"

#include "a328p_rt.h"

unsigned long d_ram;
unsigned char lcd_go_rt=0;
unsigned short beep_time_ms;

char rs232_tx_cmd[32];  // null terminated
char rs232_rx_cmd[32];  
char rs232_tx_ptr = -1;   // -1 means last tx complete and >0 means in progress of transmitting...
char rs232_rx_ptr = 0;   // -1 means a command was received, -2 means overflow
short         rs232_com_to    = -1;  // Time-out
char rs232_com_state = 0;  // Odd values are states where we are waiting for an answer from CGEM
// States are:
// 0: Is aligned
// 2: Is in goto 
// 4: get current pos
// 6: if in mozaic mode and aligned, and the time is right, do a goto
                      

volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;


#define CDB_SIZE 15
volatile long cdb[CDB_SIZE];
//  0: Runnig time in seconds
//  1: RA from CGEM
//  2: DE from CGEN 
//  3: Exposure time in mili-second
//  4: Delta time in mili-second between shoots
//  5: Shot #        total: 81 on a 9x9 grid
//  6: Shot RA  id   9x9 grid
//  7: Shot DEC id   9x9 grid
//  8: Total # shot
//  9: Communication Time-Out RS232 (no response from CGEM)
// 10: Aligned ?
// 11: In goto ?
// 12: Mozaic goto error
// 13: get position error {e}
// 14: Time since last position update

//   "W132\00145\00245.32\003 \006"  //  Example of custom characters 
PROGMEM const char linetxt[]="                "        //  0: 
                             "                "        //  1: 
                             "Version 1.0     "        //  2: 
                             "RA:\242R\001          "  //  3: @R will display "?XXX:XX:XX.XX"   Deg/Min/Sec  ? is E/W
                             "RA:\242H\001          "  //  4: @H will display "XXhXXmXX.XXs"   Hour/Min/Sec  
                             "DE:\242D\002          "  //  5: @D will display "?XXX:XX:XX.XX"   Deg/Min/Sec  ? is N/S
                             "Mozaic Menu...  "        //  6: 
                             "Timelap Menu... "        //  7:
                             "Total Span:     "        //  8:    Mozaic span in deg:min:sec
                             "Exposure Time:  "        //  9:    
                             "Delta Time:     "        // 10:
                             "Current:\242s\000 sec "  // 11: @s will display "XXX Sec"
                             "    Set:\243s\000 sec "  // 12:
                             "Cur:\242h\000         "  // 13: @h will display "XXhXXm"
                             "Set:\243h\000         "  // 14:
                             "Cur:\242d\000         "  // 15: @d will display "XXX:XX:XX.XX"  Deg/Min/Sec
                             "Set:\243d\000         "  // 16:
                             "Start Mozaic... "        // 17:
                             "Start Timelaps.."        // 18:
                             "Really Cancel ? "        // 19:
                             "Mozaic: \242o\000     "  // 20: @o will display "xx of yy" and use two consecutive CDB location
                             "Pos Id: \242p\000     "  // 21: @p will display xx,yy  and use two consecutive CDB location
                             "Motor Tests...  "        // 22:
                             "Step up/down    "        // 23: // use up/down to control up/down
                             "Ramp up/down    "        // 24: // use up/dowm to control the rate
                             "Use Up/Down     "        // 25: 
                             "To Step         "        // 26:
                             "To change rate  "        // 27:

                             "Camera Driver   "  //  always last to make sure all are 16 bytes wide... the \xxx makes it a bit difficult
;

//                                              M  T  S  .  M  M  M  M  M  M  M  M  S  S  S
// States:                             0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20
PROGMEM const unsigned char line1[] = {28,3 ,4 ,6 ,7 ,22,0 ,9 ,9 ,8 ,8 ,17,20,19,0 ,23,25,24,25,0 ,0  };   // what string to display on line 1 for each states
PROGMEM const unsigned char line2[] = {2 ,5 ,5 ,0 ,0 ,0 ,0 ,11,12,11,12,0 ,21,0 ,0 ,0 ,26,0 ,27,0 ,0  };   // what string to display on line 2 for each states

short d_state;   // main state machine that controls what to display
#define BT_LONG  1000            // a long push is 1 sec (1000 ticks)
volatile unsigned char  button,button_sp,button_lp;   // Current button and real-time sets the short push and long push
unsigned char  button_last;   // Prevous button

PROGMEM const char main_state_machine[]= {0,// ENTER  UNDO   UP     DOWN             // -1 means no action    -2 means edit ?
                                               1     ,1     ,1     ,1                // State:  0  welcome message, any key to go to state 1
                                              ,-1    ,-1    ,5     ,2                // State:  1  Show current telescope position {e} 
                                              ,-1    ,-1    ,1     ,3                // State:  2  Show current telescope position {e} in hour min sec
                                              ,7     ,-1    ,2     ,4                // State:  3  Mozaic Menu
                                              ,-1    ,-1    ,3     ,5                // State:  4  Timelaps pictures Menu
                                              ,15    ,-1    ,4     ,1                // State:  5  Motor Test
                                              ,-1    ,-1    ,-1    ,-1               // State:  6  Spare
                                              ,8     ,3     ,11    ,9                // State:  7  Mozaic > Current Exposure Time  
                                              ,7     ,7     ,-2    ,-2               // State:  8  Mozaic > Set Exposure Time  
                                              ,10    ,3     ,7     ,11               // State:  9  Mozaic > Current Total span     
                                              ,9     ,9     ,-2    ,-2               // State: 10  Mozaic > Set Total span     
                                              ,12    ,3     ,9     ,7                // State: 11  Mozaic > Start Mosaic   
                                              ,-1    ,13    ,-1    ,-1               // State: 12  Mozaic > Mozaic in progress              (undo to cancel)
                                              ,11    ,12    ,12    ,12               // State: 13  Mozaic >>  Sure you want to cancel ?       (enter to confirm)
                                              ,-1    ,-1    ,-1    ,-1               // State: 14  Mozaic > spare
                                              ,16    ,5     ,17    ,17               // State: 15  Motor  > Step Up/Down 
                                              ,-1    ,15    ,-1    ,-1               // State: 16  Motor  > Use Up/Down to Step...
                                              ,18    ,5     ,15    ,15               // State: 17  Motor  > Ramp up/down
                                              ,-1    ,17    ,-1    ,-1               // State: 18  Motor  > Use Up/Down to Change rate...
                                              ,-1    ,-1    ,-1    ,-1               // State: __  
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
void p_val(unsigned char *buf,short val,unsigned char option_digit)
{
unsigned char nnn;
unsigned char digit = 1 + (option_digit & 0x07);  // 3 lsb = nb digits
unsigned char sign = (option_digit & 0x30);  // this bit means...  0x00: dont show +/-  0x10: showw only "-" if negative   0x30: show +/-
unsigned char lead = (option_digit & 0x40);  // this bit means we want to display leaging zeros
short iii;
long  max;
nnn = val < 0;
if ( sign ) max = 1;
else        max = 10; // if not displaying signs, then we have one more character

for ( iii=2 ; iii<digit ; iii++ ) max=max*10;
if(val<=-max)   // 10000000000  (10^digit)-1
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '-';   // below max negative displayable
   }
else if (val>=max*10)
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '+';   // above max positive displayable
   }
else
   {
   if ( nnn ) val = -val;
   for ( iii=digit-1 ; val>0 ; val /= 10 ) buf[iii--] = val%10 + '0';
   if ( iii < 0 ) return; // no space left
   if ( lead )
      {
      while (iii>0) buf[iii--] = '0';
      if ( sign == 0 ) buf[iii--] = '0';
      }
   if ( sign ) 
      {
      if ( nnn )               buf[iii--] = '-';
      else if ( sign == 0x30 ) buf[iii--] = '+';
      }
   while (iii>=0) buf[iii--] = ' ';
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

void p08x(unsigned char *bof,long value)
{
unsigned char *p_it = (unsigned char*) &value;
p_it = (unsigned char*) &value;
p02x(&bof[6],*p_it++);
p02x(&bof[4],*p_it++);
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
}

void p04x(unsigned char *bof,long value)
{
unsigned char *p_it = (unsigned char*) &value;
p_it = (unsigned char*) &value;
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

volatile unsigned short msms;
volatile unsigned char TT,SS,MM,HH;

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
static unsigned short button_down_ms;   // check how long a button is held down

rt_TIMER1++;

if ( lcd_go_rt ) lcd_rt_print_next();
if ( beep_time_ms )
   {
   OCR2B = 10;
   OCR2A = 20;
   if ( 0 == --beep_time_ms ) OCR2B = 254;  // stop the beep
   }
if ( button ) 
   {
   if (button<=BT_LONG) button_down_ms++; // check how long the button is down;
   if      ( button_down_ms == 10 )       { beep_time_ms = 10; button_sp = button; }    // Beep
   else if ( button_down_ms == BT_LONG )  { beep_time_ms = 10; button_lp = button; }    // Beep high pitch on long push
   }
else button_down_ms=0;

msms++;
if ( msms >= 100 ) 
   { 
   msms = 0;
   TT++;                              // Tenths
   if ( TT >= 10 )
      {
      SS ++;                          // Seconds
      cdb[0]++; // running time
      if ( SS >= 60 )
         {
         SS = 0;
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
      if ( rs232_tx_cmd[(unsigned char)rs232_tx_ptr] == 0 ) rs232_tx_ptr = -1;  // we are done
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

// TODO: put an histogram...
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
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
int main(void)
{
unsigned char iii,jjj,nb_display=0;
unsigned char l_TT=0;
long lll;
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

d_state=0;
lcd_go_rt = 1;

while(1)
   {
   static unsigned char id=255;
   static unsigned char set[10],fmt[10],pos[10],off[10]; // show/edit format position cdb-offset    <- values used to set/show values on LCD
   short i1,i2;

   if      (LCD_BUTTON_1 == 0 ) {button = BT_ENTER; }
   else if (LCD_BUTTON_2 == 0 ) {button = BT_UP;    }
   else if (LCD_BUTTON_3 == 0 ) {button = BT_UNDO;  }
   else if (LCD_BUTTON_4 == 0 ) {button = BT_DOWN;  }
   else                         {button = button_lp = button_sp = 0; } // _sp gets set when button is at least 10ms down

   if ( (button_sp != button_last) || (id==255))   // is=255 used as a first pass
      {
      if ( button_sp == 0 ) //  action on key-off
         {               // Check next state...
         unsigned char next = pgm_read_byte( &main_state_machine[d_state*4 + button_last]);
         // if ( button_lp ) if long push, then different meaning
         if ( next< sizeof(line1) ) d_state = next;
         }

      button_last = button_sp;
      i1 = pgm_read_byte(&line1[d_state]) * 16;
      i2 = pgm_read_byte(&line2[d_state]) * 16;
      for ( id=0;id<16;id++ ) 
         {
         lcd_lines[id+0x00] = pgm_read_byte ( &linetxt[i1]+id);
         lcd_lines[id+0x10] = pgm_read_byte ( &linetxt[i2]+id);
         } 
      nb_display = 0; // assume nothing to do
      for ( id=0;id<32;id++ ) // check for printable / editable strings
         {
         if ( (lcd_lines[id] == 0xA2) || (lcd_lines[id] == 0xA3) )    // we have something to do...
            {
            set[nb_display] = lcd_lines[id]==0xA2;   // show ?
            fmt[nb_display] = lcd_lines[id+1]; // format
            off[nb_display] = lcd_lines[id+2]; // CDB label offset
            pos[nb_display] = id;
            nb_display++;
            }
         } 

      if ( d_state==16 )  // Process button inputs for in Motor Step up/down mode
         {
         if (button == BT_UP)   CANON_FOCUS_HIGH;  
         if (button == BT_DOWN) CANON_FOCUS_LOW;   // Led on
         rt_wait_ms(1);
         if ((button == BT_UP) || (button == BT_DOWN))   
            {
            CANON_SHOOT_HIGH;
            rt_wait_ms(100);
            }
         CANON_SHOOT_LOW;
         }
      } 
   if ( button_lp && ( d_state==16 ) )  // Motor Tests
      {
            CANON_SHOOT_HIGH;
            rt_wait_ms(100);
            CANON_SHOOT_LOW;
            rt_wait_ms(50);
      }

   for ( iii=0 ; iii < nb_display ; iii++ )  // display required values
      {
      lll = cdb[off[iii]];
      jjj = pos[iii];
      if ( fmt[iii] == 's' )  // @sX format  (seconds)
         {
         p_val(&lcd_lines[jjj],lll,0x00 + 0x02);
         }
      else if ( fmt[iii] == 'R' )  // @RX format  (RA)
         {
         p08x(&lcd_lines[jjj],lll);
         }
      else if ( fmt[iii] == 'D' )  // @DX format  (DEC)
         {
         p08x(&lcd_lines[jjj],lll);
         }
      else if ( fmt[iii] == 'H' )  // @HX format  (Hour)
         {
         p08x(&lcd_lines[jjj],lll);
         }
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
         if ( rs232_com_state == 1 ) // response from aligned {J}
            {
            if ( rs232_rx_cmd[1]=='#' ) cdb[10] = rs232_rx_cmd[0];
            }
         else if ( rs232_com_state == 3 ) // response from in goto  {L}
            {
            if ( rs232_rx_cmd[1]=='#' ) cdb[11] = rs232_rx_cmd[0];
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
         l_TT = TT;
         for ( iii=0 ; iii<sizeof(rs232_tx_cmd) ; iii++ ) rs232_tx_cmd[iii]=0; // clear the buffer

         rs232_com_to = 0;   // reset the time-out monitor
         if ( rs232_com_state == 0 ) // send: aligned {J}
            {
            rs232_tx_cmd[0]='J';  
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
            {
            // if aligned and not in goto mode
            //rs232_tx_cmd[0]='r';  
            rs232_com_state = -1;   // Jumpt to state 0
            }
         rs232_tx_ptr    = 0;     // FG... pls tx the command
         rs232_com_state++;       // wait response
            


//         if ( cdb[1] != 0 )  // got something: send it reversed...
//            {
//            rs232_tx_cmd[0] = 'r';
//            rs232_tx_cmd[1] = 'x';
//            rs232_tx_cmd[2] = ':';
//            p08x(&rs232_tx_cmd[3],cdb[2]); 
//            rs232_tx_cmd[11] = '~';
//            p08x(&rs232_tx_cmd[12],cdb[1]); 
//            rs232_tx_cmd[20] = 0;
//            rs232_tx_ptr    = 0;     // FG... pls tx the command
//            cdb[1] = 0;
//            }
//         else if ( rs232_tx_ptr < 0 )
//            {
//            rs232_tx_cmd[0] = 'e';   // temp...find better way to copy strings
//            rs232_tx_cmd[1] = 0;
//            rs232_tx_ptr    = 0;     // FG... pls tx the command
//            }
         }
      } // if ( rs232_com_state & 0x01 )
   } // while(1)
}

// #include "a328p_lcd.c"   <-- I need the makefile to compile with -DUSE_LCD
//
//
