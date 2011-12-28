/*
$Author: pmichel $
$Date: 2011/12/28 22:30:03 $
$Id: telescope.c,v 1.34 2011/12/28 22:30:03 pmichel Exp pmichel $
$Locker: pmichel $
$Revision: 1.34 $
$Source: /home/pmichel/project/telescope/RCS/telescope.c,v $

TODO:
- > Detect interrupt mayham where the histogram reads values all over the place
-> remove the falling edge interrupt, instead do everything in the same handler....to aviod skipping steps
- Add main stars from directory
- Code to toggle motor on/off :  M
- Code to reset histogram     :  H
- add a command that displays at the console : time, position, star name (corrected star position)
- add a panic button that stops a movement (without the need to reset and loose everything)
- add a way to change the direction of movement, when going to far positions, the tube might start to turn the wring way (shortedt path)
- process RETURN as command complete
- decode modes:  GOTO / MOVEMENT / DIRCTORY
- decode command set_star             [RECORD]              (DIRECTORY MODE : the current H/W position is the corrected position for the active star) 
- decode command set_pos              [RECORD + Number]     (MOVEMENT MODE : the current H/W position is stored in memory to go back after) 
- decode command go_ra                [VOL+- + numbers]     (GOTO MODE : manual goto ex: CH+ 0130 means : North 1 deg 30 minutes 00.00 sec)
- decode command go_dec               [CH+- + numbers]      (GOTO MODE)
- display command on LCD
- do banding in SP0 to handle both axis and not burn too much CPU time, tried quickly and it doesn't work first shot

*/
#define AP0_DISPLAY 0    // ca plante asse souvent si j'essaye de rouller avec AP)

// This is a Little Endian CPU
// Notes
// in minicom, to see the degree character, call minicom: minicom -t VT100 
#include <pololu/orangutan.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ADC6C            6
#define AI_BATTERY       ADC6C

#define DO_10KHZ         IO_B2
#define DI_REMOTE        IO_D2
#define DO_DISABLE       IO_D4

#define DO_DEC_DIR       IO_C3  // 0x11   17
#define DO_DEC_STEP      IO_C2  // 0x10   16
#define DO_RA_DIR        IO_C1  // 0x0F   15
#define DO_RA_STEP       IO_C0  // 0x0E   14

#define FAST_RA_STEP     0x01
#define FAST_RA_DIR      0x02
#define FAST_DEC_STEP    0x04
#define FAST_DEC_DIR     0x08


char fast_portc=0;
// Use all pins of PORTB for fast outputs  ... B0 B1 cant be used, and B4 B5 causes problem at download time
#define FAST_SET_RA_DIR_(VAL)                   \
   {                                           \
   set_digital_output(DO_RA_DIR  ,VAL    );    \
   }
#define FAST_SET_RA_STEP_(VAL)                  \
   {                                           \
   set_digital_output(DO_RA_STEP ,VAL    );    \
   }
#define FAST_SET_RA_DIR(VAL)                   \
   {                                           \
   if ( VAL ) PORTC |= (1<<DO_RA_DIR);      /* Set   C3 */   \
   else       PORTC &= 255-(1<<DO_RA_DIR);  /* Reset C3 */   \
   }
#define FAST_SET_RA_STEP(VAL)                  \
   {                                           \
   if ( VAL ) PORTC |= (1<<DO_RA_STEP);      /* Set   C2 */   \
   else       PORTC &= 255-(1<<DO_RA_STEP);  /* Reset C2 */   \
   }
#define FAST_SET_DEC_DIR(VAL)                  \
   {                                           \
   set_digital_output(DO_DEC_DIR  ,VAL    );   \
   }
#define FAST_SET_DEC_STEP(VAL)                 \
   {                                           \
   set_digital_output(DO_DEC_STEP ,VAL    );   \
   }
 
////////////////////////////////// DEFINES /////////////////////////////////////   TICKS_P_STEP * 16 * GEAR_BIG/GEAR_SMALL * STEP_P_REV / RA_DEG_P_REV
#define F_CPU_K          20000
#define FRAME            10000
#define baud             9600
#define GEAR_BIG         120
#define GEAR_SMALL       24
#define STEP_P_REV       200
#define RA_DEG_P_REV     3
#define DEC_DEG_P_REV    6
#define POLOLU           1
#define ACCEL_PERIOD     100
// 450 = nb cycles per day / nb steps per full turn  = 24*60*60*10000Hz / [ (360/RA_DEG_P_REV) * 200 steps per motor turn * (GEAR_BIG/GEAR_SMALL) * 16 microstep
#define EARTH_COMP       450    
// Maximum number of step per full circle on RA axis
// one turn on RA is 3 deg, so 360 deg is 120 turns
// the gear ration is 120 for 24, so the ration is 5 (5 turn of the stepper moter is one turn on the RA axis)
// one turn is 200 steps, each steps is devided in 16 by the stepper controler
// so, there are 120 * 5 * 200 * 16  steps on the RA axis : 1920000 : 0x1D4C00
// so , each step is 0x100000000 / 0x1D4C00 = 2236.962133 ~= 0x8BD
// so 2236 is the highest value of ticks that we can use for one step
// si if we use 6*360 = 2160, we are close to 2236 and we have a value that makes the math a lot easyer
//
/// 1 Day = 86400 Seconds                   100 Steps = 3 degrees (Dec axis)  
///       = 9600 * 9 (seconds)              200 Steps = 9 Seconds = 3 degrees (RA axis)     
///       = 9600 * 200 steps                  1 Step  = 0.045 Sec
///
#define DEAD_BAND        (147767296)                        //  147767296    // 0x08CEC000  // This is 0x100000000 - TICKS_P_DAY = 147767296
#define TICKS_P_STEP     (6*360)                            //       2160    //             // 2160 which is easy to sub divide without fractions
#define TICKS_P_DAY      (9600UL*200UL*TICKS_P_STEP      )  // 4147200000    // 0xF7314000  // One day = 360deg = 9600*200 micro steps
#define TICKS_P_DEG      (9600UL*200UL*TICKS_P_STEP/360UL)  //   11520000    //             // One day = 360deg = 9600*200 steps
#define TICKS_P_180_DEG  (9600UL*200UL*TICKS_P_STEP/2UL  )  // 2073600000    // 0x7B98A000  // One day = 360deg = 9600*200 micro steps
#define TICKS_P_45_DEG   (9600UL*25UL*TICKS_P_STEP       )  //  518400000    // 0x1EE62800  //
#define TICKS_P_DEG_MIN  (TICKS_P_DEG/60UL)                 //     192000    //             // 
#define TICKS_P_DEG_SEC  (TICKS_P_DEG/3600UL)               //       3200    //             // 
#define TICKS_P_HOUR     (9600UL*200UL*TICKS_P_STEP/24UL)   //  172800000    //             // 
#define TICKS_P_MIN      (TICKS_P_HOUR/60UL)                //    2880000    //             // 
#define TICKS_P_SEC      (TICKS_P_HOUR/3600UL)              //      48000    //             // 

#define MAX_SPEED        (TICKS_P_STEP-TICKS_P_STEP/EARTH_COMP-2)

// This macro makes sure we never stay in the dead band
#define ADD_VALUE_TO_POS(VALUE,POS)                                         \
{                                                                           \
POS += VALUE;                                                               \
if ( (unsigned long)(POS) >= TICKS_P_DAY )   /* we are in the dead band */  \
   {                                                                        \
   if ( VALUE > 0 ) POS += DEAD_BAND; /* step over the dead band */         \
   else             POS -= DEAD_BAND; /* step over the dead band */         \
   }                                                                        \
}

// I define: 
// A step is a pure step from the motor (only one coil is 100% ON at a time)        200 steps = 1 full turn ... with the gear ratio : 1000 steps = 1 full turn = 3 deg (ra) = 12 minutes  
// A microstep is 1/16th of a step and is managed by the stepper motor controller   16 microstep = 1 step
// A TICK is the name I use for the fraction of microsteps,                         2160 ticks = 1 microstep 

/// STRUCT must not use up more than 16 longs because it uses one row in the display routine
typedef struct   // those values are required per axis to be able to execute goto commands
   {             // I use a structure so that both DEC and RA axis can use the exact same routines
   long  pos;              // units: ticks (this is the current sky position)
   long  pos_cor;          // units: ticks (corrected demmanded position using polar error)
   long  pos_hw;           // units: ticks (current position of the tube, this value should be the same as the corrected position) 
   long  speed;            // units: ticks per iteration
   long  state;            // no units                     
   long  pos_initial;      // units: ticks
   long  pos_target;       // units: ticks
   long  pos_delta;        // units: ticks (it's the required displacement: pos_target-pos_initial)
   long  pos_displ;        // units: ticks (it's the displacement count)
   long  pos_dem;          // units: ticks (demanded position = sky position + Earth compensation)
   long  pos_earth;        // units: ticks (it's the calculation of the ticks caused by the earth's rotation)
   long  pos_part1;        // units: ticks (it's ths total amount of displacement during acceleration)
   long  last_high_speed;  // units: ticks per iteration 
   char  accel;            // units: nanostep per iteration per iteration
   char  accel_seq;        // no units (it's the sequence counter)
   char  spd_index;        // no units (requested slew speed)
   unsigned char  next;             // no units (should there be a step at the next iteration ? )                    
   unsigned char  direction;        // no units (which direction ? )                    
   } AXIS;

AXIS *ra;   // points in the display array dd_v
AXIS *dec;  // points in the display array dd_v

#define NB_SPEEDS 13
PROGMEM const long speed_index[NB_SPEEDS] = {0,1,2,5,10,20,50,100,200,500,1000,2000,MAX_SPEED};


PROGMEM const char NLNL[]="\012\015";
#ifdef POLOLU
// this is 1K if slash:
PROGMEM const char pololu[]={"\
                ------------------------------\012\015\
           M1B  < 01                      24 >  VCC\012\015\
           M1A  < 02                      23 >  GND\012\015\
           PB0  < 03   IO/CLK/TIMER       22 >  M2BN\012\015\
           PB1  < 04   IO/PWM             21 >  M2A\012\015\
DO 10Khz   PB2  < 05   IO/PWM     AI/IO   20 >  PC5 (ADC5/SCL)\012\015\
           PB4  < 06   IO         AI/IO   19 >  PC4 (ADC4/SDA)\012\015\
           PB5  < 07   IO         AI/IO   18 >  PC3 (ADC3) DO  DEC DIR\012\015\
DI RX232   PD0  < 08   IO         AI/IO   17 >  PC2 (ADC2) DO  DEC STEP\012\015\
DO TX232   PD1  < 09   IO         AI/IO   16 >  PC1 (ADC1) DO  RA DIR\012\015\
IR REMOTE  PD2  < 10   IO         AI/IO   15 >  PC0 (ADC0) DO  RA STEP\012\015\
DO DISABLE PD4  < 11   IO         AI      14 >      (ADC6) AIP BATTERY MONITOR\012\015\
           PD7  < 12   IO                 13 >  PC6 (RESET)                      * PD7 cant be used as DIP !\012\015\
                ------------------------------\012\015\
\012\015\
RS232 Commands:\012\015\
  <   >  Prev/Next Catalog Star\012\015\
  [   ]  Move to Prev/Next Catalog Star\012\015\
    8    Move North\012\015\
  4 5 6  Move West / All Stop / Move East\012\015\
    2    Move South\012\015\
      *  Go to current star\012\015\
      +  Save current position to slot X\012\015\
    G    Go to position specified by the next characters\012\015\
    !    To redraw screen\012\015\
  (   )  Start / Stop earth tracking\012\015\
\012\015\
"};

