/*

simple simple program that reads an analog input and drives an output in pwm at 1Khz 

The purpose is to drive the telescope heater in PWM


////////// HEATER MODE \\\\\\\\\\\ A328 Pins used:
28 Analog input  
16 1KHz PWM Out
11 1Hz PWM Out
12 1Hz PWM Out

////////// CAMERA MODE \\\\\\\\\\\ A328 Pins used:
26 SELECT BUTTON, after reset, press to select mode, press 2 seconds to start taking pictures
16 1KHz PWM Out
17 DOP Camera Driver
5-6-9-10-11-12-13-14   Seven Segment driver 

   PROGMEM const char pololu[]={"\
A328p DIP:                                                        \012\015\
(PCINT14/RESET)      PC6   1 |    | 28  PC5 (ADC5/SCL/PCINT13)    \012\015\
(PCINT16/RXD)        PD0   2 |    | 27  PC4 (ADC4/SDA/PCINT12)    \012\015\
(PCINT17/TXD)        PD1   3 |    | 26  PC3 (ADC3/PCINT11)        \012\015\
(PCINT18/INT0)       PD2   4 |    | 25  PC2 (ADC2/PCINT10)        \012\015\
(PCINT19/OC2B/INT1)  PD3   5 |    | 24  PC1 (ADC1/PCINT9)         \012\015\
(PCINT20/XCK/T0)     PD4   6 |    | 23  PC0 (ADC0/PCINT8)         \012\015\
                     VCC   7 |    | 22  GND                       \012\015\
                     GND   8 |    | 21  AREF                      \012\015\
(PCINT6/XTAL1/TOSC1) PB6   9 |    | 20  AVCC                      \012\015\
(PCINT7/XTAL2/TOSC2) PB7  10 |    | 19  PB5 (SCK/PCINT5)          \012\015\
(PCINT21/OC0B/T1)    PD5  11 |    | 18  PB4 (MISO/PCINT4)         \012\015\
(PCINT22/OC0A/AIN0)  PD6  12 |    | 17  PB3 (MOSI/OC2A/PCINT3)    \012\015\
(PCINT23/AIN1)       PD7  13 |    | 16  PB2 (SS/OC1B/PCINT2)      \012\015\
(PCINT0/CLKO/ICP1)   PB0  14 |    | 15  PB1 (OC1A/PCINT1)         \012\015\
"};

*/

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define  F_CKDIV8  

#ifdef   F_CKDIV8
   // if the CKDIV8 is active, then our clock is 1MHz   (see fuses setup)
   #define  RT_CPU_K_FRAME          1000
#else
   #define  RT_CPU_K_FRAME          8000
#endif

//#define  PWM_HEATER 1
#define  CAMERA_DRIVER 1

#define LONG_PUSH 1000

   void rt_init_disp(void)  // 1Khz
   {
   TIMSK1 |= 1 <<  TOIE1;    // timer1 interrupt when Overflow                    ///////////////// SP0C0
   // dont drive the pins TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
   TCCR1A  = 0x23;           // FAST PWM, Clear OC1B on counter match, SET on BOTTOM
// TCCR1A  = 0x83;           // FAST PWM, Clear OC1A on counter match, SET on BOTTOM
// TCCR1A  = 0x03;           // FAST PWM, 
   TCCR1B  = 0x19;           // Clock divider set to 1 , FAST PWM, TOP = OCR1A
   OCR1A   = RT_CPU_K_FRAME/1;      // Clock divider set to 1, so RT_CPU_K_FRAME/1 is CLK / 1000 /1 thus, every 1000 there will be an interrupt : 1Khz
   OCR1B   = RT_CPU_K_FRAME/4;     // By default, set the PWM to 25%
   }


volatile short desired=0;
volatile short indicator=0;
volatile short CCC=0;
volatile short SSS=0;

volatile unsigned char show=0;
volatile unsigned char dot=0;
unsigned char PD[20]={0xA8,0x20,0x90,0xB0,0x38,0xB8,0xB8,0x20,0xB8,0x38,0x38,0xB8,0x88,0xB0,0x98,0x18,0x00,0x00,0x00,0x00};
unsigned char PB[20]={0xC1,0x80,0xC1,0xC0,0x80,0x40,0x41,0xC0,0xC1,0xC0,0xC1,0x01,0x41,0x81,0x41,0x41,0x00,0x00,0x00,0x00};
//                       0    1    2    3    4    5    6    7    8    9    A    b    C    d    E    F   16   17   18   19

volatile unsigned char  time_cnt[2]  = {5,7};  // max index to the below values
volatile unsigned short time_on[]    = {3,2,1,6,5,4}; // how many 10 seconds on  (3 = 30 sectonds)
volatile unsigned short time_off[]   = {0,1,2,3,4,5,10,11}; // how many minutes off A=10min B=20min
volatile unsigned char  time_idx[2]  = {0,0};  // index to the above values
volatile unsigned char  showing      = 0;      // are we showing Time On or Time Off 

