/*

simple simple program that reads an analog input and drives an output in pwm at 1Khz 

The purpose is to drive the telescope heater in PWM


A328 Pins used:
28 Analog input
16 PWM Out

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

#define  RT_CPU_K_FRAME          8000

   void rt_init_disp(void)  // 1Khz
   {
   TIMSK1 |= 1 <<  TOIE1;    // timer1 interrupt when Overflow                    ///////////////// SP0C0
   // dont drive the pins TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
   TCCR1A  = 0x23;           // FAST PWM, Clear OC1B on counter match, SET on BOTTOM
// TCCR1A  = 0x83;           // FAST PWM, Clear OC1A on counter match, SET on BOTTOM
   TCCR1B  = 0x19;           // Clock divider set to 1 , FAST PWM, TOP = OCR1A
   OCR1A   = RT_CPU_K_FRAME/1;      // Clock divider set to 1, so RT_CPU_K_FRAME/1 is CLK / 1000 /1 thus, every 1000 there will be an interrupt : 1Khz
   OCR1B   = RT_CPU_K_FRAME/4;     // By default, set the PWM to 25%
   }

volatile short desired=0;
volatile short indicator=0;
volatile short CCC=0;
volatile short SSS=0;

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 1 KHz
{
OCR1B = desired;
CCC++;
if ( CCC < indicator) PORTD = 0x60; 
else                  PORTD = 0x00; 

if ( CCC >= 1000 ) 
   {
   CCC =0; // seconds
   SSS++;
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

PRR   = 0; // Enable all systems

DDRD |= 0x60;  // PD6 test
DDRB |= 0x04;  // PD6 test


sei();         //enable global interrupts

ADCSRA = 0x84;  // enable 
while(1)
   {
   while ( CCC == ccc );
   ccc = CCC;

   ADMUX = 0x05;  // read from adc 5
   ADCSRA |= 0x40;  // start conversion...
   while ( (ADCSRA & 0x40) != 0 ) ; // wait for completion
   adc  = ADC*10 - 800; 
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