#else
// this is 1K if slash:
PROGMEM const char dip328p[]={"\
                            -----------\012\015\
 (PCINT14/RESET)      PC6  < 01     15 > PC5 (ADC5/SCL/PCINT13)\012\015\
 (PCINT16/RXD)        PD0  < 02     16 > PC4 (ADC4/SDA/PCINT12)\012\015\
 (PCINT17/TXD)        PD1  < 03     17 > PC3 (ADC3/PCINT11)\012\015\
 (PCINT18/INT0)       PD2  < 04     18 > PC2 (ADC2/PCINT10)\012\015\
 (PCINT19/OC2B/INT1)  PD3  < 05     19 > PC1AREF (ADC1/PCINT9)\012\015\
 (PCINT20/XCK/T0)     PD4  < 06     20 > PC0 (ADAVCCC0/PCINT8)\012\015\
                      VCC  < 07     21 > GND\012\015\
                      GND  < 08     22 > AREF\012\015\
 (PCINT6/XTAL1/TOSC1) PB6  < 09     23 > AVCC\012\015\
 (PCINT7/XTAL2/TOSC2) PB7  < 10     24 > PB5 (SCK/PCINT5)\012\015\
 (PCINT21/OC0B/T1)    PD5  < 11     25 > PB4 (MISO/PCINT4)\012\015\
 (PCINT22/OC0A/AIN0)  PD6  < 12     26 > PB3 (MOSI/OC2A/PCINT3)\012\015\
 (PCINT23/AIN1)       PD7  < 13     27 > PB2 (SS/OC1B/PCINT2)\012\015\
 (PCINT0/CLKO/ICP1)   PB0  < 14     28 > PB1 (OC1A/PCINT1)\012\015\
                            -----------\012\015\
"};
#endif

#define STAR_NAME_LEN 23
PROGMEM const char pgm_stars_name[]   =  // format: Constellation:Star Name , the s/w will use the first 2 letters to tell when we reach the next constellation
                    {
                     "Orion:Betelgeuse (Red)\0"     /*  0   */  \
                     "Orion:Rigel (Blue)    \0"     /*  1   */  \
                     "Pisces:19psc          \0"     /*  2   */  \
                     "Lyra:Vega             \0"     /*  3   */  \
                     "Gynus:Deneb           \0"     /*  4   */  \
                     "Gynus:Sadr            \0"     /*  5   */  \
                     "Origin: 0,0           \0"     /*  6   */  \
                     "Q1 RA+,DEC+           \0"     /*  7   */  \
                     "Q2 RA-,DEC+           \0"     /*  8   */  \
                     "Q3 RA+,DEC-           \0"     /*  9   */  \
                     "Q4 RA-,DEC-           \0"     /*  10  */  \
                     "Orion:Saiph           \0"     /*  11  */  \
                     "Orion:Bellatrix       \0"     /*  12  */  \
                     "Orion-B:Alnitak       \0"     /*  13  */  \
                     "Orion-B:Mintaka       \0"     /*  14  */  \
                     "Orion-B:Alnilam       \0"     /*  15  */  \
                     "Pignon Maison         \0"     /*  15  */  \
                     "\0"\
                    };
