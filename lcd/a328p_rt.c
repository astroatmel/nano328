//
// Atmel a328p RealTime driver routines
// by pmichel
//

// The RealTime requires the package:
// ...
//
// The RealTime requires the define:
//
// The RealTime requires the A328P pins:
//
//


#define A328P_RT_MAIN
#include "a328p_rt.h"


#ifdef  USE_RT        // Option to enable/disable the REALTIME module

   void rt_init_disp(void)
   {
   TIMSK1 |= 1 <<  TOIE1;    // timer1 interrupt when Overflow                    ///////////////// SP0C0
   // dont drive the pins TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
   TCCR1A  = 0x03;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
   TCCR1B  = 0x19;           // Clock divider set to 1 , FAST PWM, TOP = OCR1A
   OCR1A   = RT_CPU_K_FRAME/1;      // Clock divider set to 1, so RT_CPU_K_FRAME/1 is CLK / 1000 /1 thus, every 1000 there will be an interrupt : 1Khz
   OCR1B   = RT_CPU_K_FRAME/2;     // By default, set the PWM to 50%
   }
   
   void rt_wait_us(unsigned short time)     /// TO BE TESTED... with the scope TODO
   {
   unsigned short now = TCNT1;           // read timer1 counter...
   unsigned long  target = now + time*(RT_CPU_K_FRAME/1000);
   if ( time > RT_CPU_K_FRAME )               // the amount of time to wait is so long that we should use wait_ms()
      {                                  // this part is written in case... but really, wait_us() should be called with small values
      unsigned long rt_now = rt_TIMER1;
      unsigned long rt_time = time/1000;
      while ( rt_TIMER1-rt_now < rt_time ) ;
      return;
      }
   
   if ( target > RT_CPU_K_FRAME )             // TCNT1 never goes over RT_CPU_K_FRAME ...
      {
      target -= RT_CPU_K_FRAME;               // Set the target for after the loop around. . . 
      while ( TCNT1 >= now ) ;           // wait until it starts over
      }
   
   while ( TCNT1 < target ) ;            // wait until we reach the count. . .
   }
   
   void rt_wait_ms(unsigned long time)
   {
   unsigned long rt_now = rt_TIMER1;
   while ( rt_TIMER1-rt_now < time ) ;
   }
   void rt_wait(unsigned short time)
   {
   unsigned long rt_now = rt_TIMER1;
   while ( rt_TIMER1-rt_now < time*1000 ) ;
   }

#else

   #define rt_init_disp()   ;
   #define rt_wait_us(time) ;
   #define rt_wait_ms(time) ;
   #define rt_wait(time)    ;


#endif // USE_RT


