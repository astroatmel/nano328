//
// Atmel a328p RealTime driver routines
// by pmichel
//
//
// Options:
// A328P_RT_PWM : TBD, create defines to activate POM outputs and set USING_A328P_PIN_xx
//
//
//
//

// The RealTime requires the package:
// ...
//
// The RealTime requires the define:
//
// The RealTime requires the A328P pins:
//
//

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#ifdef  USE_RT        // Option to enable/disable the REALTIME module

   #ifndef A328P_RT
   #define A328P_RT
   
   
   #ifdef A328P_RT_MAIN
      // local labels only to LCD_MAIN module
      volatile unsigned char  rt_sec=0,rt_min=0,rt_hour=0;
      volatile unsigned short rt_msec;
      volatile unsigned long  rt_TIMER0, rt_TIMER1;
   #else
      extern volatile unsigned char  rt_sec,rt_min,rt_hour;
      extern volatile unsigned short rt_msec;
      extern volatile unsigned long  rt_TIMER0, rt_TIMER1;
   #endif
   
   
   void rt_init_disp(void);
   void rt_wait_us(unsigned short time);  
   void rt_wait_ms(unsigned long time);
   void rt_wait(unsigned short time);

   
   #endif // A328P_RT


#else

   // if USE_RT not defined, void the usage of the user functions:
   #define rt_init_disp() ;
   #define rt_wait_us(time)  ;
   #define rt_wait_ms(time)  ;
   #define rt_wait(time)     ;

#endif // USE_RT

#define SEC 1000
#define MSEC 1

#ifndef  RT_CPU_K_FRAME
   #define  RT_CPU_K_FRAME          20000
#endif