// pignon: -83°58'30.6"  (E) 075°03'39.1"  05h00m14.61s
PROGMEM const unsigned long pgm_stars_pos[] =    // Note, the positions below must match the above star names
                    {                  //   RA                                                DEC
                      ( 5*TICKS_P_HOUR+55*TICKS_P_MIN+10.3*TICKS_P_SEC), (  7*TICKS_P_DEG+24*TICKS_P_DEG_MIN+25  *TICKS_P_DEG_SEC),     // Orion:Betelgeuse (Red)  5h55m10.3  +7;24'25.0
                      ( 5*TICKS_P_HOUR+14*TICKS_P_MIN+32.3*TICKS_P_SEC), (351*TICKS_P_DEG+47*TICKS_P_DEG_MIN+54.0*TICKS_P_DEG_SEC),     // Orion:Rigel (Blue)      5h14m32.3  -8;12'06.0
                      (23*TICKS_P_HOUR+46*TICKS_P_MIN+23.5*TICKS_P_SEC), (  3*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Pisces:19psc           23h46m23.5s  3;29'12.0
                      (18*TICKS_P_HOUR+36*TICKS_P_MIN+56.3*TICKS_P_SEC), ( 38*TICKS_P_DEG+47*TICKS_P_DEG_MIN+ 3  *TICKS_P_DEG_SEC),     // Lyra:Vega              18h36m56.3 +38;47'03.0
                      (20*TICKS_P_HOUR+41*TICKS_P_MIN+25.9*TICKS_P_SEC), ( 45*TICKS_P_DEG+16*TICKS_P_DEG_MIN+49  *TICKS_P_DEG_SEC),     // Gynus:Deneb            20h41m25.9 +45;16'49.0
                      (20*TICKS_P_HOUR+22*TICKS_P_MIN+13.7*TICKS_P_SEC), ( 40*TICKS_P_DEG+15*TICKS_P_DEG_MIN+24  *TICKS_P_DEG_SEC),     // Gynus:Sadr             20h22m13.7 +40;15'24.0
                      0,0,   // origin
                      ( 1*TICKS_P_HOUR+16*TICKS_P_MIN+23.5*TICKS_P_SEC), (  3*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Inhouse test point Q1
                      (23*TICKS_P_HOUR+46*TICKS_P_MIN+23.5*TICKS_P_SEC), (  3*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Inhouse test point Q2
                      ( 1*TICKS_P_HOUR+16*TICKS_P_MIN+23.5*TICKS_P_SEC), (353*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Inhouse test point Q3
                      (23*TICKS_P_HOUR+46*TICKS_P_MIN+23.5*TICKS_P_SEC), (353*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Inhouse test point Q4
                      ( 5*TICKS_P_HOUR+47*TICKS_P_MIN+45.4*TICKS_P_SEC), (350*TICKS_P_DEG+19*TICKS_P_DEG_MIN+49  *TICKS_P_DEG_SEC),     // Orion: Saiph     7h47m45.4s -9;40;11.0
                      ( 5*TICKS_P_HOUR+25*TICKS_P_MIN+ 7.9*TICKS_P_SEC), (  6*TICKS_P_DEG+20*TICKS_P_DEG_MIN+59.0*TICKS_P_DEG_SEC),     // Orion: Bellatrix 5h25m7.9s   6;20;59.0
                      ( 5*TICKS_P_HOUR+40*TICKS_P_MIN+45.5*TICKS_P_SEC), (358*TICKS_P_DEG+3 *TICKS_P_DEG_MIN+27.0*TICKS_P_DEG_SEC),     // Orion: Alnitak   5h40m45.5  -1;56;33.0
                      ( 5*TICKS_P_HOUR+32*TICKS_P_MIN+ 0.4*TICKS_P_SEC), (359*TICKS_P_DEG+42*TICKS_P_DEG_MIN+ 3.0*TICKS_P_DEG_SEC),     // Orion: Mintaka   5h32m0.4   -0;17.57.0
                      ( 5*TICKS_P_HOUR+36*TICKS_P_MIN+ 12 *TICKS_P_SEC), (358*TICKS_P_DEG+48*TICKS_P_DEG_MIN+ 0.0*TICKS_P_DEG_SEC),     // Orion: Alnilam   5h36m12s   -1;12;00.00
                      ( 5*TICKS_P_HOUR+ 0*TICKS_P_MIN+14.6*TICKS_P_SEC), (276*TICKS_P_DEG+ 1*TICKS_P_DEG_MIN+29.4*TICKS_P_DEG_SEC),     // Orion: Alnilam   5h36m12s   -1;12;00.00
                      0,0 // last one
                    };

PROGMEM const char pgm_free_mem[]="Free Memory:";
PROGMEM const char pgm_starting[]="Telescope Starting...";
PROGMEM const char pgm_display_bug[]="Display routines problem with value (FMT_NE/EW):";
PROGMEM const char pgm_display_bug_ra[]="Display routines problem with value (FMT_RA):";
volatile unsigned long d_TIMER0;
volatile unsigned long d_TIMER1;
volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;
unsigned long d_now;
short ssec=0;
unsigned short histo_sp0=1; // place to store histogram of 10Khz interrupt routine

long l_ra_pos1,l_ra_pos2,l_dec_pos,l_ra_pos_hw,l_ra_pos_hw1,l_ra_pos_hw2,l_dec_pos_hw;
long l_ra_pos_cor1,l_ra_pos_cor2,l_dec_pos_cor;
long l_star_ra1,l_star_ra2,l_star_dec;
long l_ra_speed,l_dec_speed;
char l_ra_state,l_dec_state;

volatile char moving=0;
volatile char motor_disable=1;       // Stepper motor Disable
volatile char goto_cmd=0;            // 1 = goto goto_ra_pos
volatile char slew_cmd=0;            //     slew position
volatile char stop_cmd=0;            // Stop any movement or slew
volatile char earth_tracking=0;      // Stop earth tracking

void display_data(char *str,short xxx,short yyy,const char *pgm_str,unsigned long value,char fmt,char size);

////////////////////////////////////////////// SIN COS /////////////////////////////////////////////////////////////   (uses 3K if flash)

// mult two fixed point values
// a = 0xMMMMLLLL = 0xMMMM0000 + 0x0000LLLL (a positive)   = 0xMMMM0000 + 0xFFFFLLLL (a negative) 
// 
long fp_mult(long a, long b)
{
long tah,tal;
long tbh,tbl;
long sum; 

tah = a >> 16;         // temp a high part  (sign extend)
tal = a & 0x0000FFFF;  // temp a low part
tbh = b >> 16;         // temp a high part  (sign extend)
tbl = b & 0x0000FFFF;  // temp a low part

sum = tah * tbh * 2;  // both high parts        // the *2 is because .1b * .1b = 0.01b  but 4000 * 4000 = 1000 0000 but we want 0x 2000 0000 (0.01b)
sum += (tah * tbl + 0x8000) >> 15;              // the 0x8000 is for rounding up, but probably needs to be different for negative numbers
sum += (tbh * tal + 0x8000) >> 15;

return sum;
}

// Fixed point representation of the first factorials 1/n!
#define FAC_2 ((long)0x40000000) // 1/2!  = 1/2     
#define FAC_3 ((long)0x15555555) // 1/3!  = 1/6
#define FAC_4 ((long)0x05555555) // 1/4!  = 1/24   
#define FAC_5 ((long)0x01111111) // 1/5!  = 1/120
#define FAC_6 ((long)0x002D82D8) // 1/6!  = 1/720  
#define FAC_7 ((long)0x00068068) // 1/7!  = 1/5040
#define FAC_8 ((long)0x0000D00D) // 1/8!  = 1/40320 
#define FAC_9 ((long)0x0000171E) // 1/9!  = 1/362880
#define FAC_A ((long)0x00000250) // 1/10! = 1/3628800

// this function receives an angle in radian (fixed point)
// the angle must be between -PI/4 and PI/4   (-45 to 45)
// and if COS is true,  then the function returns the COS(angle)
// and if COS is false, then the function returns the SIN(angle)
// the returned value is in fixed point aswell
//
long fp_sin_low(long fp_45_e1,char COS)  
{
long   fp_45_e2, fp_45_e3, fp_45_e4, fp_45_e5, fp_45_e6, fp_45_e7, fp_45_e8, fp_45_e9, fp_45_eA;
long   fp_23,fp_45,fp_67,fp_89,fp_AB;  // combining them to save stack space
long   ret_val;


fp_45_e2 = fp_mult(fp_45_e1,fp_45_e1);  // x^2
fp_45_e3 = fp_mult(fp_45_e2,fp_45_e1);  // x^3
fp_45_e4 = fp_mult(fp_45_e2,fp_45_e2);  // x^4
fp_45_e5 = fp_mult(fp_45_e3,fp_45_e2);  // x^5
fp_45_e6 = fp_mult(fp_45_e3,fp_45_e3);  // x^6
fp_45_e7 = fp_mult(fp_45_e4,fp_45_e3);  // x^7
fp_45_e8 = fp_mult(fp_45_e4,fp_45_e4);  // x^8
fp_45_e9 = fp_mult(fp_45_e5,fp_45_e4);  // x^9
fp_45_eA = fp_mult(fp_45_e5,fp_45_e5);  // x^A

if (  COS ) fp_23 = fp_mult(fp_45_e2,FAC_2);  // cos  x^2/2!
else        fp_23 = fp_mult(fp_45_e3,FAC_3);  // sin  x^3/3!
if (  COS ) fp_45 = fp_mult(fp_45_e4,FAC_4);  // cos  x^4/4!
else        fp_45 = fp_mult(fp_45_e5,FAC_5);  // sin  x^5/5! 
if (  COS ) fp_67 = fp_mult(fp_45_e6,FAC_6);  // cos  x^6/6!
else        fp_67 = fp_mult(fp_45_e7,FAC_7);  // sin  x^7/7!
if (  COS ) fp_89 = fp_mult(fp_45_e8,FAC_8);  // cos  x^8/8!
else        fp_89 = fp_mult(fp_45_e9,FAC_9);  // sin  x^9/9!
if (  COS ) fp_AB = fp_mult(fp_45_eA,FAC_A);  // cos  x^10/10!
else        fp_AB = 0x00000000;

// TAYLOR: cos(x) = 1 - x^2/2! + x^4/4! - x^6/6! + x^8/8! - x^10/10!
// TAYLOR: sin(x) = x^1/1! - x^3/3! + x^5/5! - x^7/7! + x^9/9!

if ( COS ) ret_val  = 0x7FFFFFFF;  // 1.0
else       ret_val  = fp_45_e1;

ret_val -= fp_23;
ret_val += fp_45;
ret_val -= fp_67;
ret_val += fp_89;
ret_val -= fp_AB;

return ret_val;
}

// receives a positive angle, units: TICKS  (telescope position 000 to 360 deg )
// and returns the sinus of that angle
// since the fp_sin_low function only works with angles below +=45 deg
// I have to split a full sinus cycle in sections of 45 degrees and use sin() or cos()
// 
// usage:
// long ccc = fp_cos(tick);
// long sss = fp_sin(tick);

long fp_sin(unsigned long tick_angle)
{
long rad,tick;
if      ( tick_angle > TICKS_P_45_DEG * 7 ) tick = tick_angle - 8*TICKS_P_45_DEG;   // then use  sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 5 ) tick = tick_angle - 6*TICKS_P_45_DEG;   // then use -cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 3 ) tick = tick_angle - 4*TICKS_P_45_DEG;   // then use -sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 1 ) tick = tick_angle - 2*TICKS_P_45_DEG;   // then use  cos(x)
else                                        tick = tick_angle - 0*TICKS_P_45_DEG;   // then use  sin(x)

tick = tick << 2 ; // multiply by 4 because the factor is 3.253529539    -> TICKS * 3.253529539 = RADIANS in fixed point
                  // 3.25 exceeds the floating point range which is 1.0, so we multiply by 4 then by 0.813382384 (which is 3.253529539/4.0)
rad = fp_mult(tick,(long)0x681CE9FB);

if      ( tick_angle > TICKS_P_45_DEG * 7 ) return  fp_sin_low(rad,0);                    // then  sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 5 ) return -fp_sin_low(rad,1);                    // then -cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 3 ) return -fp_sin_low(rad,0);                    // then -sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 1 ) return  fp_sin_low(rad,1);                    // then  cos(x)
else                                        return  fp_sin_low(rad,0);                    // then  sin(x)
}  

long fp_cos(unsigned long tick_angle)
{
long rad,tick;
if      ( tick_angle > TICKS_P_45_DEG * 7 ) tick = tick_angle - 8*TICKS_P_45_DEG;   // then use  cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 5 ) tick = tick_angle - 6*TICKS_P_45_DEG;   // then use  sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 3 ) tick = tick_angle - 4*TICKS_P_45_DEG;   // then use -cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 1 ) tick = tick_angle - 2*TICKS_P_45_DEG;   // then use -sin(x)
else                                        tick = tick_angle - 0*TICKS_P_45_DEG;   // then use  cos(x)

tick = tick << 2 ; // multiply by 4 because the factor is 3.253529539    -> TICKS * 3.253529539 = RADIANS in fixed point
                  // 3.25 exceeds the floating point range which is 1.0, so we multiply by 4 then by 0.813382384 (which is 3.253529539/4.0)
rad = fp_mult(tick,(long)0x681CE9FB);

if      ( tick_angle > TICKS_P_45_DEG * 7 ) return  fp_sin_low(rad,1);                    // then  cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 5 ) return  fp_sin_low(rad,0);                    // then  sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 3 ) return -fp_sin_low(rad,1);                    // then -cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 1 ) return -fp_sin_low(rad,0);                    // then -sin(x)
else                                        return  fp_sin_low(rad,1);                    // then  cos(x)
}  

/////////////////////////////////////////// RMOTE INPUTS ///////////////////////////////////////////////////////////
// long ir_code,l_ir_code,ll_ir_code,ll_l_ir_code;
// short ir_count,l_ir_count,ll_ir_count;
short l_ir_count;
/////////////////////////////////////////// RS232 INPUTS ///////////////////////////////////////////////////////////
#define RS232_RX_BUF 32
unsigned char rs232_rx_buf[RS232_RX_BUF] = {0};
//unsigned char rs232_rx_idx=0;                          // <- always points on the NULL
//unsigned char l_rs232_rx_idx=0;                       // 
volatile unsigned char rs232_rx_clr=0;               // set by foreground to tell ap0c0 that it can clear the buffer
/////////////////////////////////////////// RS232 OUTPUTS //////////////////////////////////////////////////////////
#define RS232_TX_BUF 64
unsigned char rs232_tx_buf[RS232_TX_BUF] = {0}; // buffer that contains the proper thing to display based on current d_task
unsigned char rs232_tx_idx=0;
///////////////////////////////////////// CONSOLE OUTPUTS //////////////////////////////////////////////////////////
#define CONSOLE_BUF 132
volatile char redraw=0;
unsigned char console_buf[CONSOLE_BUF]; 
volatile unsigned char console_go=0;               // set to true when something is ready for transmit
volatile const char *console_special=0;           // set to true when something is ready for transmit
unsigned short console_idx=0;                    // when 0, we are not currently transmitting
////////////////////////////////////////// DISPLAY SECTION //////////////////////////////////////////
//
//  000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111
//  012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
//18| 12345678 - 11h23m32s > event log                      
//19| 12345678 - 11h23m32s > event log    (first value is the time in sec since the reset)
//20|_______________________________________________________________________________________________________
//21|                                                                                                                        
//22|          DATE/TIME: 2011/JAN/27  11h23m32s                                                                                                                       
//23|   CURRENT LOCATION: (N)-90 59'59.0"  (W)-179 59'59.0"                                                                                 
//24|        POLAR ERROR: (N)-90 59'59.0"  (W)-179 59'59.0"                                                                
//25|       SKY POSITION: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s  0x12345678 / 0x12345678                                         
//26| CORRECTED STAR POS: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s  0x12345678 / 0x12345678                                       
//27|      TELESCOPE POS: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s  0x12345678 / 0x12345678                                       
//28|       CATALOG STAR: BETLEGEUSE       / ORION         
//29|   CATALOG STAR POS: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s   
//30|   IR CODE RECEIVED: DECODED STRING                                                                                       
//31|   IR CODE RECEIVED: 12345678 12345678 / 1234                RA SPEED:                RA STATE:  
//32| PREV CODE RECEIVED: 12345678 12345678                      DEC SPEED:               DEC STATE:  
//33| PREV PREV RECEIVED: 12345678 12345678                                              
//34|    BATTERY VOLTAGE: 12.0V
//35|          HISTOGRAM: 1234 1234 1234 1234 1234 1234 1234 1234  1234 1234 1234 1234 1234 1234 1234 1234            <-- once one reaches 65535, values are latched and printed 
//36|              DEBUG: 12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//37|                     12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//38|                     12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//39|                     12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//40|                LCD: |1234567812345678|    MODE:                                                                                          
//41|                     |1234567812345678|                                                                                                               
//42|_______________________________________________________________________________________________________
//43|    IR-PLUS> GO  23h59m59.000s  90 59'59.00"  (assembled command from IR)
//44| RS232-PLUS> GO  23h59m59.000s  90 59'59.00"  (assembled command from RS232)

PROGMEM const char display_clear_screen[]     = "\033c\033c\033[2J\033[7h"; // reset reset cls line wrap 
PROGMEM const char display_define_scrolling[] = "\033[r\033[0;20r";         // enable scrolling + define scrolling
PROGMEM const char display_sa[]               = "\033[20;0H";                           // this is the last scrooling line
PROGMEM const char display_ns_line[]          = "\033[21;0H_______________________________________________________________________________________________________";

PROGMEM const char display_main[]={"\012\015\
          DATE/TIME: \012\015\
   CURRENT LOCATION: \012\015\
        POLAR ERROR: \012\015\
       SKY POSITION: \012\015\
 CORRECTED STAR POS: \012\015\
      TELESCOPE POS: \012\015\
       CATALOG STAR: \012\015\
   CATALOG STAR POS: \012\015\
   IR CODE RECEIVED: \012\015\
   IR CODE RECEIVED:                                         RA SPEED:                RA STATE:\012\015\
 PREV CODE RECEIVED:                                        DEC SPEED:               DEC STATE:\012\015\
 PREV PREV RECEIVED: \012\015\
    BATTERY VOLTAGE: \012\015\
      SP0 HISTOGRAM: \012\015\
              DEBUG: \012\015\
\012\015\
\012\015\
\012\015\
                LCD: |                |    MODE:\012\015\
                     |                |         \012\015\
_______________________________________________________________________________________________________\012\015\
    IR-PLUS> \012\015\
 RS232-PLUS> \012\015\
"};

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// TABLE THAT DEFINES WHAT,WHERE and HOW TO PRINT ///////////////////
//
// to accelerate the s/w and reduce code size, I here make it more complicated !
// all values are all long, they are all in the _v and _p arrays
// the _p table is the previous value of _v
// in some cases, it's going to use a but more ram and a bit more flash
// but over all, the code will run faster and use less ram
// I will stop to use the if then else, and instead use a home made jump table
//
// I did a check-in before this change, the .HEX size was: 81451 bytes (28K)  free ram was: 0x044D
//              after the optimization, the .HEX size is : 57061 bytes (20K)  free ram is:  0x0311

// enum
#define   FMT_NO_VAL    0x00
#define   FMT_HEX       0x10
#define   FMT_DEC       0x20
#define   FMT_HMS       0x30
#define   FMT_NS        0x40
#define   FMT_EW        0x50
#define   FMT_RA        0x60
#define   FMT_STR       0x70
#define   FMT_ASC       0x80
#define   PGM_POLOLU    0x90
#define   PGM_DIPA328P  0xA0
#define   PGM_STR       0xB0

#define   DD_FIELDS     0x60
volatile long dd_v[DD_FIELDS]; 
volatile long dd_p[DD_FIELDS];
//#############################>> the first are fucked because of NB_DTASKS NB_DTASKS NB_DTASKS NB_DTASKS NB_DTASKS
//                                             0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
PROGMEM const unsigned char dd_x[DD_FIELDS]={ 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94 , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94     // 0x00: DEBUG  0->15
                                            , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94 , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94     // 0x10: DEBUG 16->31
                                            , 22 , 27 , 32 , 37 , 42 , 47 , 52 , 57 , 64 , 69 , 74 , 79 , 84 , 89 , 94 , 99     // 0x20: Histogram 0->15
                                            , 39 , 39 , 39 , 72 , 97 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x30: RA structure values  [pos, pos_cor,pos_hw, speed, state
                                            , 22 , 22 , 22 , 72 , 97 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x40: DEC structure values [pos, pos_cor,pos_hw, speed, state
                                            , 57 , 57 , 57 , 35 , 22 , 39 , 57 , 31 , 22 , 22 , 22 , 14 ,  0 ,  0 ,  0 ,  0     // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds, start pos dec,ra,ra
                                            };
PROGMEM const unsigned char dd_y[DD_FIELDS]={ 36 , 36 , 36 , 36 , 36 , 36 , 36 , 36 , 37 , 37 , 37 , 37 , 37 , 37 , 37 , 37     // 0x00: DEBUG  0->15
                                            , 38 , 38 , 38 , 38 , 38 , 38 , 38 , 38 , 39 , 39 , 39 , 39 , 39 , 39 , 39 , 39     // 0x10: DEBUG 16->31
                                            , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35     // 0x20: Histogram 0->15
                                            , 25 , 26 , 27 , 31 , 31 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x30: RA structure values   [pos, pos_cor,pos_hw, speed, state
                                            , 25 , 26 , 27 , 32 , 32 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x40: DEC structure values  [pos, pos_cor,pos_hw, speed, state
                                            , 25 , 26 , 27 , 22 , 29 , 29 , 29 , 31 , 31 , 32 , 28 , 44 ,  0 ,  0 ,  0 ,  0     // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds
                                            };
PROGMEM const unsigned char dd_f[DD_FIELDS]={0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18    // 0x00: DEBUG  0->15      all HEX 8 bytes
                                            ,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18    // 0x10: DEBUG 16->31      all HEX 8 bytes
                                            ,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14    // 0x20: Histogram 0->15   all HEX 4 bytes
                                            ,0x50,0x50,0x50,0x26,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00    // 0x30: RA structure values
                                            ,0x40,0x40,0x40,0x26,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00    // 0x40: DEC structure values
                                            ,0x60,0x60,0x60,0x38,0x40,0x50,0x60,0x14,0x18,0x18,0xB0,0x70,0x00,0x00,0x00,0x00    // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds
                                            };  
// define the Start of each variable in the array
#define DDS_DEBUG         0x00
#define DDS_HISTO         0x20
#define DDS_RA            0x30   // ra  = (struct AXIS*)dd_v[DDS_RA];
#define DDS_DEC           0x40   // dec = (struct AXIS*)dd_v[DDS_DEC];
#define DDS_RA_POS2       0x50 
#define DDS_RA_POS2_COR   0x51 
#define DDS_RA_POS2_HW    0x52 
#define DDS_SECONDS       0x53 
#define DDS_STAR_DEC_POS  0x54 
#define DDS_STAR_RA_POS   0x55 
#define DDS_STAR_RA_POS2  0x56 
#define DDS_IR_COUNT      0x57  // dd_v[DDS_IR_COUNT]
#define DDS_IR_CODE       0x58  // dd_v[DDS_IR_CODE]
#define DDS_IR_L_CODE     0x59  // dd_v[DDS_IR_L_CODE]
#define DDS_CUR_STAR      0x5A  // dd_v[DDS_CUR_STAR]
#define DDS_RX_IDX        0x5B  // dd_v[DDS_RX_IDX]

unsigned char dd_go(unsigned char task,char first)
{
if ( dd_p[task] != dd_v[task] || first)
   {
   short XXX = pgm_read_byte(&dd_x[task]);
   short YYY = pgm_read_byte(&dd_y[task]);
   short FMT = pgm_read_byte(&dd_f[task]);

   if ( (XXX == 0) && (YYY == 0) ) return 0; // found nothing to display

   if ( task == DDS_CUR_STAR )     display_data((char*)rs232_tx_buf,XXX,YYY,pgm_stars_name + dd_v[DDS_CUR_STAR] * STAR_NAME_LEN ,0,FMT&0xF0,FMT&0x0F);   // special cases strings stored in FLASH
   else if ( task == DDS_RX_IDX )  display_data((char*)rs232_tx_buf,XXX,YYY,0,(short)rs232_rx_buf,FMT&0xF0,FMT&0x0F); 
   else                            display_data((char*)rs232_tx_buf,XXX,YYY,0,dd_v[task],FMT&0xF0,FMT&0x0F);

   dd_p[task] = dd_v[task];
   return 1;
   }

// a bit inneficient, but here for now
dd_v[DDS_RA_POS2]     = ra->pos;
dd_v[DDS_RA_POS2_COR] = ra->pos_cor;
dd_v[DDS_RA_POS2_HW]  = ra->pos_hw;

dd_v[DDS_STAR_DEC_POS] = pgm_read_dword(&pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+1]);
dd_v[DDS_STAR_RA_POS]  = pgm_read_dword(&pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+0]);
dd_v[DDS_STAR_RA_POS2] = dd_v[DDS_STAR_RA_POS];

return 0; // found nothing to display
}


////////////////////////////////////////// DISPLAY SECTION //////////////////////////////////////////
// PRINT FORMATS:
// LAT : (N)-90 59'59.00"
// LON : (W)-179 59'59.00"
// RA  : 23h59m59.000s

void pc(char ccc);
void ps(char *ssz);
void pgm_ps(const char *ssz);

void pc(char ccc)
{
while((UCSR0A & (1 <<UDRE0)) == 0);  // data register is empty...
UDR0 = ccc;
while((UCSR0A & (1 <<UDRE0)) == 0);  // data register is empty...
}

void ps(char *ssz)   // super simple blocking print string
{
while ( *ssz ) pc(*ssz++);
}
void psnl(char *ssz)   // super simple blocking print string
{
while ( *ssz ) pc(*ssz++);
pgm_ps(NLNL);
}

void psnl_progmem(const char *ssz)   // super simple blocking print string
{
while ( pgm_read_byte(ssz) ) pc(pgm_read_byte(ssz++));
pgm_ps(NLNL);
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
if(val<-(max-1))   // 10000000000  (10^digit)-1
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '-';   // below max negative displayable
   }
else if (val>(max*10))
   {
   for ( iii=0 ; iii<digit ; iii++ ) buf[iii] = '+';   // above max positive displayable
   }
else 
   {
   if ( nnn ) val = -val;
   for ( iii=digit-1 ; iii>=0 ; iii-- , val /= 10 )
      {
      buf[iii] = val%10 + '0';
      }
   if ( nnn ) buf[0] = '-';
   else       buf[0] = '+';
   }
buf[digit] = 0;
return &buf[digit];
}

// digit is the demanded precision
// convert to ?????d??h??m??s
//            543210987654321
//            111111000000000
void phhd(char *buf,long val,short digit)
{
char *ppp=buf;
long ttt = 86400; // 3600*24;
if(val<0) val = -val;  // time only in absolute
if(digit > 10) // days are required
   {
   ppp = pxxd(ppp,val/(ttt),digit-10); 
   *ppp++ = 'd';
   }
val = val % (ttt);
if(digit > 7)
   {
   if ( digit == 8 ) ppp = pxxd(ppp,val/(3600),1); 
   else              ppp = pxxd(ppp,val/(3600),2); 
   *ppp++ = 'h';
   }
val = val % (3600);
if(digit > 4)
   {
   if ( digit == 5 ) ppp = pxxd(ppp,val/(60),1); 
   else              ppp = pxxd(ppp,val/(60),2); 
   *ppp++ = 'm';
   }
val = val % (60);
if(digit > 0)
   {
   if ( digit == 1 ) ppp = pxxd(ppp,val,1); 
   else              ppp = pxxd(ppp,val,2); 
   *ppp++ = 's';
   }
*ppp++ =  0;
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

char s_send[64];
void p_str_hex(char* str,long hex, char nl)
{
ps(str);
p08x(s_send,hex);
ps(s_send);
if ( nl ) pgm_ps(NLNL);
}

void pe_str_hex(const char* str,long hex, char nl)   // print from eprom string
{
pgm_ps(str);
p08x(s_send,hex);
ps(s_send);
if ( nl ) pgm_ps(NLNL);
}

// Main function to print data in various format
void display_data(char *str,short xxx,short yyy,const char *pgm_str,unsigned long value,char fmt,char size)
{
short iii=0;
unsigned long  jjj=0;
short svalue=(short)value;
char  ccc;
// "\033[20;0H" pgm_str value
str[iii++] = 27;
str[iii++] = '[';
if ( yyy > 9  ) str[iii++] = ((yyy%100)/10) + '0';
str[iii++] = (yyy%10) + '0';
str[iii++] = ';';
if ( xxx > 99 ) str[iii++] = (xxx/100) + '0';
if ( xxx > 9  ) str[iii++] = ((xxx%100)/10) + '0';
str[iii++] = (xxx%10) + '0';
str[iii++] = 'H';
if ( pgm_str!=0 ) while ( (ccc=pgm_read_byte(&pgm_str[jjj++])) ) str[iii++] = ccc;
str[iii] = 0;  // in case nothing matches below . . .
if ( fmt == FMT_HEX )
   {
   if      ( size == 8 ) p08x(&str[iii],value);
   else if ( size == 4 ) p04x(&str[iii],value);
   else if ( size == 2 ) p02x(&str[iii],value);
   }
else if ( fmt == FMT_HMS )  // Hour Minute Seconds                          > 11h23m32s
   {
   jjj = (value/(60*60UL))%24;
   str[iii++] = (jjj/10) + '0';
   str[iii++] = (jjj)%10 + '0';
   str[iii++] = 'h';
   jjj = (value/(60))%60;
   str[iii++] = (jjj/10) + '0';
   str[iii++] = (jjj)%10 + '0';
   str[iii++] = 'm';
   jjj = (value)%60;
   str[iii++] = (jjj/10) + '0';
   str[iii++] = (jjj)%10 + '0';
   str[iii++] = 's';
   str[iii] = 0;  // in case nothing matches below . . .
   }
else if ( fmt == FMT_NS || fmt == FMT_EW )  // North South / East Weast     > (N)-90°59'59.00" 
   {                                        //                              > (W)-179°59'59.00"
   short vvv=0,prob=0;

   str[iii++] = '(';
   if ( value < TICKS_P_180_DEG ) 
      {
      if ( fmt == FMT_NS ) str[iii++] = 'N';
      else                 str[iii++] = 'E';
      str[iii++] = ')'; 
      str[iii++] = ' '; 
      if ( fmt == FMT_NS && value>TICKS_P_DEG*90) value = TICKS_P_180_DEG-value;
      }
   else
      {
      if ( fmt == FMT_NS ) str[iii++] = 'S';
      else                 str[iii++] = 'W';
      str[iii++] = ')'; 
      str[iii++] = '-'; 
      value = TICKS_P_DAY - value;
      if ( fmt == FMT_NS && value>TICKS_P_DEG*90) value = TICKS_P_180_DEG-value;
      } 

   vvv = value / TICKS_P_DEG; 
   value -= vvv * TICKS_P_DEG;
   if ( vvv>180 ) {vvv=180;prob|=0x2000;}

   if ( fmt == FMT_EW )str[iii++] = (vvv/100) + '0';     // 100 deg
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 

   str[iii++] = 0xC2;   // degree character
   str[iii++] = 0xB0;   // degree character

   vvv = value / TICKS_P_DEG_MIN ;
   value -= vvv * TICKS_P_DEG_MIN;
   if ( vvv>=60 ) {vvv=59;prob|=0x200;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 

   str[iii++] = '\'';  
   
   vvv = value / TICKS_P_DEG_SEC ;
   value -= vvv * TICKS_P_DEG_SEC;
   if ( vvv>=60 ) {vvv=59;prob|=0x20;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 

   str[iii++] = '.';  

   vvv = (10*value) / TICKS_P_DEG_SEC ;
   if ( vvv>=10 ) {vvv=9;prob|=0x2;}
   str[iii++] = (vvv)%10 + '0'; 

   str[iii++] = '"';  
   str[iii]   = 0;  

   if ( prob ) 
      {
      if(console_go==0) {display_data((char*)console_buf,0,20,pgm_display_bug,prob,FMT_HEX,8);  console_go = 1;}
      }

   }
else if ( fmt == FMT_RA )  // Right Assention    23h59m59.000s
   {
   short vvv=0,prob=0;

   vvv = value / TICKS_P_HOUR; 
   value -= vvv * TICKS_P_HOUR;
   if ( vvv>=24 ) {vvv=23 ;prob|=0x1000;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 
   str[iii++] = 'h'; 

   vvv = value / TICKS_P_MIN ;
   value -= vvv * TICKS_P_MIN;
   if ( vvv>=60 ) {vvv=59;prob|=0x100;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 
   str[iii++] = 'm';  
   
   vvv = value / TICKS_P_SEC ;
   value -= vvv * TICKS_P_SEC;
   if ( vvv>=60 ) {vvv=59;prob|=0x10;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 
   str[iii++] = '.';  

   vvv = (100*value) / TICKS_P_SEC ;
   if ( vvv>=100 ) {vvv=99;prob|=0x1;}
   str[iii++] = (vvv/10)%10 + '0'; 
   str[iii++] = (vvv)%10    + '0'; 

   str[iii++] = 's';  
   str[iii]   = 0;  

   if ( prob ) 
      {
      if(console_go==0) {display_data((char*)console_buf,0,20,pgm_display_bug_ra,prob,FMT_HEX,8);  console_go = 1;}
      }
   }
else if ( fmt == FMT_DEC )  // Decimal value
   {
   pxxd(&str[iii],value,size); // that was easy !
   }
else if ( fmt == FMT_STR )  // String
   {
   char *ppp = ((char*)svalue);
   while ( (str[iii++] = *ppp++) );
   str[iii-1] = ' ';               
   str[iii]   = 0;  
   }
else if ( fmt == PGM_STR )  // ASCII from flash
   {
   char *ppp = ((char*)svalue);
   while ( (str[iii++] = pgm_read_byte(ppp++)) );
   str[iii-1] = ' ';               
   str[iii]   = 0;  
   }
else if ( fmt == FMT_ASC )  // ASCII
   {
   str[iii++] = '[';  
   str[iii++] = value;  
   str[iii++] = ']';  
   str[iii]   = 0;  
   }
}

void goto_pos(short pos)
{
if ( ! ( moving || goto_cmd ) )
   {
   ra->pos_target  = pgm_read_dword(&pgm_stars_pos[pos*2+0]);
   dec->pos_target = pgm_read_dword(&pgm_stars_pos[pos*2+1]);
   goto_cmd = 1;
   }
}

void display_next(void);
void display_next_bg(void) 
{
dd_v[DDS_DEBUG + 0x0C]++;
if ( AP0_DISPLAY == 0 ) display_next();  // if not printing from AP0, then print here

///// Process IR commands
if ( l_ir_count != dd_v[DDS_IR_COUNT])
   {
   long code = dd_v[DDS_IR_CODE];
   l_ir_count = dd_v[DDS_IR_COUNT]; // tell SP0 that he can go on
   if      ( code == 0x01D592A6 ) slew_cmd = 8;       // North
   else if ( code == 0x01D582A7 ) slew_cmd = 2;       // South
   else if ( code == 0x01D562A9 ) slew_cmd = 4;       // East
   else if ( code == 0x01D572A8 ) slew_cmd = 6;       // West
   else if ( code == 0x01DF420B ) slew_cmd = 5;       // Stop
   else if ( code == 0x01D062F9 ) redraw   = 1;       // redraw everything
   else if ( code == 0x01D0A2F5 ) earth_tracking=0;   // Stop tracking
   else if ( code == 0x01D0B2F4 ) earth_tracking=1;   // Start tracking
   else if ( code == 0x01D302CF ) goto_pos(6+0);      //  0
   else if ( code == 0x01D312CE ) goto_pos(6+1);      //  1
   else if ( code == 0x01D322CD ) goto_pos(6+2);      //  2
   else if ( code == 0x01D332CC ) goto_pos(6+3);      //  3
   else if ( code == 0x01D342CB ) goto_pos(6+4);      //  4
   else if ( code == 0x01D272D8 ) goto_pos(dd_v[DDS_CUR_STAR]); // GOTO
   else if ( code == 0x01C2E3D1 || code == 0x01C2F3D0 )   //   <   >
      {
      if ( code == 0x01C2E3D1 ) dd_v[DDS_CUR_STAR]--;
      if ( code == 0x01C2F3D0 ) dd_v[DDS_CUR_STAR]++;
      if ( 0 == pgm_read_byte(pgm_stars_name + dd_v[DDS_CUR_STAR]*STAR_NAME_LEN) ) dd_v[DDS_CUR_STAR]=0; // we reached the last star
      if ( dd_v[DDS_CUR_STAR]<0 )
         {
         short iii;
         for(iii=0;pgm_read_byte(pgm_stars_name + iii*STAR_NAME_LEN);iii++ );  // search the last star
         dd_v[DDS_CUR_STAR] = iii-1;
         }
      }
    
   }
///// Process Keyboard commands

if ( dd_v[DDS_RX_IDX]==0 ) {;}  // nothing to do
else if ( dd_v[DDS_RX_IDX]==1 )  // check if it's a single key command
   {
   if( rs232_rx_buf[0] == '<' || rs232_rx_buf[0] == '>')
      {
      if ( rs232_rx_buf[0] == '<') dd_v[DDS_CUR_STAR]--; 
      if ( rs232_rx_buf[0] == '>') dd_v[DDS_CUR_STAR]++; 
      if ( 0 == pgm_read_byte(pgm_stars_name + dd_v[DDS_CUR_STAR]*STAR_NAME_LEN) ) dd_v[DDS_CUR_STAR]=0; // we reached the last star
      if ( dd_v[DDS_CUR_STAR]<0 )
         {
         short iii;
         for(iii=0;pgm_read_byte(pgm_stars_name + iii*STAR_NAME_LEN);iii++ );  // search the last star
         dd_v[DDS_CUR_STAR] = iii-1;
         }
      rs232_rx_buf[0] = 0;
      dd_v[DDS_RX_IDX]=0;
      }
   else if ( rs232_rx_buf[0] == '*')
      {
      if ( ! ( moving || goto_cmd ) ) 
         {
         ra->pos_target  = pgm_read_dword(&pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+0]);
         dec->pos_target = pgm_read_dword(&pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+1]);
         goto_cmd = 1;
         rs232_rx_buf[0] = 0;
         dd_v[DDS_RX_IDX]=0;
         }
      }
   else if ( rs232_rx_buf[0] == '[' || rs232_rx_buf[0] == '[')
      {
      }
   else if ( rs232_rx_buf[0] == '(' || rs232_rx_buf[0] == ')')
      {
      if ( rs232_rx_buf[0] == '(' ) earth_tracking=1;
      else                          earth_tracking=0;
      rs232_rx_buf[0] = 0;
      dd_v[DDS_RX_IDX]=0;
      }
   else if ( rs232_rx_buf[0] == '!')
      {
      redraw = 1;  // redraw everything
      rs232_rx_buf[0] = 0;
      dd_v[DDS_RX_IDX]=0;
      }
   else if ( rs232_rx_buf[0] == '2' || rs232_rx_buf[0] == '4' || rs232_rx_buf[0] == '5' || rs232_rx_buf[0] == '6' || rs232_rx_buf[0] == '8')
      {
      slew_cmd = rs232_rx_buf[0] - '0';
      rs232_rx_buf[0] = 0;
      dd_v[DDS_RX_IDX]=0;
      }
   }
else if ( rs232_rx_buf[dd_v[DDS_RX_IDX]-1]==0x0D || rs232_rx_buf[dd_v[DDS_RX_IDX]-1]==0x0A )  // Return
   {
   rs232_rx_buf[0] = 0;
   dd_v[DDS_RX_IDX]=0;
   }
   
}

#define SEC 10000
#define MSEC 10
void wait(long time,long mult)
{
long d_now = d_TIMER1;
while ( d_TIMER1-d_now < time*mult ) display_next_bg(); 
}


//PROGMEM const char *display_tasks[] = {
char *display_tasks[] = {
(char*)   display_clear_screen,        // 00 PGM string
(char*)   display_define_scrolling,    // 01 PGM string
(char*)   display_ns_line,             // 02 PGM string
(char*)   display_main,                // 03 PGM string
(char*)   0,
};
#define NB_DTASKS 4                    // NB of DISPLAY TASKS IN *display_tasks[]

unsigned char  d_task=0;
void display_next(void)
{
static unsigned char  first=1;
static unsigned char  cycle=0;
static unsigned char  console_started=0;
static unsigned char  console_special_started=0;
static unsigned short d_idx=0;
unsigned char Next=0;
unsigned char CCC;

dd_v[DDS_DEBUG + 0x03]=ra->pos;
dd_v[DDS_DEBUG + 0x04]=fp_sin(ra->pos);
dd_v[DDS_DEBUG + 0x05]=ra->pos;
dd_v[DDS_DEBUG + 0x06]=ra->pos_dem;
dd_v[DDS_DEBUG + 0x07]=dec->state;
dd_v[DDS_DEBUG + 0x0D]=goto_cmd + 0x100*moving;
dd_v[DDS_DEBUG + 0x0F]=ra->state;

if ( redraw ) 
   {
   first = 1;
   redraw = d_task = cycle = console_started = console_special_started = d_idx = rs232_tx_idx = 0;
   }

// process RX
if ( (UCSR0A & 0x80) != 0)    // ********** CA CA LIT BIEN...
   {
   CCC = UDR0;
   if ( rs232_rx_clr )
      {
      rs232_rx_clr = dd_v[DDS_RX_IDX] = 0;
      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
      }
   if ( CCC == 0x08 ) // backspace
      {
      if ( dd_v[DDS_RX_IDX] > 0 ) dd_v[DDS_RX_IDX]--;
      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
      }
   else
      {
      rs232_rx_buf[dd_v[DDS_RX_IDX]++] = CCC;
      if ( dd_v[DDS_RX_IDX] >= RS232_RX_BUF ) dd_v[DDS_RX_IDX]=RS232_RX_BUF-1;
      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
      }
   }

// process TX
if ( (UCSR0A & (1 <<UDRE0)) == 0) return;  // not ready to receive next char

if ( d_task<NB_DTASKS ) // print header
   {
   Next = pgm_read_byte(&display_tasks[d_task][d_idx++]); // Display PGM

   if ( Next == 0 ) // found null go to next task
      {
      rs232_tx_buf[0] = d_idx = 0;
      if ( pgm_read_byte(&display_tasks[d_task][0]) != 0) d_task++;
      }
   }
else // print field data
   {
   if ( console_started )
      {
      if ( console_started == 3 )
         {
         console_started = console_go = 0;
         Next = '\015';
         }
      else if ( console_started == 1 )
         {
         Next = console_buf[console_idx++];
         if ( Next == 0 ) console_started++;  //end of console output
         }
      if ( console_started == 2 )
         { 
         console_started++;
         Next = '\012'; 
         }
      }
   else if ( console_special_started )
      {
      if ( console_buf[console_idx]!=0 && console_special_started==1)  // print to console area
         {Next = console_buf[console_idx++];}
      else
         {
         if ( console_special_started == 1 ) console_idx = 0;         // a bit complicated, but I save RAM
         console_special_started = 2;                                 // a bit complicated, but I save RAM
         Next = pgm_read_byte(&console_special[console_idx++]);
         if ( Next == 0 ) console_special = 0;  // we are done
         if ( Next == 0 ) console_special_started = 0;  // we are done
         }
      }
   else
      {
      if ( rs232_tx_idx != 0 ) Next = rs232_tx_buf[rs232_tx_idx++];  // continue where we were...
      if ( Next == 0 ) // reached the end
         {
         rs232_tx_idx=0;
         cycle++;
         if ( cycle%4 == 0 && console_go != 0 ) // once in a while, check for console output
            {
            console_idx = 0;
            console_started = 1; 
            return;
            }
         if ( cycle%4 == 1 && console_special != 0 ) // once in a while, check for console output
            {
            console_idx = 0;
            console_special_started = 1; 
            display_data((char*)console_buf,0,20,0,0,FMT_NO_VAL,0);
            return;
            }
   
         if ( dd_go(d_task-NB_DTASKS,first) )  Next = rs232_tx_buf[rs232_tx_idx++];     // something to display was found, rs232_tx_buf has something
         d_task++;

         if ( d_task >= DD_FIELDS + NB_DTASKS) { first = 0 ; d_task = NB_DTASKS; }
         }
      }
   } 
if ( Next != 0 ) UDR0 = Next;
}


short process_goto(AXIS *axis,char ra_axis)
{
if ( stop_cmd )
   {
   if ( axis->state == 2  || axis->state == 3) axis->state=4;  // accel or cruise  -> decel
   if ( axis->state == 6  || axis->state == 7) axis->state = axis->speed = axis->accel = 0;         // we are done
   stop_cmd = 0;
   }

if      ( axis->state == 0 ) // idle ready to receive a command
   {
   if ( goto_cmd ) axis->state++; // new goto command
   if ( slew_cmd ) axis->state = 10;  // new slew command , change to slew mode until stop_cmd
   }
else if ( axis->state == 1 ) // setup the goto
   {
   short tt;
   axis->pos_initial  = axis->pos; 
   axis->accel_seq    = 0;  // reset the sequence 
   axis->pos_displ    = 0;  // This position counter is independant of the DEAD_BAND

   // This is a tricky part:
   // because I have a dead band, I need to check if I will cross over it in the goto process
   if ( (unsigned long)axis->pos_initial < (unsigned long) TICKS_P_180_DEG )
      {
      if ( (unsigned long) axis->pos_target > (unsigned long)(axis->pos_initial + TICKS_P_180_DEG ) )   // Yes we will cross over it
         {
         axis->accel = -10; // going backward
         axis->pos_delta    = axis->pos_target - axis->pos_initial + DEAD_BAND;
         tt=1;
         }
      else if ( (unsigned long) axis->pos_target < (unsigned long)(axis->pos_initial) )  
         {
         axis->accel = -10; // going backward
         axis->pos_delta    = axis->pos_target - axis->pos_initial;
         tt=1;
         }
      else
         {
         axis->accel =  10; // going forward
         axis->pos_delta    = axis->pos_target - axis->pos_initial;
         tt=2;
         }
      }
   else  // axis->pos_initial > TICKS_P_180_DEG
      {
      if ( (unsigned long) axis->pos_target < (unsigned long) axis->pos_initial - (unsigned long)TICKS_P_180_DEG )   // Yes we will cross over it
         {
         axis->accel =  10; // going forward
         axis->pos_delta    = axis->pos_target - axis->pos_initial - DEAD_BAND;
         tt=3;
         }
      else if ( (unsigned long) axis->pos_target > (unsigned long)(axis->pos_initial) )  
         {
         axis->accel =  10; // going forward
         axis->pos_delta    = axis->pos_target - axis->pos_initial;
         tt=1;
         }
      else
         {
         axis->accel = -10; // going backward
         axis->pos_delta    = axis->pos_target - axis->pos_initial;
         tt=4;
         }
      }

   axis->state++;
   if ( axis->pos_delta == 0 ) axis->state=6; // No movement required
   if ( axis->accel < 0 )      axis->pos_delta -= TICKS_P_MIN; // Always go 1 minute downtime for mechanical friction issue

// if ( ra_axis )
// {
// dd_v[DDS_DEBUG + 0x10]=tt;
// dd_v[DDS_DEBUG + 0x11]=axis->pos_target;
// dd_v[DDS_DEBUG + 0x12]=axis->pos_delta;
// dd_v[DDS_DEBUG + 0x13]=axis->accel;
// }
// else
// {
// dd_v[DDS_DEBUG + 0x18]=tt;
// dd_v[DDS_DEBUG + 0x19]=axis->pos_target;
// dd_v[DDS_DEBUG + 0x1A]=axis->pos_delta;
// dd_v[DDS_DEBUG + 0x1B]=axis->accel;
// }
   }
else if ( axis->state == 2 )  // detect max speed, or halway point
   {
   axis->pos_part1 = axis->pos_displ;

// if ( ra_axis ) dd_v[DDS_DEBUG + 0x14]=2*axis->pos_displ;
// else           dd_v[DDS_DEBUG + 0x1C]=2*axis->pos_displ;

   if ( axis->pos_delta > 0 ) // going forward
      {
      if ( 2*axis->pos_displ >= axis->pos_delta ) // we reached the mid point
         {
         axis->state = 4; // decelerate
         axis->accel_seq = ACCEL_PERIOD - axis->accel_seq;  // reverse the last plateau
         }
      }
   else
      {
      if ( 2*axis->pos_displ <= axis->pos_delta ) // we reached the mid point
         {
         axis->state = 4; // decelerate
         axis->accel_seq = ACCEL_PERIOD - axis->accel_seq;  // reverse the last plateau
         }
      }

   if ( axis->speed == MAX_SPEED || -axis->speed == MAX_SPEED ) // reached steady state
      {
      axis->state = 3; // cruise
      }
   else axis->last_high_speed = axis->speed;

// if ( ra_axis ) dd_v[DDS_DEBUG + 0x15]=axis->state;
// else           dd_v[DDS_DEBUG + 0x1D]=axis->state;
   if ( slew_cmd ) axis->state = 20; // decelerate - emergency stop
   }
else if ( axis->state == 3 )  // cruise speed
   {
   if ( axis->pos_delta > 0 ) // going forward
      {
      if ( axis->pos_part1 + axis->pos_displ > axis->pos_delta )  // we reached the end of the cruise period
         {
         axis->state = 4; // decelerate
         axis->accel_seq = 0;  // reset the sequence 
         }
      }
   else
      {
      if ( axis->pos_part1 + axis->pos_displ < axis->pos_delta )  // we reached the end of the cruise period
         {
         axis->state = 4; // decelerate
         axis->accel_seq = 0;  // reset the sequence 
         }
      }
   if ( slew_cmd ) axis->state = 20; // decelerate - emergency stop
   }
else if ( axis->state == 4 )  // set deceleration
   {
   if ( axis->pos_delta > 0 ) axis->accel = -10; // going forward but decelerate
   else                       axis->accel =  10; // going backward but decelerate

   axis->state++;
   axis->speed = axis->last_high_speed;  // restore last high speed
   }
else if ( axis->state == 5 )  // deceleration
   {
   if ( axis->last_high_speed * axis->speed < 0 ) axis->state = 6;
   }
else if ( axis->state == 6 )  // done, since the math is far from perfect, we often are not at the exact spot
   {
   long tmp = axis->pos - axis->pos_target;
   long abs = tmp;

   axis->accel = axis->speed = 0;
   if ( abs < 0 )              abs = -abs;
   if ( tmp > 0 )              axis->speed = -100; // we went a bit too far, slowly go back
   else                        axis->speed =  100; // we stopped short, sloly go forward
   if ( abs < TICKS_P_STEP   ) axis->state = 7;    // we are very close, we are done
   }
else if ( axis->state == 7 )  // done, since the math is far from perfect, we often are not at the exact spod
   {
   axis->pos   = axis->pos_target;                      // we are very close, set the position exactly at the target position
   axis->state = axis->speed = axis->accel = 0;         // we are done
   }
else if ( axis->state == 20 )  // emergency deceleration
   {
   if ( axis->last_high_speed * axis->speed < 0 ) axis->state++;
   }
else if ( axis->state == 21)  // done, since the math is far from perfect, we often are not at the exact spod
   {
   axis->state = axis->speed = axis->accel = 0;         // we are done
   }


if ( axis->state == 10 )  // Slewing   -> for slewing, we must catch the command in the first iteration
   {
   if ( slew_cmd != 0 )
      {
      if ( ra_axis ) 
         {
         if ( slew_cmd == 6 ) axis->spd_index++;       // Right - East
         if ( slew_cmd == 4 ) axis->spd_index--;       // Left - West
         }
      else
         {
         if ( slew_cmd == 8 ) axis->spd_index++;       // Up - North
         if ( slew_cmd == 2 ) axis->spd_index--;       // Down - South
         }
      if    ( slew_cmd == 5 ) axis->spd_index = 0;     // all Stop
      if ( axis->spd_index >=  NB_SPEEDS ) axis->spd_index =  (NB_SPEEDS-1);
      if ( axis->spd_index <= -NB_SPEEDS ) axis->spd_index = -(NB_SPEEDS-1);
      }
   }

/////// do the math...
axis->accel_seq++;
if ( axis->accel_seq > ACCEL_PERIOD )
   {
   axis->accel_seq = 0;
   if ( axis->state >=10 )     // Slewing
      {
      long spd_req;
      if ( axis->spd_index < 0 ) spd_req = -pgm_read_dword(&speed_index[(short)-axis->spd_index]);   // negative speed request
      else                       spd_req =  pgm_read_dword(&speed_index[(short) axis->spd_index]);   // positive speed request
      
      if      ( axis->speed < spd_req ) // need to go positive speed...
         {
         if ( axis->speed +10 < spd_req ) axis->speed += 10;      // too fast
         else                             axis->speed  = spd_req;
         }
      else if ( axis->speed > spd_req ) // need to go negative speed...
         {
         if ( axis->speed -10 > spd_req ) axis->speed -= 10;   // too slow
         else                             axis->speed  = spd_req;
         }
      if ( axis->speed == 0 ) axis->state=0;        // stopped
      }
   else if ( axis->state >=1 ) // goto
      {
      axis->speed += axis->accel;
      if (  axis->speed > MAX_SPEED && axis->accel > 0 ) axis->accel =  0;
      if (  axis->speed > MAX_SPEED )                    axis->speed =  MAX_SPEED;
      if ( -axis->speed > MAX_SPEED && axis->accel < 0 ) axis->accel =  0;
      if ( -axis->speed > MAX_SPEED )                    axis->speed = -MAX_SPEED;
      }
   }
ADD_VALUE_TO_POS(axis->speed,axis->pos);
axis->pos_displ += axis->speed;            // Use a parallel pos counter that is independent from the DEADBAND

axis->pos_dem = axis->pos;
if ( ra_axis ) ADD_VALUE_TO_POS(axis->pos_earth,axis->pos_dem);   // add the earth's rotation only on the RA axis

axis->pos_cor = axis->pos_dem;

//ra->pos_dem = ra->pos_earth + ra->pos;
//ra->pos_cor = ra->pos_dem;  // for now, no correction

return axis->state;
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


ISR(TIMER0_OVF_vect)     // AP0C0 ... process RS232 events
{                       
unsigned long histo;
d_TIMER0++;
 
TIMSK0 = 0;     // timer0 interrupt disable (prevent re-entrant)

sei();
display_next();

while ( TCNT1*100UL > F_CPU_K*9UL ) ; // if SP0 is about to start, wait... I dont want to be interruped in the next iterations  -> while(TCNT1 > 90% of target)
TCNT0 = 0;                            // Make sure I'm not called in the next iterations by my own AP0 interrupt

if ( histo_sp0 == 0 ) // if we are not monitoring SP0
   {  
   histo = TCNT0;
   histo *= 15 * 10 * 8;  // TCNT0 counts from 0 to F_CPU_K/10/8 (see OCR0A) 
   histo /= F_CPU_K;      // TCNT0 counts from 0 to F_CPU_K/10/8 (see OCR0A) 
   if ( histo > 15 ) histo=15;
   dd_v[DDS_HISTO + histo]++;
   }

TIMSK0 = 1;     // timer0 interrupt enable
}

ISR(TIMER1_COMPB_vect)   // Clear the Step outputs
{
return;
}

void close_loop(void)  // Clear the Step outputs
{
long temp;
// too long to complete !! set_digital_output(DO_RA_STEP,0);    // eventually, I should use my routines...flush polopu...
// too long to complete !! set_digital_output(DO_DEC_STEP,0);   // eventually, I should use my routines...flush polopu...
//FAST_SET_RA_STEP(0);
//FAST_SET_DEC_STEP(0);

temp = ra->pos_hw-ra->pos_cor;
if ( temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_RA_DIR,0); // go backward
   //FAST_SET_RA_DIR(0);
   ra->direction=0x00;
   ADD_VALUE_TO_POS(-TICKS_P_STEP,ra->pos_hw);
   ra->next=FAST_RA_STEP;
   }
else if ( -temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_RA_DIR,1); // go forward
   //FAST_SET_RA_DIR(1);
   ra->direction=FAST_RA_DIR;
   ADD_VALUE_TO_POS(TICKS_P_STEP,ra->pos_hw);
   ra->next=FAST_RA_STEP;
   }
else { ra->next=0; }

temp = dec->pos_hw-dec->pos_cor;
if ( temp >= (2*TICKS_P_STEP) )      // The DEC axis has 2 times less teeths 
   {
   // too long to complete !!set_digital_output(DO_DEC_DIR,0); // go backward
   //FAST_SET_DEC_DIR(0);
   dec->direction=0x00;
   ADD_VALUE_TO_POS(-(2*TICKS_P_STEP),dec->pos_hw);
   dec->next=FAST_DEC_STEP;
   }
else if ( -temp >= (2*TICKS_P_STEP) )
   {
   // too long to complete !!set_digital_output(DO_DEC_DIR,1); // go forward
   //FAST_SET_DEC_DIR(1);
   dec->direction=FAST_DEC_DIR;
   ADD_VALUE_TO_POS( (2*TICKS_P_STEP),dec->pos_hw);
   dec->next=FAST_DEC_STEP;
   }
else { dec->next=0; }

//dd_v[DDS_DEBUG + 0x02]=ra->direction;
//dd_v[DDS_DEBUG + 0x0A]=dec->direction;

PORTC = ra->direction | dec->direction;    // I do this to optimize execution time

}

//  IR CODE:
//  The code is in the delay between "1"
//  __-----__-__-_-_-__-__-__-   
//  At 10 Khz sampling with the Proscan remote, with an average of 3 iteration 
//  The Signal stays at 1 for 3~4 iterations ; only the start bit is longer than 3~4 iterations
//  Then goes to 0 for either 0x0B~0x0C : Logic 0    or either 0x15~0x16 : Logic 1
//  The  Start bit is a "1" for ~ 0x2A then a "0" for !0x2A
//  The code with Proscan is about 25 bits of information

ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
static char IR0,IR1,IR2,IR,l_IR,code_started=0,count_bit;
unsigned long histo;
static short earth_comp=0;
static short count_0;
static long loc_ir_code;
static unsigned short ir_timeout=0;  // limit the inputs to 4 per seconds

// These takes too long to complete !!!  set_digital_output(DO_RA_STEP ,ra->next);     // eventually, I should use my routines...flush polopu...
// These takes too long to complete !!!  set_digital_output(DO_DEC_STEP,dec->next);    // eventually, I should use my routines...flush polopu...
PORTC = ra->next | dec->next | ra->direction | dec->direction;    // I do this to optimize execution time   activate the STEP CLOCK OUTPUT

//////////////////////// Process IR ///////////////////////

IR2  = IR1;
IR1  = IR0;
IR0  = is_digital_input_high(DI_REMOTE);
IR   = IR0 & IR1 & IR2;
if ( ir_timeout ) ir_timeout--;
if ( code_started!=0 || IR!=0 )
   {
   code_started = 1;
   if ( ! IR ) count_0++;   // count the 0 time
   if ( IR != l_IR ) 
      {
      l_IR = IR;
      if ( IR )
         {
         loc_ir_code = loc_ir_code<<1;   
         if ( count_0 > 0x10 ) loc_ir_code|=1;   // set the "1" bit 
         count_0 = 0;
         }
      else count_bit++;
      }
   if ( count_0 > 0x40 || count_bit==0x1A) // code over
      {
      if ( dd_v[DDS_IR_COUNT] == l_ir_count && ir_timeout==0 )  // wait for bg to process the code
         {
         dd_v[DDS_IR_L_CODE] = dd_v[DDS_IR_CODE];
         dd_v[DDS_IR_CODE]   = loc_ir_code;
         dd_v[DDS_IR_COUNT]++;
         dd_v[DDS_DEBUG + 0x1F]=dd_v[DDS_IR_CODE];
         ir_timeout = 2500;      // 1/4 seconds
         }
      count_0 = code_started = count_bit = loc_ir_code = 0;
      }
   }
   


// this takes a lot of time . . . .: ssec = (ssec+1)%10000;   
ssec++; if ( ssec == 10000 ) ssec = 0;
if ( ssec == 0) dd_v[DDS_SECONDS]++;

d_TIMER1++;             // counts time in 0.1 ms
if ( ! motor_disable )    //////////////////// motor disabled ///////////
   {
 
   ///////////// Process earth compensation
   // this takes a lot of time . . . .:earth_comp = (earth_comp+1)%EARTH_COMP;
   earth_comp++ ; if ( earth_comp == EARTH_COMP ) earth_comp = 0;
   if ( earth_comp == 0 && earth_tracking !=0 ) ADD_VALUE_TO_POS(TICKS_P_STEP,ra->pos_earth);    // Correct for the Earth's rotation
   
   ///////////// Process goto

   process_goto(ra ,1); // 1 : specify that this is the RA axis     >> 2/16
   process_goto(dec,0); // 0 : this is the DEC axis
//   if ( d_TIMER1 & 1 ) process_goto(&ra ,1); // 1 : specify that this is the RA axis     >> 2/16
//   else                process_goto(&dec,0); // 0 : this is the DEC axis
 
//   dd_v[DDS_DEBUG + 0x00]=ra->accel;
//   dd_v[DDS_DEBUG + 0x08]=dec->accel;
   
//   dd_v[DDS_DEBUG + 0x01]=ra->speed;
//   dd_v[DDS_DEBUG + 0x09]=dec->speed;
//   
//   dd_v[DDS_DEBUG + 0x02]=ra->pos_delta;
//   dd_v[DDS_DEBUG + 0x0A]=dec->pos_delta;
//   
//   dd_v[DDS_DEBUG + 0x03]=ra->state;
//   dd_v[DDS_DEBUG + 0x0B]=dec->state;
//
//   dd_v[DDS_DEBUG + 0x10]=ra->next | dec->next | ra->direction | dec->direction;
//   dd_v[DDS_DEBUG + 0x18]=goto_cmd;
//   
//   dd_v[DDS_DEBUG + 0x11]=ra->spd_index;
//   dd_v[DDS_DEBUG + 0x19]=dec->spd_index;
//   
   if ( (ra->state == 0) && (dec->state == 0)) moving=0;
   else                                        moving=1;  // we are still moving 

//   dd_v[DDS_DEBUG + 0x12]=moving;
//   dd_v[DDS_DEBUG + 0x1A]=moving;

   if ( moving ) slew_cmd=goto_cmd=0;                   // command received
   
   }

close_loop();

// This section terminates the interrupt routine and will help identify how much CPU time we use in the real-time interrupt
if ( histo_sp0 == 1 ) // if we are not monitoring SP0
   {  
   histo = TCNT1;
   histo *= 15 * 10;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
   histo /= F_CPU_K;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
   if ( histo > 15 ) histo=15;
   dd_v[DDS_HISTO + histo]++;
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
long temp = F_CPU_K*1000UL/(16UL*baud)-1;  // set uart baud rate register

UBRR0H = (temp >> 8);
UBRR0L = (temp & 0xFF);
UCSR0B= (1 <<RXEN0 | 1 << TXEN0 );  // enable RX and TX
//UCSR0B|= (1 <<RXCIE0);              // enable receive complete interrupt
//UCSR0B|= (1 <<TXCIE0);              // enable transmit complete interrupt
UCSR0C = (3 <<UCSZ00);              // 8 bits, no parity, 1 stop
}

void init_disp(void)
{
// TIMSK2 |= 1 << OCIE2A;    // Interrupt on A comparator
//   TCCR2B  = 0x02;           // Clock divider set to 8 , this will cause a Motor driver PWM at 10KHz
//   TCCR2A  = 0xF3;           // Forced to 0xF3 by the get_ms() routines

// pmichel: disabled the close loop interrupt, I now call it at the ent of the Overflow interrupt ::::: TIMSK1 |= 1 << OCIE1B;    // timer1 interrupt when Output Compare B Match
TIMSK1 |= 1 <<  TOIE1;    // timer1 interrupt when Overflow                    ///////////////// SP0C0
TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
TCCR1B  = 0x19;           // Clock divider set to 1 , FAST PWM, TOP = OCR1A
OCR1A   = F_CPU_K/10;     // Clock divider set to 1, so F_CPU_K/10 is CLK / 1000 / 10 thus, every 10000 there will be an interrupt : 10Khz
OCR1B   = 1500      ;     // By default, set the PWM to 75%    3*F_CPU_K/40 = 1500

if ( AP0_DISPLAY == 1 )      // because only one interrupt can be active on the A328P, to have AP0 requires a few tricks, I seem to get a lot of bizzare issued when AP0 is active
   {                         // so I disactivate it with AP0_DISPLAY == 0    ... I will debug this later
   TIMSK0 |= 1 << TOIE0;     // timer0 interrupt when overvlow                 ///////////////// APOC0
   TCCR0A  = 0x03;           // FAST PWM, OC0A/OC0B pins, not driven
   TCCR0B  = 0x0A;           // WGM=111 Clock divider set to 8 , this creates a PMW wave at 40Khz
   OCR0A   = F_CPU_K/(10*8); // F_CPU_K/(10*8);     Clock divider set to 8, so F_CPU_K/10/8 is CLK / 1000 / 10 thus, every 10000 there will be an interrupt : 10Khz
   OCR0B   = F_CPU_K/(20*8); // By default, set the PWM to 50%
   }
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


////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////
////////////////////////////////////////// MAIN ///////////////////////////////////////////////////


unsigned long d_ram;
unsigned long d_state;



int main_telescope(void)
{
long iii;
///////////////////////////////// Init /////////////////////////////////////////
ra  = (AXIS*)&dd_v[DDS_RA];   // init pointers
dec = (AXIS*)&dd_v[DDS_DEC];  // init pointers

init_rs232();
init_disp();


ps("\033[2J");
ps("telescope starting...");

set_digital_output(DO_DISABLE  ,motor_disable);   // Start disabled
set_digital_output(DO_RA_DIR   ,PULL_UP_ENABLED);
set_digital_output(DO_RA_STEP  ,PULL_UP_ENABLED);    
set_digital_output(DO_DEC_DIR  ,PULL_UP_ENABLED);    
set_digital_output(DO_DEC_STEP ,PULL_UP_ENABLED);    
set_digital_output(DO_10KHZ    ,PULL_UP_ENABLED);    // Set 10 Khz pwm output of OC1B (from TIMER 1) used to control the steps
set_digital_input (DI_REMOTE   ,PULL_UP_ENABLED);
//DDRC = 0x00; // set pins 0 1 2 3 of port C as output

//set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();
sei();         //enable global interrupts

wait(1,SEC);
while (console_go) display_next_bg(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_starting,d_ram,FMT_NO_VAL,8);  console_go = 1;

#ifdef POLOLU
   while (console_special); /* wait for ready */ console_special = pololu;
#else
   while (console_special); /* wait for ready */ console_special = dip328p;
#endif

while (console_go) display_next_bg(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_free_mem,d_ram,FMT_HEX,8);  console_go = 1;
while (console_go) display_next_bg(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_free_mem,d_ram,FMT_HEX,8);  console_go = 1;
while (console_go) display_next_bg();

for ( iii=0 ; iii<31 ; iii++ ) dd_v[DDS_DEBUG + iii] = iii * 0x100;

wait(1,SEC);

dd_v[DDS_DEBUG + 0x0E]=0x3333;
wait(2,SEC);

d_now   = d_TIMER1;
for(iii=0;iii<16;iii++) dd_v[DDS_HISTO + iii]=0;
dd_v[DDS_SECONDS] = ssec = 0;
motor_disable = 0;   // Stepper motor enabled...
set_digital_output(DO_DISABLE  ,motor_disable);   

#ifdef ASFAS 
   dd_v[DDS_DEBUG + 0x0E]=0x4444;
   wait(5,SEC);
   
   dd_v[DDS_DEBUG + 0x0E]=0x5555;
   //ra->pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 10UL; // thats 30 degrees , thats 10 turns, thats 357920000 , thats 0x15556D00     got:1555605F
   ra->pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 3UL; 
   dec->pos_target = 0;
   goto_cmd = 1;  
   dd_v[DDS_DEBUG + 0x0E]=0x6666;
   while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
   wait(5,SEC);
   
   
   dd_v[DDS_DEBUG + 0x0E]=0x7777;
   // go the other way around
   ra->pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 2UL;
   dec->pos_target = 0;
   goto_cmd = 1;  
   while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
   wait(5,SEC);
   
   dd_v[DDS_DEBUG + 0x0E]=0x8888;
   // go the other way around
   ra->pos_target = TICKS_P_DAY -TICKS_P_STEP * 200UL * 5UL * 16UL * 3UL;
   dec->pos_target = 0;
   goto_cmd = 1;  
   while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
   wait(5,SEC);
   
#endif
dd_v[DDS_DEBUG + 0x0E]=0x9999;
// go the other way around
//ra->pos_target  = pgm_read_dword(&pgm_stars_pos[cur_star*2+0]);
//dec->pos_target = pgm_read_dword(&pgm_stars_pos[cur_star*2+1]);
//dd_v[DDS_DEBUG + 0x10]=ra->pos_target;
//dd_v[DDS_DEBUG + 0x18]=dec->pos_target;
//goto_cmd = 1;  
//while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);


while (1 )
   {
   dd_v[DDS_DEBUG + 0x0E]=0xAAAA;
   d_now   = d_TIMER1;
   wait(1,SEC);
   dd_v[DDS_DEBUG + 0x0E]=0xBBBB;
   d_now   = d_TIMER1;
   wait(1,SEC);
   }

d_state = 0;       // simple state machine to test some logic
   
while(1);

return 0;
}

int main(void)
{
main_telescope(); 
return 0;
}

/*
$Log: telescope.c,v $
Revision 1.34  2011/12/28 22:30:03  pmichel
Major change in the display of values
saved 10K of flash
at the expence of ~250 bytes of ram
Remains to display the stars

Revision 1.33  2011/12/28 19:35:55  pmichel
This is before I start to manually optimize for speed

Revision 1.32  2011/12/27 22:20:57  pmichel
Yet another big error fixed

Revision 1.31  2011/12/27 21:57:11  pmichel
important fix for sin() cos() functions
fg

Revision 1.30  2011/12/25 03:43:03  pmichel
Version with sin/cos !!!
thats seems to work very well
unfortunately
it also seems that I wont have enough flash rom
to put all the code for polar correction

Revision 1.29  2011/12/24 04:47:36  pmichel
Added code that reduces mechanical friction problem

Revision 1.28  2011/12/23 05:49:58  pmichel
Reduced to 4 codes per seconds that the remote can send

Revision 1.27  2011/12/23 00:01:33  pmichel
Fixed the goto and the deadzone
Slew works well

Revision 1.26  2011/12/21 05:49:47  pmichel
Ok found bug,
Not sure why this version works
but Slew works with Keyboard and Remote...

Revision 1.25  2011/12/21 05:35:10  pmichel
Version with IR decoded, but slews
dont work anymore !

Revision 1.24  2011/12/20 05:21:55  pmichel
Prettymuch completed Proscan IR decoding
not exactly as per Levolor.c
but works

Revision 1.23  2011/12/19 04:10:22  pmichel
Slew speed is complete and running well

Revision 1.22  2011/12/16 05:50:52  pmichel
Now I can go to the selected star using *

Revision 1.21  2011/12/16 05:35:32  pmichel
Waow, that was fast,
I now can process < and > to select different stars from the catalog

Revision 1.20  2011/12/16 04:56:26  pmichel
Added 16 mode debug

Revision 1.19  2011/12/15 05:39:46  pmichel
Working with 2 axis
using 6/16 of cputime in standby and 8/16 when moving

Revision 1.18  2011/12/14 04:18:49  pmichel
Started some cleanup,
tested and still working...

Revision 1.17  2011/12/13 06:08:25  pmichel
Little hack to be able to store the stars

Revision 1.16  2011/12/13 04:50:35  pmichel
Dead band done
Be careful when using goto position, if you specify -VALUE
and if happens that it's outside the dead band, you might not get the proper value
always use TICKS_P_DAY - VALUE for negative values

As for AP0, the thing becomes very unstable when AP0 is active

Revision 1.15  2011/12/13 02:30:45  pmichel
Starting to process the dead band

Revision 1.14  2011/12/12 04:19:54  pmichel
Now using 6*360 ticks per steps
and all the math is so much easyer now

Revision 1.13  2011/12/11 19:24:02  pmichel
Version that uses 2^32 full span
but I see more problems than benefits
so I will go to 2236 * 9600 * 200 ticks per day


*/