volatile unsigned short select_di[2] = {8,4};  // [1] is index of button near header, [0] is the other
volatile unsigned short select_on[2] = {0,0};  // select button count, if more than 100ms, then change number
volatile unsigned short timeout      = 0; // dont show number for more than 3 seconds
volatile unsigned char  running      = 0; // if 1, then we take pictures...
volatile unsigned char  sequence     = 0; // 1/9, take a short exposure
volatile unsigned char  exposing     = 0; 
volatile unsigned long  wait         = 0; // time until next event
volatile unsigned char  camera       = 0; 

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 1 KHz
{
short iii;
CCC++;

if ( running )
   {
   if ( wait == 0 ) // time for next
      {
      CCC = 0; // restart a fresh second...
      exposing = !exposing;
      if ( exposing )
         {
         if ( sequence == 0  ) wait = 1 + time_on[time_idx[0]];
         else                  wait = 1 + time_on[time_idx[0]]*10; 
         sequence++;
         if ( sequence == 10 ) sequence=0;
         camera = 0x02;  // DOP BIT 
         }
      else
         {
         camera = 0x00;
         wait = time_off[time_idx[1]]*60;  
         if ( wait==0 ) wait = 5;          // minimum is 5 second wait
         } 
      }
   }
else camera = sequence = exposing = wait = 0;

#ifdef PWM_HEATER
OCR1B = desired;
if ( CCC < indicator) PORTD = 0xF8; 
else                  PORTD = 0x00; 
if ( CCC < indicator) PORTB = 0xC1 | camera; 
else                  PORTB = 0x00 | camera; 
#endif
#ifdef CAMERA_DRIVER
if ( timeout>0 ) timeout--;
if ( (CCC & 0x0F) == 0x0F)
   {
   if( running )
      {
      if ( (CCC & 0x40) && exposing ) PORTD |= 0x40;
      }
   else if ( timeout>0 ) 
      {
      if ( show < 20 ) { PORTD = PD[show];
                         PORTB = PB[show] | camera; }
      else             { PORTD = 0x00;
                         PORTB = 0x00 | camera;     }
      if (dot) PORTD |= 0x40;
      }
   }
else
   {
   PORTD = 0x00;
   PORTB = 0x00 | camera;     
   }
#endif

if ( CCC >= 1000 ) 
   {
   CCC = 0; // seconds
   SSS++;
   if ( wait > 0 ) wait--;
   // if ( show <17 ) show++;
   // if ( show <17 ) dot = !dot;
   // if ( show > 15 ) show = 0;
   }
//dot = (PINC & 0x04) !=0;  // button near the header connector
//dot = (PINC & 0x08) !=0;

//////// ON button /////////
for ( iii=0; iii<2 ; iii++)
   {
   if ( (PINC & select_di[iii]) ==0 ) select_on[iii]++;
   else 
      {
      if ( (select_on[iii] > 100) && (select_on[iii] < LONG_PUSH) && (running==0)) 
         {
         // dot = !dot;
         dot=iii;
         if ( (timeout !=0) && (showing == iii))
            {
            time_idx[iii]++;
            if ( time_idx[iii] > time_cnt[iii] ) time_idx[iii]=0;
            }
         showing = iii;
         timeout = 3000;
         if ( showing == 0 ) show = time_on[time_idx[showing]];
         if ( showing == 1 ) show = time_off[time_idx[showing]];
         }
      select_on[iii] = 0;
      }
   if ( select_on[iii] == LONG_PUSH ) 
      {
      running = !running;
      timeout = 3000;
      }
   }
}

////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
int main(void)
{
long adc;
short ccc=0,sss=0;

rt_init_disp();


// PRR   = 0x00; // Enable all systems
#ifdef PWM_HEATER
PRR   = 0xE6; // Enable only TIMER1 and ADC
DDRD |= 0x60;  // PD6 test
DDRB |= 0x04;  // PB2 PWM out
#endif

#ifdef CAMERA_DRIVER
PRR   = 0xE7;  // Enable only TIMER1
DDRD |= 0xF8;  // Seven segment
DDRB |= 0x04;  // PB2 PWM out
DDRB |= 0x02;  // PB1 Camera driver
DDRB |= 0xC1;  // Seven segment
PORTC |=0x0C;  // Activate Pull-ups
//      #if ( F_CKDIV == 256 )
//CLKPR = 0x80;  // change clock pre-scaler
//CLKPR = 0x06;  // change clock pre-scaler to /256
//      #endif
#endif

sei();         //enable global interrupts

ADCSRA = 0x84;  // enable 
while(1)
   {
   while ( CCC == ccc );
   ccc = CCC;

   ADMUX = 0x05;  // read from adc 5
   ADCSRA |= 0x40;  // start conversion...
   while ( (ADCSRA & 0x40) != 0 ) ; // wait for completion

// adc  = ADC*10 - 800; 
   #ifdef F_CKDIV8
      adc  = ADC - 80;      // approximation for 1MHz clock
   #else
      adc  = ADC*10 - 800;  // approximation for 8MHz clock
   #endif
   if ( adc > RT_CPU_K_FRAME ) desired = RT_CPU_K_FRAME;
   else if ( adc > 0 )         desired = adc;
   else                        desired = 0;

   if ( sss != SSS )
      {
      adc = desired;
      adc *= 1000;
      adc /= RT_CPU_K_FRAME;
      if ( adc < 1 ) adc = 1;
      indicator = adc;
      sss = SSS;
      }
   }

return 0;
}











