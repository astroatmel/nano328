/*
$Author: pmichel $
$Date: 2012/04/18 03:35:50 $
$Id: telescope2.c,v 1.92 2012/04/18 03:35:50 pmichel Exp pmichel $
$Locker: pmichel $
$Revision: 1.92 $
$Source: /home/pmichel/project/telescope2/RCS/telescope2.c,v $


Back home test alignment:
------------------------------------
Telescope Master Starting...        
Free Memory:000001BD                


Aligned with 3 points:
41=ok
49: put 1.5 deg too high
74=ok
------ below is the result matrix -----
I used this error to mark bad points in pink

Error :00052632                                                                                                                                                                                                                                                          
Hour   :02h34m35.09s                                                                                                                                                                                                                                                     
Declin :(N) 01�°18'25.6"                                                                                                                                                                                                                                                 
Hour   :02h34m57.07s                                                                                                                                                                                                                                                     
Hour   :00h00m00.87s                                                                                                                                                                                                                                                     
Declin :(N) 00�°00'00.8"                                                                                                                                                                                                                                                 
Polar Matrix:                                                                                                                                                                                                                                                            
Data: 7FFAB9D8  0.99983904                                                                                                                                                                                                                                               
Data: FFB28B81  -.00236373                                                                                                                                                                                                                                               
Data: 0246C291  0.01778442                                                                                                                                                                                                                                               
Data: 004521FD  0.00210976                                                                                                                                                                                                                                               
Data: 7FFC9543  0.99989572                                                                                                                                                                                                                                               
Data: 01D41F6E  0.01428597                                                                                                                                                                                                                                               
Data: FDB831B8  -.01781633                                                                                                                                                                                                                                               
Data: FE2D2E9A  -.01424615                                                                                                                                                                                                                                               
Data: 7FF77918  0.99973977


TODO next version: 
-easy way to enter fake alignments points
-method to re-enter polar error RA/DEC and by-pass the alignment process (ex: need to recompile, but the stand does not move)



TODO:
****** Cleanup debug pages and make debug paper sheet so that I dont need to go search on the code each time ---- plus, one page per debug topic (polar align, states, )

*** Problems:
** Add a search patern function to locate a point of interest
** use yellow pwm sequence to indicate remote control input state (command) off = ready to process a command 25% means 1 key in, 50%: 2 key in ...  max sequqnce = 100% solid
*** if Motor off and tracking, then position decrements (we fall back)

- Add command to set the RA to an approx value (first thing to execute when aligning)
- Confirm... the master does not receive 32 bytes...does the slave receive 32 bytes ?

** when clearing corrected stars positions, ALSO reset the polar vector to 0,0,1
- Key to disable use_polar
- ramp up/down the polar correction
- show current polar X/Y displacement required (in steps)
- show total error with current polar correction calculation using the stores corrected star position
- show x and y of polar correction (good indication of direction and amplitude)
-
 > Detect interrupt mayham where the histogram reads values all over the place
- Add main stars from directory
- add a way to change the direction of movement, when going to far positions, the tube might start to turn the wring way (shortedt path)
- display command on LCD
- do banding in SP0 to handle both axis and not burn too much CPU time, tried quickly and it doesn't work first shot
- reduce the amount of ram that the sin() cos() functions use
*/
#define AP0_DISPLAY 0    // ca plante asse souvent si j'essaye de rouller avec AP)
#define TEST_POLAR  0
#define FULL_DISPLAY  1

// This is a Little Endian CPU
// Notes
// in minicom, to see the degree character, call minicom: minicom -t VT100 
//#include <pololu/orangutan.h>

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ADC6C            6
#define AI_BATTERY       ADC6C

#define DO_PB1_TORQA1    (1<<1) 
#define DO_PB2_TORQA2    (1<<2) 
#define DO_PB3_TORQB1    (1<<3) 
#define DO_PB4_TORQB2    (1<<4) 
#define DO_PB5_DECAYB2   (1<<5) 

#define DO_PC0_DEC_STEP  (1<<0) // 0x10   16
#define DO_PC1_DEC_DIR   (1<<1) // 0x11   17
#define DO_PC2_RA_STEP   (1<<2) // 0x0E   14
#define DO_PC3_RA_DIR    (1<<3) // 0x0F   15

#define DO_PC4_TWI_SDA   (1<<4) 
#define DO_PC5_TWI_SCL   (1<<5) 

#define DI_PD2_REMOTE    (1<<2) // IO_D2   PORTD BIT 2
#define DO_PD3_DISABLE   (1<<3) // IO_D3   PORTD BIT 3   // Step motor enable
#define DO_PD4_TWI_START (1<<4) 
#define DO_PD5_DECAYA1   (1<<5) 
#define DO_PD6_DECAYA2   (1<<6) 
#define DO_PD7_DECAYB1   (1<<7) 

// SLAVE
#define DO_PB7_GREEN     (1<<7) 
#define DO_PD5_YELLOW    (1<<5) 
#define DO_PD6_RED       (1<<6) 
#define DO_PB0_LCD_B4    (1<<0) 
#define DO_PB1_LCD_B5    (1<<1) 
#define DO_PB2_LCD_B6    (1<<2) 
#define DO_PB3_LCD_B7    (1<<3) 
#define DO_PC0_LCD_EE    (1<<0) 
#define DO_PC1_LCD_RW    (1<<1) 
#define DO_PC2_LCD_RS    (1<<2) 

#define IF_CONDITION_PORT_BIT(CC,PP,BB)        \
{                                               \
if ( CC ) PP |=  (BB); /* Set bits of port   */  \
else      PP &= ~(BB); /* Clear bits of port */   \
}                                     

#define IF_NOT_CONDITION_PORT_BIT(CC,PP,BB)    \
{                                               \
if ( CC ) PP &= ~(BB); /* Clear bits of port */  \
else      PP |=  (BB); /* Set bits of port   */   \
}                                     

// Macro to force some bits to some value
#define PORT_MASK_BIT(PP,MM,BB)                        \
{                                                       \
PP  =  (PP & (~(MM))) | (BB); /* Set bits of port   */   \
}                                     


void wait(long time,long mult);
#define SEC 10000
#define MSEC 10

// Debug Page 0 : value at offset 0x01 : High short = Nb Tx from Master : Low short = Nb rx from the Master
//              : values at offset 0x08 to 0x1F are used to display TWI states 
// Debug Page 1 : Shows TWI buffer of what the Master sends (on the master console)    and what the Slave receives (on the slave console)
// Debug Page 2 : Shows TWI buffer of what the Master receives (on the master console) and what the Slave sends (on the slave console)
// Debug Page 3 : 
// Debug Page 4 : 
char debug_page = 3; // depending of the debug mode, change what the debug shows 
#define  NB_DEBUG_PAGE 7 
char align_state = 0; // 0 : Waiting Reference RA     -> the first PLAY X X X tells the controler that we did point the telescope at a known bright star, thus setting the RA and DEC
                      // 1 : fake a first alignment correction  (debug)
                      // 2 : fake a second alignment correction  (debug)
                      // ...
                      // 10 : Star Position Correction -> ask the controler to go to known stars, and manually correct the position
                      // 11 : Polar Align              -> after 5 or more corrected stars , the slave will do the polar align
                      // 12 : Aligned                  -> Ready to operate
                       
unsigned char cmd_state=0;

char fast_portc=0;

// Use all pins of PORTB for fast outputs  ... B0 B1 cant be used, and B4 B5 causes problem at download time
///- #define FAST_SET_RA_DIR_(VAL)                   
///-    {                                           
///-    set_digital_output(DO_PC1_RA_DIR  ,VAL    );    
///-    }
///- #define FAST_SET_RA_STEP_(VAL)                  
///-    {                                           
///-    set_digital_output(DO_PC0_RA_STEP ,VAL    );    
///-    }
///- #define FAST_SET_RA_DIR(VAL)                  
///-    {                                           
///-    if ( VAL ) PORTC |= (1<<DO_PC1_RA_DIR);      /* Set   C3 */   
///-    else       PORTC &= 255-(1<<DO_PC1_RA_DIR);  /* Reset C3 */   
///-    }
///- #define FAST_SET_RA_STEP(VAL)                  
///-    {                                          
///-    if ( VAL ) PORTC |= (1<<DO_PC0_RA_STEP);      /* Set   C2 */   
///-    else       PORTC &= 255-(1<<DO_PC0_RA_STEP);  /* Reset C2 */   
///-    }
///- #define FAST_SET_DEC_DIR(VAL)                  
///-    {                                           
///-    set_digital_output(DO_PC3_DEC_DIR  ,VAL    );   
///-    }
///- #define FAST_SET_DEC_STEP(VAL)                 
///-    {                                           
///-    set_digital_output(DO_PC2_DEC_STEP ,VAL    );   
///-    }
 
////////////////////////////////// DEFINES /////////////////////////////////////   TICKS_P_STEP * 16 * GEAR_BIG/GEAR_SMALL * STEP_P_REV / RA_DEG_P_REV
#define F_CPU_K          20000
#define FRAME            10000
#define baud             9600
#define GEAR_BIG         120
#define GEAR_SMALL       24
#define STEP_P_REV       200
// Not TRUE:
// #define RA_DEG_P_REV     3  
// #define DEC_DEG_P_REV    6
// After verification, the RA gear seems to have 130 teeths, and the DEC has 65 , thus RA_DEG_P_REV = 2.76923...not 3.000000
// Stupid me... the two define above are not used anymore....
#define POLOLU           1
#define ACCEL_PERIOD     100
// Maximum number of step per full circle on RA axis
// FALSE: one turn on RA is 3 deg, so 360 deg is 120 turns
// FALSE: the gear ration is 120 for 24, so the ration is 5 (5 turn of the stepper moter is one turn on the RA axis)
// FALSE: one turn is 200 steps, each steps is devided in 16 by the stepper controler
// FALSE: so, there are 120 * 5 * 200 * 16  steps on the RA axis : 1920000 : 0x1D4C00
// FALSE: so , each step is 0x100000000 / 0x1D4C00 = 2236.962133 ~= 0x8BD
// FALSE: so 2236 is the highest value of ticks that we can use for one step
// FALSE: if we use 6*360 = 2160, we are close to 2236 and we have a value that makes the math a lot easyer
/////////////////////   135 * 5 * 200 * 16  steps on the RA axis : 2160000 : 0x20F580
/////////////////////       0x100000000 / 0x20F580 = 1988.4107
/////////////////////  so use 1620 which is close to 1988.4
//
/// 1 Day = 86400 Seconds    ... but we do 1 + 1/365.25 turn : 1.002737 turn so that the sun rises in the morning each day
///                              in reality one 360 degree rottation is one day minus 236.55 seconds
/// 1 Day = 86400 Seconds - 236.55 seconds   = 86163.449 seconds
/// so to compensate the earth rottation we must do 86163.449 seconds / TICKS_P_DAY = 0.041424735   (every 41ms or every 414 iterations) 
///  but 414 strait will cause 6956 excessive steps per day.... if every 1674 we dont increment, then all will fall vary close
///  
#define EARTH_COMP       414    
#define EARTH_SKIP       1674   
  
#define DEAD_BAND        (925367296)                        //  925367296    // 0x3727FC00  // This is 0x100000000 - TICKS_P_DAY = 925367296
#define TICKS_P_STEP     (4*81*5)                           //       1620    //             // 1620 which is easy to sub divide without fractions 
#define TICKS_P_DAY      (2080000UL   *TICKS_P_STEP      )  // 3369600000    // 0xC8D80400  // One day = 360deg = 2080000 micro steps   ///// Warning: fp_sin() has a factor that relies on TICKS_P_DAY being=3369600000
#define TICKS_P_DEG      (2080000UL   *TICKS_P_STEP/360UL)  //    9360000    //   0x8ED280  // 
#define TICKS_P_45_DEG   (2080000UL   *TICKS_P_STEP/8UL  )  //  421200000    // 0x191B0080  //
#define TICKS_P_90_DEG   (2080000UL   *TICKS_P_STEP/4UL  )  //  842400000    // 0x32360100  //
#define TICKS_P_180_DEG  (2080000UL   *TICKS_P_STEP/2UL  )  // 1684800000    // 0x646C0200  // 
#define TICKS_P_DEG_MIN  (TICKS_P_DEG/60UL)                 //     156000    //             // 
#define TICKS_P_DEG_SEC  (TICKS_P_DEG/3600UL)               //       2600    //             // 
#define TICKS_P_HOUR     (2080000UL   *TICKS_P_STEP/24UL)   //  140400000    //             // 
#define TICKS_P_MIN      (TICKS_P_HOUR/60UL)                //    2340000    //             // 
#define TICKS_P_SEC      (TICKS_P_HOUR/3600UL)              //      39000    //             // 

//#define MAX_SPEED        (TICKS_P_STEP-TICKS_P_STEP/EARTH_COMP-2)
#define MAX_SPEED        (TICKS_P_STEP-100)

// This function makes sure we never stay in the dead band
// using pointers to save space
void add_value_to_pos(long value,long *pos)
{
//unsigned char was_positive = *pos>=0;
//unsigned char now_positive;
*pos+=value;
if ( (unsigned long)(*pos) >= TICKS_P_DAY )   /* we are in the dead band */  
   {                                                                        
   if ( value > 0 ) *pos += DEAD_BAND; /* step over the dead band */         
   else             *pos -= DEAD_BAND; /* step over the dead band */         
   }
//else  // we are not in the dead band, but check to see if we changes side
//   {
//   now_positive = *pos>=0;
//   if (now_positive != was_positive)  // yes we changed side
//      {
//      if ( value > 0 ) *pos += DEAD_BAND;
//      else             *pos -= DEAD_BAND;
//      }
//   }
}

#define NB_SAVED 20
typedef struct
   {         
   unsigned long ra;
   unsigned long dec;
   unsigned char ref_star;  // if not 0, then it tells us that this is a Corrected star position
   } POSITION;
POSITION saved[NB_SAVED];

typedef struct
   {
   long x,y,z;
   } VECTOR;

typedef struct
   {
   long m11,m12,m13;
   long m21,m22,m23;
   long m31,m32,m33;
   } MATRIX;

MATRIX PoleMatrix;

void set_vector(VECTOR *VVV,unsigned long *RA,unsigned long *DEC);
void apply_polar_correction(MATRIX *R,VECTOR *S);
unsigned char use_polar=0;
long loc_ra_correction,ra_correction;   // local RA correction from polar error
long loc_dec_correction,dec_correction;  // local DEC correction from polar error
unsigned char last_antena=0;

// TWI exchanged data
void twitt(void);
unsigned char twi_seq=0;       // Seq: sequence counter , incremented by 2 to indicate to store a new correced star position, first bit =1 means do polar correction
unsigned char twi_fb_count=0;  // How many bytes should we send back 
unsigned char twi_hold=0;      // tell foreground to not update the current pos
long twi_test=0;               // when active, the foreground will not drive the twi_pos
long twi_pos[2];               // Current pos sent to Slave
//long twi_star_ra,twi_star_dec; // Currently Selected Star pos sent from Slave
long twi_ra_corr,twi_dec_corr; // Polar correction calculated by Slave
//short twi_star;              // Current Star index
unsigned char twi_star_ptr;    // Offset and Char to transfer the Star name from the Slave to the Master
char          twi_star_char;   // Offset and Char to transfer the Star name from the Slave to the Master

unsigned char torq=0,dcay=0;   // Requested Torq and Decay value
unsigned char effectiv_torq=0; // Effective Torq
unsigned char minumum_torq=0;  // Minumum   Torq

void record_pos(short pos);

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

#ifdef AT_SLAVE
unsigned long polar_ra,polar_dec;               // polar shift
#endif
#ifdef AT_MASTER
static char set_ra_armed=0;
PROGMEM const long mosaic_span[10] = {
                                      1 * TICKS_P_DEG_MIN, // 0 : one minute
                                      2 * TICKS_P_DEG_MIN, // 1 : two minute
                                      4 * TICKS_P_DEG_MIN, // 2 : four minute
                                      6 * TICKS_P_DEG_MIN, // 3 : six minute
                                      8 * TICKS_P_DEG_MIN, // 4 : eight minute
                                     10 * TICKS_P_DEG_MIN, // 5 : ten minute
                                     20 * TICKS_P_DEG_MIN, // 6 : twenty minute
                                     30 * TICKS_P_DEG_MIN, // 7 : thirty minute
                                     60 * TICKS_P_DEG_MIN, // 8 : one degree
                                    120 * TICKS_P_DEG_MIN, // 9 : two degrees
                                     };
long   mosaic_base_ra,mosaic_base_dec;
char   mosaic_span_idx;
char   mosaic_nb_tiles;
char   mosaic_seq_ra;
char   mosaic_seq_de;
char   mosaic_dt;
short  mosaic_seconds=-1;  // -1 means mosaic not active
#endif
unsigned char  mosaic_twi=-1;  // // Bits: 210 : Primary, Secondary, Mosaic active

AXIS *ra;   // points in the display array dd_v
AXIS *dec;  // points in the display array dd_v

#define NB_SPEEDS 13
PROGMEM const long speed_index[NB_SPEEDS] = {0,1,2,5,10,20,50,100,200,500,1000,2000,MAX_SPEED};


PROGMEM const char NLNL[]="\012\015";

#ifdef DISPLAY_HELP
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
   DO RX232   PD1  < 09   IO         AI/IO   16 >  PC1 (ADC1) DO  RA DIR\012\015\
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
MASTER:\012\015\
   >RESET     (PCINT14/RESET)      PC6  < 01     15 > PC5 (ADC5/SCL/PCINT13)   SCL               \012\015\
   >RX        (PCINT16/RXD)        PD0  < 02     16 > PC4 (ADC4/SDA/PCINT12)   SDA               \012\015\
   <TX        (PCINT17/TXD)        PD1  < 03     17 > PC3 (ADC3/PCINT11)       BATTERY<          \012\015\
              (PCINT18/INT0)       PD2  < 04     18 > PC2 (ADC2/PCINT10)       LCD RS >          \012\015\
              (PCINT19/OC2B/INT1)  PD3  < 05     19 > PC1AREF (ADC1/PCINT9)    LCD RW >          \012\015\
              (PCINT20/XCK/T0)     PD4  < 06     20 > PC0 (ADAVCCC0/PCINT8)    LCD E  >          \012\015\
                                   VCC  < 07     21 > GND                                        \012\015\
                                   GND  < 08     22 > AREF                                       \012\015\
   >CLKOUT    (PCINT6/XTAL1/TOSC1) PB6  < 09     23 > AVCC                                       \012\015\
   <LED GREEN (PCINT7/XTAL2/TOSC2) PB7  < 10     24 > PB5 (SCK/PCINT5)                           \012\015\
   <LED YELLOW(PCINT21/OC0B/T1)    PD5  < 11     25 > PB4 (MISO/PCINT4)                          \012\015\
   <LED RED   (PCINT22/OC0A/AIN0)  PD6  < 12     26 > PB3 (MOSI/OC2A/PCINT3)   LCD B7<>          \012\015\
              (PCINT23/AIN1)       PD7  < 13     27 > PB2 (SS/OC1B/PCINT2)     LCD B6<>          \012\015\
  <>LCD B4    (PCINT0/CLKO/ICP1)   PB0  < 14     28 > PB1 (OC1A/PCINT1)        LCD B5<>          \012\015\
\012\015                                                                                                 \
SLAVE:\012\015                                                                                           \
   >RESET     (PCINT14/RESET)      PC6  < 01     15 > PC5 (ADC5/SCL/PCINT13)   SCL               \012\015\
   >RX        (PCINT16/RXD)        PD0  < 02     16 > PC4 (ADC4/SDA/PCINT12)   SDA               \012\015\
   <TX        (PCINT17/TXD)        PD1  < 03     17 > PC3 (ADC3/PCINT11)       DEC_DIR>          \012\015\
   >IR        (PCINT18/INT0)       PD2  < 04     18 > PC2 (ADC2/PCINT10)       DEC_STEP>         \012\015\
   <ENABLE    (PCINT19/OC2B/INT1)  PD3  < 05     19 > PC1AREF (ADC1/PCINT9)    RA_DIR>           \012\015\
   <TWI_START (PCINT20/XCK/T0)     PD4  < 06     20 > PC0 (ADAVCCC0/PCINT8)    RA_STEP>          \012\015\
                                   VCC  < 07     21 > GND                                        \012\015\
                                   GND  < 08     22 > AREF                                       \012\015\
    XTAL      (PCINT6/XTAL1/TOSC1) PB6  < 09     23 > AVCC                                       \012\015\
    XTAL      (PCINT7/XTAL2/TOSC2) PB7  < 10     24 > PB5 (SCK/PCINT5)         decayb2>          \012\015\
   <decaya1   (PCINT21/OC0B/T1)    PD5  < 11     25 > PB4 (MISO/PCINT4)        torqb2>           \012\015\
   <decaya2   (PCINT22/OC0A/AIN0)  PD6  < 12     26 > PB3 (MOSI/OC2A/PCINT3)   torqb1>           \012\015\
   <decayb1   (PCINT23/AIN1)       PD7  < 13     27 > PB2 (SS/OC1B/PCINT2)     torqa2>           \012\015\
   <CLKOUT    (PCINT0/CLKO/ICP1)   PB0  < 14     28 > PB1 (OC1A/PCINT1)        torqa1>           \012\015\
   "};
   #endif
#endif

#include "coords.h"  // file generated from star data from http://vizier.u-strasbg.fr/viz-bin/VizieR
// PROGMEM const char pgm_stars_name_reduced[]   =  // format: byte:StarID byte:ConstelationId Star Name string
//                    pgm_stars_name_reduced is a huge contigous string with only a few stars name (not all stars in the coords list have a name)
//                                           first two bytes are unsigned char and contains StarID followed by Constellation ID
char current_star_name[STAR_NAME_LEN+CONSTEL_NAME_LEN] = ""; // place to store the current star name and constellation name (set by slave) 

PROGMEM const char pgm_free_mem[]="Free Memory:";
#ifdef AT_MASTER
PROGMEM const char pgm_starting[]="Telescope Master Starting...";
#else
   #ifdef AT_SLAVE
   PROGMEM const char pgm_starting[]="Telescope Slave Starting...";
   #else
   PROGMEM const char pgm_starting[]="Telescope Starting...";
   #endif
#endif
PROGMEM const char pgm_display_bug[]="Display routines problem with value (FMT_NE/EW):";
PROGMEM const char pgm_display_bug_ra[]="Display routines problem with value (FMT_RA):";
volatile unsigned long d_TIMER0;
volatile unsigned long d_TIMER1;
volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;
unsigned long d_now;
short ssec=0;

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


void to_console(const char *pgm_str,unsigned long value,unsigned char fmt,unsigned char size);
void display_data(char *str,short xxx,short yyy,const char *pgm_str,unsigned long value,unsigned char fmt_size);

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

#ifdef AT_SLAVE

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

///---tick = tick << 2 ; // multiply by 4 because the factor is 3.253529539    -> TICKS * 3.253529539 = RADIANS in fixed point    (this was with 1 Day  = 4147200000 ticks)
///---                  // 3.25 exceeds the floating point range which is 1.0, so we multiply by 4 then by 0.813382384 (which is 3.253529539/4.0)
///---rad = fp_mult(tick,(long)0x681CE9FB);
tick = tick << 2 ;  // multiply by 8 because the factor is 4.004344048 * TICKS = RADIANS in fixed point
                   // this value exceeds the floating point range which is 1.0, so we multiply by 8 then by 0.500543006 (which is 4.004344048/8.0)
rad = fp_mult(tick,(long)0x4011CB11);
rad = rad << 1 ;

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

///---tick = tick << 2 ; // multiply by 4 because the factor is 3.253529539    -> TICKS * 3.253529539 = RADIANS in fixed point
///---                  // 3.25 exceeds the floating point range which is 1.0, so we multiply by 4 then by 0.813382384 (which is 3.253529539/4.0)
///---rad = fp_mult(tick,(long)0x681CE9FB);
tick = tick << 2 ;  // multiply by 8 because the factor is 4.004344048 * TICKS = RADIANS in fixed point
                   // this value exceeds the floating point range which is 1.0, so we multiply by 8 then by 0.500543006 (which is 4.004344048/8.0)
rad = fp_mult(tick,(long)0x4011CB11);
rad = rad << 1 ;

if      ( tick_angle > TICKS_P_45_DEG * 7 ) return  fp_sin_low(rad,1);                    // then  cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 5 ) return  fp_sin_low(rad,0);                    // then  sin(x)
else if ( tick_angle > TICKS_P_45_DEG * 3 ) return -fp_sin_low(rad,1);                    // then -cos(x)
else if ( tick_angle > TICKS_P_45_DEG * 1 ) return -fp_sin_low(rad,0);                    // then -sin(x)
else                                        return  fp_sin_low(rad,1);                    // then  cos(x)
}  
#endif

/////////////////////////////////////////// RMOTE INPUTS ///////////////////////////////////////////////////////////
// long ir_code,l_ir_code,ll_ir_code,ll_l_ir_code;
// short ir_count,l_ir_count,ll_ir_count;
short l_ir_count;
/////////////////////////////////////////// RS232 INPUTS ///////////////////////////////////////////////////////////
short rs232_rx;
volatile unsigned char rs232_rx_cnt,l_rs232_rx_cnt; // set by foreground to tell ap0c0 that it received something
/// what follows is the last method...
//==#define RS232_RX_BUF 32
//==unsigned char rs232_rx_buf[RS232_RX_BUF] = {0};
//==//unsigned char rs232_rx_idx=0;                          // <- always points on the NULL
//==//unsigned char l_rs232_rx_idx=0;                       // 
//==volatile unsigned char rs232_rx_clr=0;               // set by foreground to tell ap0c0 that it can clear the buffer
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
//29|   CATALOG STAR POS: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s                 COORD ID: 
//30|   IR CODE RECEIVED: DECODED STRING                                                                                       
//31|   IR CODE RECEIVED: 12345678 12345678 / 1234                RA SPEED:                RA STATE:  
//32| PREV CODE RECEIVED: 12345678 12345678                      DEC SPEED:               DEC STATE:  
//33| PREV PREV RECEIVED: 12345678 12345678                                                
//34|    BATTERY VOLTAGE: 12.0V                                 DEBUG PAGE:  
//35|          HISTOGRAM: 1234 1234 1234 1234 1234 1234 1234 1234  1234 1234 1234 1234 1234 1234 1234 1234      <-- once one reaches 65535, values are latched and printed 
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

#if(FULL_DISPLAY)
PROGMEM const char display_main[]={"\012\015\
          DATE/TIME: \012\015\
   CURRENT LOCATION: \012\015\
        POLAR ERROR: \012\015\
       SKY POSITION: \012\015\
 CORRECTED STAR POS: \012\015\
      TELESCOPE POS: \012\015\
       CATALOG STAR: \012\015\
   CATALOG STAR POS:                                                                  COORD ID:\012\015\
   IR CODE RECEIVED: \012\015\
   IR CODE RECEIVED:                                         RA SPEED:                RA STATE:\012\015\
 PREV CODE RECEIVED:                                        DEC SPEED:               DEC STATE:\012\015\
 PREV PREV RECEIVED:\012\015\
    BATTERY VOLTAGE:                                       DEBUG PAGE: \012\015\
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
#else
PROGMEM const char display_main[]={"short"};   // I'm too close to the 32K limit
#endif

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

#define   FMT_MASK_CONSOLE 0x01
#define   FMT_MASK_FMT     0xF0
#define   FMT_MASK_SIZE    0x0E    // valid sizes are 2 4 6 8 10 12 14 

// enum
#define   FMT_CONSOLE      0x01
#define   FMT_NO_VAL       0x00
#define   FMT_HEX          0x10
#define   FMT_DEC          0x20
#define   FMT_HMS          0x30
#define   FMT_NS           0x40
#define   FMT_EW           0x50
#define   FMT_RA           0x60
#define   FMT_STR          0x70
#define   FMT_ASC          0x80
#define   PGM_POLOLU       0x90
#define   PGM_DIPA328P     0xA0
#define   PGM_STR          0xB0
#define   FMT_FP           0xC0

#define   DD_FIELDS        0x60
volatile long dd_v[DD_FIELDS]; 
volatile long dd_p[DD_FIELDS];

PROGMEM const unsigned char dd_x[DD_FIELDS]=
//     0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    { 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94 , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94     // 0x00: DEBUG  0->15
    , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94 , 22 , 32 , 42 , 52 , 64 , 74 , 84 , 94     // 0x10: DEBUG 16->31
    , 22 , 27 , 32 , 37 , 42 , 47 , 52 , 57 , 64 , 69 , 74 , 79 , 84 , 89 , 94 , 99     // 0x20: Histogram 0->15
    , 39 , 39 , 39 , 72 , 97 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x30: RA structure values  [pos, pos_cor,pos_hw, speed, state
    , 22 , 22 , 22 , 72 , 97 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x40: DEC structure values [pos, pos_cor,pos_hw, speed, state
    , 57 , 57 , 57 , 35 , 22 , 39 , 57 , 31 , 22 , 22 , 22 , 72 , 97 ,  0 ,  0 ,  0     // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds, start pos dec,ra,ra
    }; //
PROGMEM const unsigned char dd_y[DD_FIELDS]=
    { 36 , 36 , 36 , 36 , 36 , 36 , 36 , 36 , 37 , 37 , 37 , 37 , 37 , 37 , 37 , 37     // 0x00: DEBUG  0->15
    , 38 , 38 , 38 , 38 , 38 , 38 , 38 , 38 , 39 , 39 , 39 , 39 , 39 , 39 , 39 , 39     // 0x10: DEBUG 16->31
    , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35 , 35     // 0x20: Histogram 0->15
    , 25 , 26 , 27 , 31 , 31 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x30: RA structure values   [pos, pos_cor,pos_hw, speed, state
    , 25 , 26 , 27 , 32 , 32 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0     // 0x40: DEC structure values  [pos, pos_cor,pos_hw, speed, state
    , 25 , 26 , 27 , 22 , 29 , 29 , 29 , 31 , 31 , 32 , 28 , 34 , 29 ,  0 ,  0 ,  0     // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds
    }; //
PROGMEM const unsigned char dd_f[DD_FIELDS]=
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18    // 0x00: DEBUG  0->15      all HEX 8 bytes
    ,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18    // 0x10: DEBUG 16->31      all HEX 8 bytes
    ,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14    // 0x20: Histogram 0->15   all HEX 4 bytes
    ,0x50,0x50,0x50,0x26,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00    // 0x30: RA structure values
    ,0x40,0x40,0x40,0x26,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00    // 0x40: DEC structure values 
    ,0x60,0x60,0x60,0x38,0x40,0x50,0x60,0x14,0x18,0x18,0x70,0x24,0x24,0x24,0x00,0x00    // 0x50: ra->pos2, ra->pos_cor2, ra->pos_hw2, seconds
    }; //                                               ^^^ was 0xB0 for Star name
// define the Start of each variable in the array
#define DDS_DEBUG         0x00
#define DDS_HISTO         0x20   // dd_v[DDS_HISTO]
#define DDS_RA            0x30   // ra  = (struct AXIS*)dd_v[DDS_RA];
#define DDS_DEC           0x40   // dec = (struct AXIS*)dd_v[DDS_DEC];
#define DDS_RA_POS2       0x50 
#define DDS_RA_POS2_COR   0x51 
#define DDS_RA_POS2_HW    0x52 
#define DDS_SECONDS       0x53  // dd_v[DDS_SECONDS] 
#define DDS_STAR_DEC_POS  0x54  // dd_v[DDS_STAR_DEC_POS]        // place to store the current star DEC (set by slave)
#define DDS_STAR_RA_POS   0x55  // dd_v[DDS_STAR_RA_POS]        // place to store the current star RA (set by slave)
#define DDS_STAR_RA_POS2  0x56  // dd_v[DDS_STAR_Ra_POS2]      // place to store the current star RA (set by slave)
#define DDS_IR_COUNT      0x57  // dd_v[DDS_IR_COUNT]
#define DDS_IR_CODE       0x58  // dd_v[DDS_IR_CODE]
#define DDS_IR_L_CODE     0x59  // dd_v[DDS_IR_L_CODE]
#define DDS_CUR_STAR      0x5A  // dd_v[DDS_CUR_STAR]
#define DDS_DEBUG_PAGE    0x5B  // dd_v[DDS_DEBUG_PAGE]
#define DDS_CUR_STAR_REQ  0x5C  // dd_v[DDS_CUR_STAR_REQ]   /// shown as COORD ID
//#define DDS_RX_IDX        0x5B  // dd_v[DDS_RX_IDX]

unsigned char dd_go(unsigned char task,char first)
{
// the twi logic became too complex ..using monkey method
static unsigned char p_chksum;
unsigned char          chksum=0;
short iii;
for ( iii=0 ; iii<STAR_NAME_LEN+CONSTEL_NAME_LEN ; iii++ ) chksum+=current_star_name[iii];

// static unsigned string_seq;
// if ( task == DDS_CUR_STAR ) string_seq++;
//if ( dd_p[task] != dd_v[task] || first || (string_seq==255))
if ( dd_p[task] != dd_v[task] || first || task == DDS_CUR_STAR)
   {
   short XXX = pgm_read_byte(&dd_x[task]);
   short YYY = pgm_read_byte(&dd_y[task]);
   short FMT = pgm_read_byte(&dd_f[task]);

   if ( (XXX == 0) && (YYY == 0) ) return 0; // found nothing to display

   if ( task == DDS_CUR_STAR )     
      {
//       if ( string_seq ==255 ) string_seq = 0;
      if ( p_chksum != chksum ) display_data((char*)rs232_tx_buf,XXX,YYY,0, (short) current_star_name,FMT);   // special cases strings 
      p_chksum = chksum;
      }
   else 
      display_data((char*)rs232_tx_buf,XXX,YYY,0,dd_v[task]                                          ,FMT);   // strings stored in FLASH

   dd_p[task] = dd_v[task];
   return 1;
   }

// a bit cpu inneficient, a bit exessive, but here for now
dd_v[DDS_RA_POS2]     = ra->pos;
dd_v[DDS_RA_POS2_COR] = ra->pos_cor;
dd_v[DDS_RA_POS2_HW]  = ra->pos_hw;
dd_v[DDS_DEBUG_PAGE]  = debug_page;

#ifdef AT_SLAVE
//   dd_v[DDS_STAR_DEC_POS] = pgm_read_dword( & pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+1]);
//   dd_v[DDS_STAR_RA_POS]  = pgm_read_dword( & pgm_stars_pos[dd_v[DDS_CUR_STAR]*2+0]);
   if ( dd_v[DDS_CUR_STAR_REQ] == STARS_COORD_TOTAL - 1 )   // when polar aligned, then return the polar error when the star requested is the last one (origin)
      {
      dd_v[DDS_STAR_RA_POS]  = polar_ra;
      dd_v[DDS_STAR_DEC_POS] = polar_dec;
      }
   else
      {
      dd_v[DDS_STAR_RA_POS]  = pgm_read_dword( & pgm_stars_pos[dd_v[DDS_CUR_STAR_REQ]*2+0]);
      dd_v[DDS_STAR_DEC_POS] = pgm_read_dword( & pgm_stars_pos[dd_v[DDS_CUR_STAR_REQ]*2+1]);
      }
   dd_v[DDS_STAR_RA_POS2] = dd_v[DDS_STAR_RA_POS];
   // current_star_name
      { 
      static unsigned char s_pointed=0,c_pointed,l_star_id,scan=0;
      unsigned char star_id,const_id;
      unsigned char star_jj,const_jj;


      star_id = dd_v[DDS_CUR_STAR_REQ];  
      if ( star_id != l_star_id ) 
         { // need to find the new match
         l_star_id = star_id;  
         scan = 0;
         for ( s_pointed=0 ; s_pointed < STAR_NAME_COUNT-1 ; s_pointed++ )
            {
            star_jj  = pgm_read_byte ( & pgm_stars_name_reduced[s_pointed*STAR_NAME_LEN+0] );  // Get Star ID of current selection
            const_id = pgm_read_byte ( & pgm_stars_name_reduced[s_pointed*STAR_NAME_LEN+1] );  // Get Constellation ID of current selection
            if ( star_jj==star_id ) break; // found it
            }
         for ( c_pointed=0 ; c_pointed < CONSTEL_NAME_COUNT-1  ; c_pointed++ )
            {
            const_jj = pgm_read_byte ( & pgm_const_name[c_pointed*CONSTEL_NAME_LEN+0] );  // Get Constellation ID of current selection
            if ( const_jj==const_id ) break; // found it
            }
dd_v[DDS_DEBUG + 0x0B] = const_jj;  // 0x19   =25
dd_v[DDS_DEBUG + 0x0C] = star_id;   // 0x11  
dd_v[DDS_DEBUG + 0x0D] = const_id;  // 0x19
dd_v[DDS_DEBUG + 0x0E] = star_jj;   // 0x11
         }
          
dd_v[DDS_DEBUG + 0x1B] = dd_v[DDS_CUR_STAR_REQ];   ////////////// 0x11
dd_v[DDS_DEBUG + 0x1C] = s_pointed;                      // 0x0 
dd_v[DDS_DEBUG + 0x1D] = c_pointed;                      // 0x1
      // update the strings
      scan++;
      if ( scan == (STAR_NAME_LEN - STAR_NAME_CODES) + (CONSTEL_NAME_LEN - CONSTEL_NAME_CODES) ) 
         {
         current_star_name[scan]=0;  // put the null
         scan=0;
         dd_v[DDS_CUR_STAR] = dd_v[DDS_CUR_STAR_REQ];  // Only once the string has been copied that dd_v[DDS_CUR_STAR] gets set -> thus upating the string on screen
         }
      if ( scan >= STAR_NAME_LEN - STAR_NAME_CODES )  
         {
         current_star_name[scan]= pgm_read_byte ( & pgm_const_name[ c_pointed*CONSTEL_NAME_LEN + scan - (STAR_NAME_LEN - STAR_NAME_CODES) + CONSTEL_NAME_CODES] );  
//       current_star_name[scan]= 'a'+scan;
         }
      else
         {
         current_star_name[scan]= pgm_read_byte ( & pgm_stars_name_reduced[ s_pointed*STAR_NAME_LEN + scan + STAR_NAME_CODES] );  
         }
      }
 
#else
//   dd_v[DDS_STAR_DEC_POS] = TICKS_P_45_DEG;  // temp for debug  set by twi (slave)
//   dd_v[DDS_STAR_RA_POS]  = TICKS_P_45_DEG;  // temp for debug  set by twi (slave)
//   dd_v[DDS_STAR_RA_POS2] = TICKS_P_45_DEG;  // temp for debug  set by twi (slave)
#endif

return 0; // found nothing to display
} // Function : unsigned char dd_go(unsigned char task,char first)


////////////////////////////////////////// DISPLAY SECTION //////////////////////////////////////////
// PRINT FORMATS:
// LAT : (N)-90 59'59.00"
// LON : (W)-179 59'59.00"
// RA  : 23h59m59.000s
void display_next(void);

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
   else       buf[0] = ' ';  // was buf[0] = '+';
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
void display_data(char *str,short xxx,short yyy,const char *pgm_str,unsigned long value,unsigned char size)
{
short iii=0;
unsigned long  jjj=0;
short svalue=(short)value;
unsigned char fmt;
char  ccc;

if ( (size & FMT_MASK_CONSOLE) !=0 )  // a bit ugly, but it saves FLASH space
   {                                // combine the format, the size and the to_console bit
   while (console_go) display_next(); /* wait for ready */
   console_go = 1;
   }
fmt  = size & FMT_MASK_FMT;       // a bit ugly, but it saves FLASH space
size = size & FMT_MASK_SIZE;      // combine the format, the size and the to_console bit

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

   if ( (prob!=0) && (console_go==0)) display_data((char*)console_buf,0,20,pgm_display_bug,prob,FMT_HEX+8);

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

   if ( (prob!=0) && (console_go==0)) display_data((char*)console_buf,0,20,pgm_display_bug_ra,prob,FMT_HEX+8);
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
   // If pgm_str is not 0, then the above while() at the start of the function will print the string before any other formatting
   // this is useful to display    STRING : DATA
   }
else if ( fmt == FMT_ASC )  // ASCII
   {
   str[iii++] = '[';  
   str[iii++] = value;  
   str[iii++] = ']';  
   str[iii]   = 0;  
   }
else if ( fmt == FMT_FP )  // Fixed point  fron -1.00000 to 1.000000
   {
   /* temporary until I'm confidant that it does work */ p08x(&str[iii],value);
   /* temporary until I'm confidant that it does work */ iii+=8;
   /* temporary until I'm confidant that it does work */ str[iii++] = ' ';
   /* temporary until I'm confidant that it does work */ str[iii++] = ' ';
   if ( value & 0x80000000 )  // negative
      {
      str[iii++] = '-';
      value = -value;
      }
   else str[iii++] = '0';
   str[iii++] = '.';
   value = value>>3; 

   for ( ; size > 0 ;size--)
      {   
      value *= 10; 
      jjj = value >> 28;   // jjj   is unsigned... so the upper bits will remain 0
      str[iii++] = '0'+jjj;
      value &= 0x0FFFFFFF;
      }
   str[iii++] = 0;
   }
}


PROGMEM const char pgm_recorded_pos     []="Rec pos: ";
PROGMEM const char pgm_recorded_pos_ra  []="RA :";
PROGMEM const char pgm_recorded_pos_dec []="DEC:";
PROGMEM const char pgm_recorded_pos_ref []="ref:";
#ifdef AT_SLAVE
PROGMEM const char pgm_polar_case       []="Case #:";
PROGMEM const char pgm_polar_hour       []="Hour   :";
PROGMEM const char pgm_polar_dec        []="Declin :";
PROGMEM const char pgm_polar_star       []="Star #:";
PROGMEM const char pgm_polar_error      []="Error :";
PROGMEM const char pgm_polar_sum        []="Sum = ";
PROGMEM const char pgm_polar_pass       []="Pass #:";


// set the XYZ of a VECTOR from the RA and DEC values provided
// RA and DEC are in TICK units
void set_vector(VECTOR *VVV,unsigned long *RA,unsigned long *DEC)
{
long cos_dec;
VVV->z  = fp_sin(*DEC);
cos_dec = fp_cos(*DEC);
VVV->x  = fp_mult(fp_cos(*RA),cos_dec);    // Other function uses subset of the math here, if
VVV->y  = fp_mult(fp_sin(*RA),cos_dec);    // any of the equation here changes, pls search for set_vector() in comments and change s/w 
}


// Generate a rotation matrix R from the polar RA and DEC
PROGMEM const char pgm_polar_matrix     []="Polar Matrix: ";
PROGMEM const char pgm_polar_matrix_d   []="Data: ";
void generate_polar_matrix(MATRIX *R,unsigned long *pol_ra,unsigned long *pol_dec,unsigned long *shift_ra,unsigned char PRINT)
{
long cos_pol_ra   = fp_cos(*pol_ra);
long sin_pol_ra   =-fp_sin(*pol_ra);   // reversed direction . . .
long cos_pol_dec  = fp_cos(*pol_dec);
long sin_pol_dec  =-fp_sin(*pol_dec);  // reversed direction . . .
long cos_shift_ra = fp_cos(*shift_ra);
long sin_shift_ra =-fp_sin(*shift_ra); // reversed direction . . .

// long tmp      = 0x7FFFFFFF - cos_dec;     //  1 - COS(DEC)

// Dont worry, even I dont understand this...
R->m11 =  fp_mult(sin_pol_ra, sin_shift_ra) + fp_mult(fp_mult(cos_pol_ra,cos_pol_dec),cos_shift_ra) ;
R->m21 =  fp_mult(cos_pol_ra, sin_shift_ra) - fp_mult(fp_mult(sin_pol_ra,cos_pol_dec),cos_shift_ra) ;
R->m31 = -fp_mult(sin_pol_dec,cos_shift_ra)                                                         ;

R->m12 =  fp_mult(sin_pol_ra,cos_shift_ra) - fp_mult(fp_mult(cos_pol_ra,cos_pol_dec),sin_shift_ra) ;
R->m22 =  fp_mult(cos_pol_ra,cos_shift_ra) + fp_mult(fp_mult(sin_pol_ra,cos_pol_dec),sin_shift_ra) ;
R->m32 =  fp_mult(sin_pol_dec,sin_shift_ra)                                                        ;

R->m13 =  fp_mult(cos_pol_ra,sin_pol_dec)                                                          ;
R->m23 = -fp_mult(sin_pol_ra,sin_pol_dec)                                                          ;  
R->m33 =  cos_pol_dec                                                                              ;

if ( PRINT !=0  )
   {
   display_data((char*)console_buf,0,20,pgm_polar_matrix   ,0      ,FMT_NO_VAL + FMT_CONSOLE + 8);

   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m11 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m21 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m31 ,FMT_FP + FMT_CONSOLE + 8);
   
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m12 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m22 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m32 ,FMT_FP + FMT_CONSOLE + 8);
   
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m13 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m23 ,FMT_FP + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_polar_matrix_d ,R->m33 ,FMT_FP + FMT_CONSOLE + 8);
   }
}

#if TEST_POLAR    
PROGMEM const char pgm_polar_line       []="_________";
PROGMEM const char pgm_polar_x      []="X :";
PROGMEM const char pgm_polar_y      []="Y :";
PROGMEM const char pgm_polar_z      []="Z :";

void test_polar(unsigned long hour,unsigned long deg,unsigned long shift)
{
unsigned long test_ra;
unsigned long test_dec;
long sum;
VECTOR star;

generate_polar_matrix(&PoleMatrix,&hour, &deg, &shift,0);

while (console_go) display_next(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_polar_line,0        ,FMT_NO_VAL + FMT_CONSOLE + 8);
while (console_go) display_next(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_polar_line,0        ,FMT_NO_VAL + FMT_CONSOLE + 8);
while (console_go) display_next(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_polar_hour,hour     ,FMT_RA     + FMT_CONSOLE + 8);
while (console_go) display_next(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_polar_dec ,deg      ,FMT_NS     + FMT_CONSOLE + 8);

for ( test_dec = 0 ; test_dec <= 90*TICKS_P_DEG ; test_dec += 45*TICKS_P_DEG )
   {
   for ( test_ra = 0 ; test_ra < 24*TICKS_P_HOUR ; test_ra += 3*TICKS_P_HOUR )
      {
      display_data((char*)console_buf,0,20,pgm_polar_line,0        ,FMT_NO_VAL + FMT_CONSOLE + 8);
      //to_console(pgm_polar_line,test_ra,FMT_NO_VAL,8);
      set_vector(&star, &test_ra, &test_dec);

      display_data((char*)console_buf,0,20,pgm_polar_hour,test_ra  ,FMT_RA + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_dec ,test_dec ,FMT_NS + FMT_CONSOLE + 8);
      //to_console(pgm_polar_hour,test_ra,FMT_RA,8);
      //to_console(pgm_polar_dec,test_dec,FMT_NS,8);

      display_data((char*)console_buf,0,20,pgm_polar_x ,star.x ,FMT_FP + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_y ,star.y ,FMT_FP + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_z ,star.z ,FMT_FP + FMT_CONSOLE + 8);
      //to_console(pgm_polar_x,star.x,FMT_FP,8);
      //to_console(pgm_polar_y,star.y,FMT_FP,8);
      //to_console(pgm_polar_z,star.z,FMT_FP,8);
      apply_polar_correction(&PoleMatrix,&star);
      display_data((char*)console_buf,0,20,pgm_polar_x ,star.x ,FMT_FP + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_y ,star.y ,FMT_FP + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_z ,star.z ,FMT_FP + FMT_CONSOLE + 8);
      //to_console(pgm_polar_x,star.x,FMT_FP,8);
      //to_console(pgm_polar_y,star.y,FMT_FP,8);
      //to_console(pgm_polar_z,star.z,FMT_FP,8);


      sum = fp_mult(star.x,star.x) + fp_mult(star.y,star.y) + fp_mult(star.z,star.z);
      display_data((char*)console_buf,0,20,pgm_polar_sum ,sum ,FMT_FP + FMT_CONSOLE + 8);

      if ( test_dec == 90*TICKS_P_DEG ) return;
      }
   }
}
#endif
                                                                                                                                                                                                                              
// find our polar error based on the corrected star positions
// Note:
// there are two things to correct
// 1- The polar error 
// 2- The RA timeshift
// To do so, I will first try to find the polar adjustment that brings the Z error to the minimum
//     then, I will adjust RA timeshift to find the lowest total error
void do_polar(void)
{
unsigned long next_ra;                          // start midpoint   scan +/- 12 steps   // 2073600000   which is 12h / 180 deg
         long ra_span;                          // 172800000
unsigned long next_dec;                         // start midpoint, scan 10 degrees              
         long dec_span;                        
unsigned long best_ra,best_dec;
short    dec_idx,ra_idx,span_idx;               // scan indexes
unsigned long best_error;
unsigned short star_idx,ref;
unsigned long shift,dec,ra,deg;
unsigned long error_sum,error;
unsigned char pass;
VECTOR star,real_star;

use_polar=0;
motor_disable = 1; // get all the CPU time we can
align_state=11;

for ( pass = 1 ; pass <=2 ; pass ++ )
   {
   best_ra  = next_ra = 12 * TICKS_P_HOUR;
   ra_span  =                TICKS_P_HOUR;
   best_dec = next_dec = 5 * TICKS_P_DEG; 
   dec_span =                TICKS_P_DEG;
   best_error=0x7FFFFFFF;

   for ( span_idx = 1 ; span_idx < 6 ; span_idx++ )  // do 6 passes   >> 3 each time to divide the span
      {
      for ( dec_idx = -5 ; ((pass==1)&&(dec_idx <= 5)) || ((pass==2)&&(dec_idx <= -5)) ; dec_idx++)
         {
         deg = next_dec + dec_idx * dec_span;
         for ( ra_idx = -12 ; ra_idx <= 12 ; ra_idx++)
            {
            shift = next_ra + ra_idx * ra_span;
            if ( pass == 1 ) 
               generate_polar_matrix(& PoleMatrix,&shift,&deg, &shift, 0);
            else  // best ra and best dec found already
               generate_polar_matrix(& PoleMatrix,&polar_ra,&polar_dec, &shift, 0);
 
            error_sum=0;
     
           
            for ( star_idx=error=0 ; star_idx<10 ; star_idx++ )
               { // here the magic happens... for each registered star, calculate the error: delta_x^2 + delta_y^2 + delta_z^2
               ref = saved[star_idx+10].ref_star;
               if ( ref !=0 )  // for every star with a corrected position
                  {
                  display_next();
                  ra  = pgm_read_dword( & pgm_stars_pos[ref*2+0]);
                  dec = pgm_read_dword( & pgm_stars_pos[ref*2+1]);
                  set_vector(     &star, &ra, &dec);
                  set_vector(&real_star, &saved[star_idx+10].ra, &saved[star_idx+10].dec);
                  apply_polar_correction(&PoleMatrix,&star);
      
                  if ( pass == 2 )  // calculate full error only in second pass
                     { 
                     error = star.x - real_star.x;
                     error = fp_mult(error,error);
                     ra = error_sum + (error);                      // use temp label to bake sure error only goes up
                     if (ra > error_sum ) error_sum = ra;
      
                     error = star.y - real_star.y;
                     error = fp_mult(error,error);
                     ra = error_sum + (error);                      // use temp label to bake sure error only goes up
                     if (ra > error_sum ) error_sum = ra;
                     }
      
                  error = star.z - real_star.z;
                  error = fp_mult(error,error);
                  ra = error_sum + (error);                      // use temp label to bake sure error only goes up
                  if (ra > error_sum ) error_sum = ra;
                  }
               } 
      
//-    display_data((char*)console_buf,0,20,pgm_polar_sum ,error_sum,FMT_FP + FMT_CONSOLE + 8); 
            if ( error_sum < best_error )
               {
               best_error = error_sum;
               best_ra    = shift;
               best_dec   = deg;
               }
            }
         }
      next_ra  = best_ra;
      next_dec = best_dec;
      if ( pass==1)
         {
         polar_ra  = best_ra;
         polar_dec = best_dec;
         }

      display_data((char*)console_buf,0,20,pgm_polar_pass,span_idx    ,FMT_DEC + FMT_CONSOLE + 4);
      display_data((char*)console_buf,0,20,pgm_polar_error,best_error ,FMT_HEX + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_hour,polar_ra    ,FMT_RA  + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_dec ,polar_dec   ,FMT_NS  + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_hour,best_ra     ,FMT_RA  + FMT_CONSOLE + 8);
   
      display_data((char*)console_buf,0,20,pgm_polar_hour,ra_span     ,FMT_RA  + FMT_CONSOLE + 8);
      display_data((char*)console_buf,0,20,pgm_polar_dec ,dec_span    ,FMT_NS  + FMT_CONSOLE + 8);
   
      dec_span = dec_span >> 3; // reduce span to slowly converge towards the solution
      ra_span  = ra_span  >> 3; // reduce span to slowly converge towards the solution
   /*
   Pass #:+011                 
   Error :005160A2             
   Hour   :22h10m41.71s        
   Declin :(N) 01°37'11.6"     
   Hour   :00h00m00.00s        
   Declin :(N) 00°00'00.0"    
   */
   
      }
   }
generate_polar_matrix(& PoleMatrix,&polar_ra, &polar_dec, &shift,1);
   
use_polar=1;
align_state=12;
}

  
// use rotation matrix to rotate the star...
void apply_polar_correction(MATRIX *R,VECTOR *S)
{
unsigned long x,y,z;

x = fp_mult(R->m11,S->x) + fp_mult(R->m21,S->y) + fp_mult(R->m31,S->z);
y = fp_mult(R->m12,S->x) + fp_mult(R->m22,S->y) + fp_mult(R->m32,S->z);
z = fp_mult(R->m13,S->x) + fp_mult(R->m23,S->y) + fp_mult(R->m33,S->z);
S->x = x;
S->y = y;
S->z = z;
}


#endif // AT_SLAVE
/*
Error  : 
Hour   : 
Declin : 
*/
     
#ifdef AT_MASTER
#define MOSAIC_MULT 2

void mosaic(void)
{
static char last_sec;
char        cur_sec;
if ( debug_page==7 ) dd_v[DDS_DEBUG + 0x18]=ra->pos_earth;

if ( mosaic_seconds==-1 ) return; // mosaic not active

cur_sec = dd_v[DDS_SECONDS]&0x7F;
if ( moving || goto_cmd ) return;


if ( mosaic_seconds==-2 ) //cancel
   {
   ra->pos_target  = mosaic_base_ra;  
   dec->pos_target = mosaic_base_dec;
   goto_cmd = 1;
   mosaic_seconds = -1;
   return;
   }

if ( cur_sec != last_sec ) mosaic_seconds++;  // for now, hardcode to 35 seconds (assume 30 seconds photos)
last_sec = cur_sec;


if ( mosaic_seconds > mosaic_dt )  // next move...
   {
   long sss,rrr,ddd,uuu;
   mosaic_seconds = 1;

   sss = pgm_read_dword(&mosaic_span[(short)mosaic_span_idx]);
   if ( mosaic_dt == 1 ) sss = sss >> MOSAIC_MULT;
   rrr = mosaic_seq_ra   * sss;
   ddd = mosaic_seq_de   * sss;
   uuu = mosaic_nb_tiles * sss;
   uuu = uuu >> 1 ;

   if ( mosaic_seq_ra > mosaic_nb_tiles ) // we are done...
      {
      rrr = ddd = uuu = 0; // goto start pos
      mosaic_seconds = -1;
      }
 
   ra->pos_target  = mosaic_base_ra  + rrr - uuu; // reversed RA so to stay in the same sky area
   dec->pos_target = mosaic_base_dec - ddd + uuu;
   //standby_goto = 1;   // ask to fix the error
   //while ( standby_goto > 0 ) display_next();   // wait until we are corrected
   //standby_goto = 0;
   goto_cmd = 1;

   // setup the next position...
   mosaic_seq_de++;
   if ( mosaic_seq_de > mosaic_nb_tiles )
      {
      mosaic_seq_de = 0;
      mosaic_seq_ra++;
      }   

//   if ( debug_page==7 ) 
//      {
//      dd_v[DDS_DEBUG + 0x02]=sss;
//      dd_v[DDS_DEBUG + 0x03]=rrr;
//      dd_v[DDS_DEBUG + 0x04]=ddd;
//      dd_v[DDS_DEBUG + 0x05]=mosaic_seq_ra;
//      dd_v[DDS_DEBUG + 0x06]=ra->pos_target;
//      dd_v[DDS_DEBUG + 0x07]=mosaic_seq_de;
//      dd_v[DDS_DEBUG + 0x08]=dec->pos_target;
//      dd_v[DDS_DEBUG + 0x09]=uuu;
//      dd_v[DDS_DEBUG + 0x0A]=mosaic_nb_tiles;
//      }
   }
}
 
#define PROSCAN_VCR1_0      0x021CFE30
#define PROSCAN_VCR1_1      0x021CEE31
#define PROSCAN_VCR1_2      0x021CDE32
#define PROSCAN_VCR1_3      0x021CCE33
#define PROSCAN_VCR1_4      0x021CBE34
#define PROSCAN_VCR1_5      0x021CAE35
#define PROSCAN_VCR1_6      0x021C9E36
#define PROSCAN_VCR1_7      0x021C8E37
#define PROSCAN_VCR1_8      0x021C7E38
#define PROSCAN_VCR1_9      0x021C6E39
#define PROSCAN_VCR1_NORTH  0x021A6E59
#define PROSCAN_VCR1_SOUTH  0x021A7E58
#define PROSCAN_VCR1_WEST   0x021A8E57
#define PROSCAN_VCR1_EAST   0x021A9E56
#define PROSCAN_VCR1_OK     0x0210BEF4
#define PROSCAN_VCR1_STOP   0x021E0E1F
#define PROSCAN_VCR1_PLAY   0x021EAE15
#define PROSCAN_VCR1_RECORD 0x021E8E17
#define PROSCAN_VCR1_CH_P   0x021D2E2D
#define PROSCAN_VCR1_CH_M   0x021D3E2C
#define PROSCAN_VCR1_VOL_P  0x023D0C2F
#define PROSCAN_VCR1_VOL_M  0x023D1C2E
#define PROSCAN_VCR1_FWD    0x021E3E1C
#define PROSCAN_VCR1_REW    0x021E2E1D
#define PROSCAN_VCR1_SEARCH 0x021ACE53
#define PROSCAN_VCR1_GOBACK 0x021D8E27
#define PROSCAN_VCR1_INPUT  0x021B8E47
#define PROSCAN_VCR1_ANTENA 0x021FAE05
#define PROSCAN_VCR1_CLEAR  0x021F9E06
#define PROSCAN_VCR1_GUIDE  0x021E5E1A
#define PROSCAN_VCR1_INFO   0x021C3E3C
#define PROSCAN_VCR1_POWER  0x021D5E2A
#define PROSCAN_VCR1_TRAK_P 0x021F4E0B
#define PROSCAN_VCR1_TRAK_M 0x021F5E0A
#define PROSCAN_VCR1_MUTE   0x020C0F3F
#define PROSCAN_VCR1_SPEED  0x021B9E46

#define IDX_VCR1_0          0
#define IDX_VCR1_1          1
#define IDX_VCR1_2          2
#define IDX_VCR1_3          3
#define IDX_VCR1_4          4
#define IDX_VCR1_5          5
#define IDX_VCR1_6          6
#define IDX_VCR1_7          7
#define IDX_VCR1_8          8
#define IDX_VCR1_9          9
#define IDX_VCR1_NORTH      10
#define IDX_VCR1_SOUTH      11
#define IDX_VCR1_WEST       12
#define IDX_VCR1_EAST       13
#define IDX_VCR1_OK         14
#define IDX_VCR1_STOP       15
#define IDX_VCR1_PLAY       16
#define IDX_VCR1_RECORD     17
#define IDX_VCR1_CH_P       18
#define IDX_VCR1_CH_M       19
#define IDX_VCR1_VOL_P      20
#define IDX_VCR1_VOL_M      21
#define IDX_VCR1_FWD        22
#define IDX_VCR1_REW        23
#define IDX_VCR1_SEARCH     24
#define IDX_VCR1_GOBACK     25
#define IDX_VCR1_INPUT      26
#define IDX_VCR1_ANTENA     27
#define IDX_VCR1_CLEAR      28
#define IDX_VCR1_GUIDE      29
#define IDX_VCR1_INFO       30
#define IDX_VCR1_POWER      31
#define IDX_VCR1_TRAK_P     32
#define IDX_VCR1_TRAK_M     33
#define IDX_VCR1_MUTE       34
#define IDX_VCR1_SPEED      35

/*
- the following IR commands clear after 3 seconds  ... The IR sends values to the RS232 buffer same as keyboard  3sec is 
- Store generic position 1-10  (complex command sequence)           r.X                       [RECORD  + INPUT + X]  // 10 slots avail for generic pos
- goto  generic position 1-10  (complex command sequence)           p.X                       [PLAY    + INPUT + X]
- Store corrected star position 1-10  (complex command sequence)    r!X                       [RECORD  + ANTENA + X]  //  10 slots for star correction
- goto  corrected star position 1-10  (complex command sequence)    p!X                       [PLAY    + ANTENA + X]  //     each slot also contains the index of the star (reference)
- goto  directory start position 1-??  (complex command sequence)   pXXX                      [PLAY    + X + X + X]  // The first play sets the reference RA
- Run a special command                                             gXXX                      [GUIDE   + X + X + X]  // use for : Mosaic and Fake align
- Go to Catalog star                                                *                         [GO BACK] 
- Select next/previous star from catalog                            < / >                     [FWD/REV] 
- clear all generic recorded positions                              <del> .                   [CLEAR   + INPUT]
- clear all stars corrected positions                               <del> !                   [CLEAR   + ANTENA]
- clear histogram values                                            <del> ?                   [CLEAR   + INFO]
= move back and forth to remove friction                            <del> *                   [CLEAR   + GO BACK]
- Redraw screen                                                     <del> <enter>             [CLEAR   + OK] 
- Reset input command                                               <enter> g                 [CLEAR   + GUIDE]
- goto ra position                                                  [EW]123 45 67 /           [VOL     + 123 45 67 + SEARCH]
- goto dec position                                                 [NS]123 45 67 /           [CH      + 12 34 56 78 + SEARCH]
- calculate polar error based on corrected star position            ! /                       [ANTENA  + SEARCH]
- Code to toggle motor on/off                                       ~  (` no shift required)  [POWER]
- Slew                                                              [esc][O[ABCD]             [UP/DOWN/LEFT/RIGHT]
- Slew stop                                                         <enter> / s               [OK/STOP]
- Start/stop tracking                                               + / -                     [TRACKING +/-] 
- add a command that displays at the console : time, position, star name (corrected star position)
*/

PROGMEM unsigned long PROSCANs[]= {   // The order is important   3                    4                    5                    6                    7
PROSCAN_VCR1_0      ,PROSCAN_VCR1_1      ,PROSCAN_VCR1_2      ,PROSCAN_VCR1_3      ,PROSCAN_VCR1_4      ,PROSCAN_VCR1_5      ,PROSCAN_VCR1_6      ,PROSCAN_VCR1_7      ,// 0
PROSCAN_VCR1_8      ,PROSCAN_VCR1_9      ,PROSCAN_VCR1_NORTH  ,PROSCAN_VCR1_SOUTH  ,PROSCAN_VCR1_WEST   ,PROSCAN_VCR1_EAST   ,PROSCAN_VCR1_OK     ,PROSCAN_VCR1_STOP   ,// 8
PROSCAN_VCR1_PLAY   ,PROSCAN_VCR1_RECORD ,PROSCAN_VCR1_CH_P   ,PROSCAN_VCR1_CH_M   ,PROSCAN_VCR1_VOL_P  ,PROSCAN_VCR1_VOL_M  ,PROSCAN_VCR1_FWD    ,PROSCAN_VCR1_REW    ,// 16
PROSCAN_VCR1_SEARCH ,PROSCAN_VCR1_GOBACK ,PROSCAN_VCR1_INPUT  ,PROSCAN_VCR1_ANTENA ,PROSCAN_VCR1_CLEAR  ,PROSCAN_VCR1_GUIDE  ,PROSCAN_VCR1_INFO   ,PROSCAN_VCR1_POWER  ,// 24
PROSCAN_VCR1_TRAK_P ,PROSCAN_VCR1_TRAK_M ,PROSCAN_VCR1_MUTE   ,PROSCAN_VCR1_SPEED                                                                                             // 32
                                  };
PROGMEM short         RS232EQVs[]= {  // The order is important, each rs232 character is assigned a PROSCAN equivalent
'0'                 ,'1'                 ,'2'                 ,'3'                 ,'4'                 ,'5'                 ,'6'                 ,'7'                 , 
'8'                 ,'9'                 ,0x5B41              ,0x5B42              ,0x5B44              ,0x5B43              ,13                  ,'s'                 ,
'p'                 ,'r'                 ,'i'                 ,'m'                 ,'k'                 ,'j'                 ,'>'                 ,'<'                 , 
'/'                 ,'*'                 ,'.'                 ,'!'                 ,0x7E                ,'g'                 ,'?'                 ,0x60                ,
'+'                 ,'-'                 ,'%'                 ,'$'
                                  };  
char idx_code,next_input;

char set_next_input(char idx_code)   // determine the type of input ; this valus is used strait in the cmd_state machine
{
if ( idx_code <= IDX_VCR1_9                       )           return 0;  // 0-9
if ( idx_code == IDX_VCR1_PLAY                    )           return 1;  // PLAY   / 'p'
if ( idx_code == IDX_VCR1_GUIDE                   )           return 1;  // GUIDE  / 'g'
if ( idx_code == IDX_VCR1_RECORD                  )           return 2;  // RECORD / 'r'
if ( idx_code == IDX_VCR1_CLEAR                   )           return 3;  // CLEAR  / 'delete'
if ( idx_code == IDX_VCR1_INPUT                   )           return 4;  // INPUT  / '.'
if ( idx_code == IDX_VCR1_ANTENA                  )           return 5;  // ANTENA / '!'
if ( idx_code >= IDX_VCR1_CH_P && idx_code <=IDX_VCR1_VOL_M ) return 6;  // CH_P   / 'i'      CH_M   / 'm'    OL_P  / 'k'    VOL_M  / 'j'    
if ( idx_code == IDX_VCR1_SEARCH                  )           return 6;  // SEARCH / 'p'
else                                                          return -1; // PLAY   / 'p'
}
#endif


char standby_goto;

void goto_pgm_pos(short pos)
{
if ( pos >= STARS_COORD_TOTAL ) return;  // out of bound . . .
if ( ! ( moving || goto_cmd ) )
   {
// was:   ra->pos_target  = pgm_read_dword(&pgm_stars_pos[pos*2+0]);
// was:   dec->pos_target = pgm_read_dword(&pgm_stars_pos[pos*2+1]);
   ra->pos_target  = dd_v[DDS_STAR_RA_POS];   // Set by slave
   dec->pos_target = dd_v[DDS_STAR_DEC_POS];  // Set by slave
   standby_goto = 1;   // ask to fix the error
   while ( standby_goto > 0 ) display_next();   // wait until we are corrected
   standby_goto = 0;
   goto_cmd = 1;
   dd_v[DDS_CUR_STAR_REQ] = pos;
   }
}

void goto_pos(short pos)
{
if ( pos > NB_SAVED ) return;   // out of bound . . .
if ( ! ( moving || goto_cmd ) )
   {
   ra->pos_target  = saved[pos].ra;
   dec->pos_target = saved[pos].dec;
   standby_goto = 1;   // ask to fix the error
   while ( standby_goto > 0 ) display_next();   // wait until we are corrected
   standby_goto = 0;
   goto_cmd = 1;
   }
}


void record_pos(short pos)
{
twi_hold = 1;         // Tell foreground that if it interrupts us, it must not drive the twi position because we are reading them...
if ( pos > NB_SAVED ) return;   // out of bound . . .
saved[pos].ra  = twi_pos[0]; // ra->pos;
saved[pos].dec = twi_pos[1]; // dec->pos;
saved[pos].ref_star = dd_v[DDS_CUR_STAR];
twi_hold = 0;         // Tell foreground that if it interrupts us, it must not drive the twi position because we are reading them...

if ( pos < 10 )
   {
   display_data((char*)console_buf,0,20,pgm_recorded_pos   ,pos   ,FMT_HEX + FMT_CONSOLE + 8);
   }
else
   {
   display_data((char*)console_buf,0,20,pgm_recorded_pos    ,pos   ,FMT_HEX + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_recorded_pos_ra ,saved[pos].ra        ,FMT_HEX + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_recorded_pos_dec,saved[pos].dec       ,FMT_HEX + FMT_CONSOLE + 8);
   display_data((char*)console_buf,0,20,pgm_recorded_pos_ref,saved[pos].ref_star  ,FMT_HEX + FMT_CONSOLE + 8);
   }
}




#ifdef AT_MASTER
unsigned char cmd_val[10],cmd_val_idx;  //store the last byte of the last IR codes received

// using a table state machine to shorten the s/w code
// I'm a bit supprised, I think this little table will acomplish something very complex !!
//    INPUTS:                 NUM  PLA  REC  CLR  INP  ANT  SEARCH        STATE:
PROGMEM char cmd_states[] = {   0,   1,   6,   9,   0,  11,  10        //   0 ready tp process a new command
                            ,   2, 110,   0,   0,   4,   5,   0        //   1 PLAY X  / PLAY INPUT / PLAY ANTENA / PLAY PLAY
                            ,   3,   0,   0,   0,   0,   0,   0        //   2 PLAY X X
                            , 100,   0,   0,   0,   0,   0,   0        //   3 PLAY X X X
                            , 101,   0,   0,   0,   0,   0,   0        //   4 PLAY INPUT X
                            , 102,   0,   0,   0,   0,   0,   0        //   5 PLAY ANTENA X
                            ,   0,   0,   0,   0,   7,   8,   0        //   6 RECORD INPUT / RECORD ANTENA
                            , 103,   0,   0,   0,   0,   0,   0        //   7 RECORD INPUT X
                            , 104,   0,   0,   0,   0,   0,   0        //   8 RECORD ANTENA X
                            ,   0,   0,   0,   0, 105, 106, 107        //   9 CLEAR ? INPUT / ANTENA / INFO / 
                            ,  10,   0,   0,   0,   0,   0, 108        //  10 GOTO MANUAL POSITION ?
                            ,   0,   0,   0,   0,   0,   0, 109        //  11 ANTENA SEARCH ?
                            };
#endif
short ddll=0;

void display_next_bg(void) 
{
#ifdef AT_SLAVE
twitt();
if ( AP0_DISPLAY == 0 ) display_next();  // if not printing from AP0, then print here
#elif  AT_MASTER
char code_idx=-1; // lets work with the code's index  // wether it's IR or rs232
short iii;

dd_v[DDS_DEBUG_PAGE] = debug_page;

twitt();
mosaic(); // mosaic active

// Torque control
if ( motor_disable ) { torq = 0; dcay = 3; } // start with low torq when motor off   and set the decay to 0% which seems to be the less noizy and the best one
//else if ( earth_tracking && (torq < 2) ) torq = 2; // if we are tracking, then set the torq to a minimum of 50% 

IF_NOT_CONDITION_PORT_BIT(effectiv_torq & 0x01 , PORTB , DO_PB1_TORQA1  | DO_PB3_TORQB1  );
IF_NOT_CONDITION_PORT_BIT(effectiv_torq & 0x02 , PORTB , DO_PB2_TORQA2  | DO_PB4_TORQB2  );
IF_NOT_CONDITION_PORT_BIT(dcay & 0x01 , PORTD , DO_PD5_DECAYA1 | DO_PD7_DECAYB1 );
IF_NOT_CONDITION_PORT_BIT(dcay & 0x02 , PORTD , DO_PD6_DECAYA2                  );
IF_NOT_CONDITION_PORT_BIT(dcay & 0x02 , PORTB , DO_PB5_DECAYB2                  );

if ( AP0_DISPLAY == 0 ) display_next();  // if not printing from AP0, then print here

//dd_v[DDS_DEBUG + 0x0C] = 0x5000 + torq;
if ( debug_page==0) dd_v[DDS_DEBUG + 0x07] = cmd_state;

///// Process IR commands
if ( l_ir_count != dd_v[DDS_IR_COUNT])
   {
   long code = dd_v[DDS_IR_CODE];
   long lcode;
   l_ir_count = dd_v[DDS_IR_COUNT]; // tell SP0 that he can go on
   for( iii=0 ; (iii < sizeof(PROSCANs)/4) && (code_idx<0) ; iii++ )
      {
      lcode= pgm_read_dword(&PROSCANs[iii]);
      if (code == lcode )  code_idx = iii;  // found a valid code
      }
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x04] = code;
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x05] = code_idx;
   }
else if ( l_rs232_rx_cnt != rs232_rx_cnt )  // new rs232 input
   {
   short scode;
   l_rs232_rx_cnt = rs232_rx_cnt;
   for( iii=0 ; (iii < sizeof(RS232EQVs)/2) && (code_idx<0) ; iii++ )
      {
      scode = pgm_read_word (&RS232EQVs[iii]);
      if (rs232_rx == scode )  code_idx = iii;  // found a valid code
      }
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x04] = rs232_rx;
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x05] = code_idx + rs232_rx_cnt*0x10000;
   }

///// Process Keyboard commands
if ( code_idx >= 0   ) // received a valid input from IR or RS232
   {
   unsigned char jjj=0;

   next_input = set_next_input(code_idx);
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x06] = next_input;

   if ( next_input == 0 ) jjj = code_idx;  // (This is the value) 0-9

   if ( next_input >=0 ) 
      {
      if ( cmd_state==0 ) cmd_val_idx=0;  // reset captured data
      cmd_state = pgm_read_byte(&cmd_states[cmd_state*7 + next_input]);  // go to next state based on input type
//    if ( cmd_val_idx<10) cmd_val[cmd_val_idx++] = jjj;                 // store the last code (from which we can extract a number)
      if ( cmd_val_idx<10) cmd_val[cmd_val_idx++] = code_idx;            // store the last code (from which we can extract a number) and from which we can differentiate between commands
      }
   else 
      {
      if ( cmd_state== 9 ) cmd_state=107;   // state 9 will  process anything  !
      else                 cmd_state=0;     // unknown command
      }
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x07] = cmd_val[cmd_val_idx-3]*100 + cmd_val[cmd_val_idx-2]*10 + jjj;
   if ( debug_page==0) dd_v[DDS_DEBUG + 0x03] = cmd_state;

   if ( cmd_state >= 100 ) // a command is complete, need to process it
      {
      if ( cmd_state==100 ) // PLAY X X X : goto direct to catalog star 
         {
         if ( cmd_val[0] == IDX_VCR1_GUIDE )  // GUIDE X X X : special command
            {
            if ( mosaic_seconds!=-1 ) 
               {
               if ( cmd_val[1] == IDX_VCR1_0 )  mosaic_seconds=-1;  // cancel mosaic
               else                             mosaic_seconds=-2;  // cancel mosaic and return to origin
               }
            else if ( cmd_val[1] == IDX_VCR1_9 || cmd_val[1] == IDX_VCR1_8 )   // GUIDE 9 X X : mosaic
               {
               mosaic_base_ra  = ra->pos;
               mosaic_base_dec = dec->pos;
               mosaic_nb_tiles = cmd_val[2];  // GUIDE 9 NB-TILES X
               mosaic_span_idx = cmd_val[3];  // GUIDE 9 NB-TILES SPAN
               mosaic_seq_ra   = 0; 
               mosaic_seq_de   = 0;
               mosaic_seconds  = 30;    // Start the mosaic
               mosaic_dt       = 36;
               if ( cmd_val[1] == IDX_VCR1_8 ) // scan mode (search a star...)
                  {
                  mosaic_dt       = 1;
                  mosaic_nb_tiles = cmd_val[2]<<MOSAIC_MULT;
                  }
               }
            }
         else
            {
            if ( align_state != 0 )  goto_pgm_pos(cmd_val[cmd_val_idx-3]*100 + cmd_val[cmd_val_idx-2]*10 + jjj); 
            else {
                 dd_v[DDS_CUR_STAR_REQ] = cmd_val[cmd_val_idx-3]*100 + cmd_val[cmd_val_idx-2]*10 + jjj;   // ask the slave
                 set_ra_armed = 1;
                 }
            }
         }
      if ( cmd_state==101 ) // PLAY INPUT X : goto user position X
         { goto_pos(jjj); }
      if ( cmd_state==102 ) // PLAY ANTENA X : goto star corrected position X
         { goto_pos(10+jjj);  last_antena=10+jjj; }
      if ( cmd_state==103 ) // RECORD INPUT X : record user position X
         { record_pos(jjj);  }
      if ( cmd_state==104 ) // RECORD ANTENA X : record star corrected position X
         { 
         record_pos(10+jjj); 
         twi_seq+=2;  // tell the slave to update the active star's corrected position
         if ( align_state == 0 ) align_state++; // The initial RA is set...  now fake a few alignments (to help inhouse debug)
//         if ( align_state == 0 ) align_state=10 // The initial RA is set...  now a play will make the telescope move
         }
      if ( cmd_state==105 ) // CLEAR INPUT : clear all user positinos
         { for(iii=0;iii<10;iii++) saved[iii].ra = saved[iii].dec = saved[iii].ref_star=0; }
      if ( cmd_state==106 ) // CLEAR ANTENA : clear all star corrected position 
         { for(iii=0;iii<10;iii++) saved[10+iii].ra = saved[10+iii].dec = saved[10+iii].ref_star=0; }
      if ( cmd_state==107 ) // CLEAR ? : clear something else !
         { 
         if ( code_idx == IDX_VCR1_INFO )   for(iii=0;iii<16;iii++) dd_v[DDS_HISTO+iii]=0; 
         if ( code_idx == IDX_VCR1_OK )     redraw   = 1;       // redraw everything
         if ( code_idx == IDX_VCR1_SEARCH ) loc_dec_correction=loc_ra_correction=use_polar=0; // Disable polar correction
         cmd_state = 0;
         }
      if ( cmd_state==108 ) // COMPLEX GOTO MANUAL POSITION
         {
         }
      if ( cmd_state==109 ) // ANTENA SEARCH : Calculate polar error
         { 
         // TODO to be done by the slave: if ( jjj==3) do_polar(); 
         twi_seq=-1;  // tell the slave to update the polar matrix
         align_state=11;
         }
      if ( cmd_state==110 ) // PLAY PLAY ... so, goto here (remove mechanical friction
         {
         ra->pos_target  = ra->pos+1;
         dec->pos_target = dec->pos+1;
         goto_cmd = 1;
         }
      cmd_state=0; // we are done 
      }
   else if ( next_input < 0 ) // Check for a one key command
      {
      if      ( code_idx >= IDX_VCR1_NORTH && code_idx <= IDX_VCR1_STOP    ) slew_cmd = code_idx;  // Slew commande: North South East West and Stop
      else if ( code_idx == IDX_VCR1_TRAK_P  ) earth_tracking=1;         // Start tracking
      else if ( code_idx == IDX_VCR1_TRAK_M  ) earth_tracking=0;         // Stop tracking
      else if ( code_idx == IDX_VCR1_GOBACK  ) goto_pgm_pos(dd_v[DDS_CUR_STAR]);  // goto active star
      else if ( code_idx == IDX_VCR1_SPEED   ) 
         { 
         torq = (torq+1)&3;                                // Increment torq value
         if (torq<minumum_torq) torq = minumum_torq;      // if lower than effective torq, then use effectiv torq (cant change to lower than minimum required)
         if ( debug_page==0) dd_v[DDS_DEBUG + 0x1E]=torq ; 
         }
      else if ( code_idx == IDX_VCR1_MUTE    ) { dcay = (dcay+1)&3;  if ( debug_page==0) dd_v[DDS_DEBUG + 0x1D]=dcay ; }
      else if ( code_idx == IDX_VCR1_INFO    ) 
         {
         debug_page ++ ;
         if ( debug_page> NB_DEBUG_PAGE ) debug_page = 0;
         for ( iii=0 ; iii<32 ; iii++ ) dd_v[DDS_DEBUG + iii] = 0;
         }
      else if ( code_idx == IDX_VCR1_POWER   ) 
         {
         motor_disable = !motor_disable;
         //set_digital_output(DO_PD3_DISABLE  ,motor_disable);
         //if ( motor_disable ) set_digital_output(DO_PD3_DISABLE  ,2);  // see: include/pololu/digital.h, it seems that using constants makes the code very efficiant
         //else                 set_digital_output(DO_PD3_DISABLE  ,0);
//         if ( motor_disable ) PORTD &= ~DO_PD3_DISABLE;  // set the pin to 0 V
//         else                 PORTD |=  DO_PD3_DISABLE;
         IF_CONDITION_PORT_BIT(motor_disable==0 , PORTD , DO_PD3_DISABLE );
         }
      else if ( code_idx == IDX_VCR1_FWD     ) 
         {
         if ( dd_v[DDS_CUR_STAR_REQ] == STARS_COORD_TOTAL  ) dd_v[DDS_CUR_STAR_REQ]  =0; // we reached the last star
         else                                                dd_v[DDS_CUR_STAR_REQ]  ++;
         }
      else if ( code_idx == IDX_VCR1_REW ) 
         {
         if ( dd_v[DDS_CUR_STAR_REQ] == 0 )  dd_v[DDS_CUR_STAR_REQ]   = STARS_COORD_TOTAL-1; // we reached the first star
         else                                dd_v[DDS_CUR_STAR_REQ]  --;  
         }
 
      }
   }
#endif  // AT_MASTER process IR

// make sure CUR_STAR is inbound   
if ( dd_v[DDS_CUR_STAR_REQ] >= STARS_COORD_TOTAL ) dd_v[DDS_CUR_STAR_REQ] = 0; // we reached the last star
if ( dd_v[DDS_CUR_STAR_REQ] <  0                 ) dd_v[DDS_CUR_STAR_REQ] = STARS_COORD_TOTAL-1;
}

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

//dd_v[DDS_DEBUG + 0x03]=ra->pos;
//dd_v[DDS_DEBUG + 0x04]=fp_sin(ra->pos);
//dd_v[DDS_DEBUG + 0x05]=ra->pos;
//dd_v[DDS_DEBUG + 0x06]=ra->pos_dem;
//dd_v[DDS_DEBUG + 0x07]=dec->state;
//dd_v[DDS_DEBUG + 0x0D]=goto_cmd + 0x100*moving;
//dd_v[DDS_DEBUG + 0x0F]=ra->state;
if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x08] = ra_correction;
if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x09] = dec_correction;
if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x0A] = loc_ra_correction;
if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x0B] = loc_dec_correction;
if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x0C] = align_state + 0x123000; //temp

if ( redraw ) 
   {
   first = 1;
   redraw = d_task = cycle = console_started = console_special_started = d_idx = rs232_tx_idx = 0;
   }

// process RX
if ( (UCSR0A & 0x80) != 0)    // ********** CA CA LIT BIEN...
   {
   static unsigned char esc_code=0,esc_codes[3];
   CCC = UDR0;
   if ( esc_code>0 || CCC==27 )
      {
      if ( CCC==27 ) esc_code=3;
      else esc_codes[esc_code-1] = CCC;
      if ( esc_code == 1 ) // last cde
         {
         rs232_rx = esc_codes[1]*0x100 + esc_codes[0];         
         rs232_rx_cnt++;
         } 
      esc_code--;
     dd_v[DDS_DEBUG + 0x06] = esc_codes[2]*0x10000 + esc_codes[1]*0x100 + esc_codes[0];
      }
//   else if ( rs232_rx_clr )
//      {
//      rs232_rx_clr = dd_v[DDS_RX_IDX] = 0;
//      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
//      }
//   if ( CCC == 0x08 ) // backspace
//      {
//      if ( dd_v[DDS_RX_IDX] > 0 ) dd_v[DDS_RX_IDX]--;
//      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
//      }
   else
      {
//      rs232_rx_buf[dd_v[DDS_RX_IDX]++] = CCC;
//      if ( dd_v[DDS_RX_IDX] >= RS232_RX_BUF ) dd_v[DDS_RX_IDX]=RS232_RX_BUF-1;
//      rs232_rx_buf[dd_v[DDS_RX_IDX]] = 0;
      rs232_rx = CCC;
      rs232_rx_cnt++;
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
            display_data((char*)console_buf,0,20,0,0,FMT_NO_VAL);
            return;
            }
   
         if ( dd_go(d_task-NB_DTASKS,first) )  Next = rs232_tx_buf[rs232_tx_idx++];     // something to display was found, rs232_tx_buf has something
         d_task++;

         if ( d_task >= DD_FIELDS + NB_DTASKS) { first = 0 ; d_task = NB_DTASKS; }
         }
      }
   } 
if ( Next != 0 ) UDR0 = Next;

#ifdef AT_SLAVE
if ( use_polar )
   {
   static VECTOR work;  
   static VECTOR desired;
   static unsigned char polar_state=0;  // use states to spread the work into many little chunks...mainly because I can have AP0 with SP0
   static unsigned char use_x_instead_of_y=0;  //
   static unsigned long desired_dec,ref_dec;
   static unsigned long desired_ra,ref_ra;
   static unsigned long test_bit;
   static long cos_dec;   // temp label
   static long angle_test; // new method
   static short kkk;

   if ( polar_state<10 )  // states [0..9] used to calculate desired X Y Z position
      {
      if      ( polar_state == 1 ) 
         { 
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x10]=standby_goto + 0x321000; /// REMOVE
         if ( standby_goto )
            {
            ref_ra = ra->pos_target;    // use target position so that any goto will bring us right away to the propoer position  
            ref_dec = dec->pos_target;  // use target position so that any goto will bring us right away to the propoer position;
            if ( standby_goto == 1 ) standby_goto = 2;
            }
         else
            {
            ref_ra = ra->pos; 
            ref_dec = dec->pos;
            }

         set_vector(&work, (unsigned long * ) &ref_ra, (unsigned long * ) &ref_dec); 
         }
      else if ( polar_state == 2 )
         {
//         dd_v[DDS_DEBUG + 0x08]=work.x;  // Actual value
//         dd_v[DDS_DEBUG + 0x09]=work.y;  // Actual value
//         dd_v[DDS_DEBUG + 0x0A]=work.z;  // Actual value
         }
      else if ( polar_state == 3 ) 
         { 
         set_vector(&desired , (unsigned long * ) &ref_ra, (unsigned long * ) &ref_dec); 
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x11]=desired.x;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x12]=desired.y;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x13]=desired.z;
         } 
      else if ( polar_state == 4 ) { apply_polar_correction(&PoleMatrix,&desired); } 
      else if ( polar_state == 5 )
         {
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x01]=desired.x;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x02]=desired.y;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x03]=desired.z;
         } 
      else polar_state = 10;   // go to next stage
      }
   else if ( polar_state<40 )  // states [10..39] where we find the required DEC to get the same Z as the desired Z
      {
      if      ( polar_state == 11 ) // setup  .... careful, RA is from 0 to 360, as all ticks values (positive)....but DEC is "signed" :(
         {                          // so because DEC is signed, I will search from 0 to 180 deg
         angle_test  = TICKS_P_90_DEG;
         desired_dec = 0x00000000;
         test_bit    = 0x40000000; 
         }
      else if ( test_bit >= 0x00000400 ) // while this is true, polar_state will go from 11 to .... 31 
         {
         work.z = fp_cos(desired_dec + angle_test);           
         if ( work.z > desired.z ) desired_dec += angle_test; // ok, still over, add to the angle
         test_bit = test_bit >> 1;
         angle_test = angle_test >> 1;
         }
      else  // go to next stage
         {
         long abs_x,abs_y;

         // ok, I found desired_dec, but I need to do the math to bring desired_dec to the proper value: desired_dec = TICKS_P_90_DEG - desired_dec;
         cos_dec = fp_sin(desired_dec);                 // Required for the next stages, since fp_functions work from -45 to 360 only, I use desired_dec+90 and sin()
         if ( desired_dec < TICKS_P_90_DEG ) desired_dec = TICKS_P_90_DEG - desired_dec;              // DEC above 0
         else                                desired_dec = TICKS_P_90_DEG - desired_dec - DEAD_BAND;  // DEC below 0, remove deadband aswell

         if ( desired.x < 0 ) abs_x = -desired.x;
         else                 abs_x =  desired.x;
         if ( desired.y < 0 ) abs_y = -desired.y;
         else                 abs_y =  desired.y;
         if ( abs_x > abs_y ) use_x_instead_of_y = 0;    // use Y to converge
         else                 use_x_instead_of_y = 1;    // use X to converge
         polar_state = 40;    // use X to converge
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x04]=use_x_instead_of_y;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x05]=abs_x;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x06]=abs_y;
         }
      }
   else if ( polar_state<80 )  // states [40..79] where we find the required RA to get the same Y as the desired Y    (because abs(x) > abs(y)
      {
      if      ( polar_state == 41 ) // setup
         {
         kkk=0;
         test_bit = 0x40000000; 
         angle_test = TICKS_P_90_DEG;
         if ( use_x_instead_of_y ) desired_ra = TICKS_P_180_DEG;  // Y < 0
         else                      desired_ra = TICKS_P_90_DEG;   // X < 0
         //  a bit complicated for X>0 because of negative values and the dead band, so lets use a similar technique as for the DEC
         }
      else if ( test_bit >= 0x00000400 ) // while this is true, polar_state will go from 11 to .... 31 
         {
         // Important:
         // lets not forget that fp_cos() fp_sin() uses unsigned TICKs as parameter and converts them to RAD
         // so we need to be careful when trying all the bits in desired_ra... we need to exclude the deadband
         if ( use_x_instead_of_y )
            {
            work.x  = fp_mult(fp_cos(desired_ra + angle_test),cos_dec); 
            if ( work.x < desired.x )   desired_ra += angle_test; // ok, still under, use it
            }
         else
            {
            work.y  = fp_mult(fp_sin( ( desired_ra +  angle_test) ),cos_dec);  // try this bit  // Be careful, because we are using Y, 
            if ( work.y > desired.y )   desired_ra += angle_test;              // ok, still under, use it
            //  a bit complicated for X>0 because of negative values and the dead band, so lets use a similar technique as for the DEC ...and fix the value at the end
            }
         test_bit = test_bit >> 1;
         angle_test = angle_test >> 1;
         kkk++;
         }
      else 
         {
         if ( use_x_instead_of_y )
            {
            if ( desired.y > 0 )
               {
               desired_ra = TICKS_P_DAY - desired_ra ; 
               }
            }
         else  
            {
            if ( desired.x > 0 )   // fix the value we found to addapt it 
               {
               if ( desired_ra < TICKS_P_180_DEG ) desired_ra = TICKS_P_180_DEG - desired_ra ;              // RA above 0
               else                                desired_ra = TICKS_P_180_DEG - desired_ra  - DEAD_BAND;  // RA below 0, remove deadband aswell
               }
            }
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x08] = work.x;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x09] = work.y;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x0A] = desired_ra;
         if ( (debug_page==3) && (test_bit==0x80000000) ) dd_v[DDS_DEBUG + 0x07]=work.y;
         if ( (debug_page==3) && (test_bit==0x80000000) ) dd_v[DDS_DEBUG + 0x0F]=desired.y;
         if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x1A]++;

         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x01]=desired.y;
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x02]=work.y;
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x03]=desired_ra ;
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x04]=ref_ra;
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x05]=fp_sin(desired_ra);
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x06]=fp_sin(ref_ra);
         if ( debug_page==4 ) dd_v[DDS_DEBUG + 0x07]=cos_dec;

         polar_state = 80; // go to next stage
         }
      } 
   else if ( polar_state<90 )  // states [80..89] final, generate the delta RA and delta DEC
      {
//      if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x14]=ref_ra;
//      if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x15]=desired_ra;
//      if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x16]=ref_dec;
//      if ( debug_page==3 ) dd_v[DDS_DEBUG + 0x17]=desired_dec;

      loc_ra_correction  = desired_ra  - ref_ra;
      loc_dec_correction = desired_dec - ref_dec;

      polar_state = 0;

      if ( standby_goto == 2 )
         {
         if ( ra_correction > loc_ra_correction )   { if ( ra_correction      - loc_ra_correction  < 20 ) standby_goto =3; }
         else                                       { if ( loc_ra_correction  - ra_correction      < 20 ) standby_goto =3; }
         }
      else if ( standby_goto == 3 )
         {
         if ( dec_correction > loc_dec_correction ) { if ( dec_correction     - loc_dec_correction < 20 ) standby_goto =-1; }
         else                                       { if ( loc_dec_correction - dec_correction     < 20 ) standby_goto =-1; }
         }
      }
   else polar_state=0;

   polar_state++;

//   dd_v[DDS_DEBUG + 0x17]=0x1700 + use_x_instead_of_y;
   }
else if ( standby_goto == 1 ) standby_goto = -1;  // no wait in this case
//dd_v[DDS_DEBUG + 0x18]=standby_goto;
//   dd_v[DDS_DEBUG + 0x1C]=ra_correction;
//   dd_v[DDS_DEBUG + 0x1D]=dec_correction;
//   dd_v[DDS_DEBUG + 0x14]=loc_ra_correction;
//   dd_v[DDS_DEBUG + 0x15]=loc_dec_correction;
else if ( standby_goto == 111 ) do_polar(); //   TODO This will never happen, but it's to force the compiler tp include do_polar in the hex file
#elif AT_MASTER
if ( standby_goto == 1 ) standby_goto = -1;  // no wait in this case TODO : since the polar math is done on the slave, the standby_goto seends to be sent back and forth
#endif
}


#ifdef AT_MASTER
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
   if ( axis->accel > 0 )      axis->pos_delta += (TICKS_P_MIN)>>2; // Always go 15 seconds downtime for mechanical friction issue  // mechanical friction left to right / up to down

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
   else                        axis->speed =  100; // we stopped short, slowly go forward
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
         if ( slew_cmd == IDX_VCR1_EAST ) axis->spd_index++;       // Right - East
         if ( slew_cmd == IDX_VCR1_WEST ) axis->spd_index--;       // Left - West
         }
      else
         {
         if ( slew_cmd == IDX_VCR1_NORTH ) axis->spd_index++;       // Up - North
         if ( slew_cmd == IDX_VCR1_SOUTH ) axis->spd_index--;       // Down - South
         }
      if ( slew_cmd == IDX_VCR1_OK  || slew_cmd == IDX_VCR1_STOP   ) axis->spd_index = 0;     // all Stop
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
add_value_to_pos(axis->speed,&axis->pos);
axis->pos_displ += axis->speed;            // Use a parallel pos counter that is independent from the DEADBAND

axis->pos_dem = axis->pos;
if ( ra_axis ) 
   {
   add_value_to_pos(-axis->pos_earth,&axis->pos_dem);   // add the earth's rotation only on the RA axis
   axis->pos_cor = axis->pos_dem;
   add_value_to_pos(ra_correction,&axis->pos_cor);     // add the local polar correction
   }
else
   {
   axis->pos_cor = axis->pos_dem;
   add_value_to_pos(dec_correction,&axis->pos_cor);     // add the local polar correction
   }


//ra->pos_dem = ra->pos_earth + ra->pos;
//ra->pos_cor = ra->pos_dem;  // for now, no correction

return axis->state;
}
#endif  // AT_MASTER : process_goto


///////////////////////////////////////////// TWI /////////////////////////////////////////////////////////////////////////////////
// OFFSETs to use in the s/w
#define TWI_NS  0   // NEXT STATE
#define TWI_ES  1   // ERROR STATE  (Where to go on error)
#define TWI_SR  2   // STATUS REGISTE expected value
#define TWI_CR  3   // COMMAND REGISTER value to put in
#define TWI_SC  4   // SPECIAL CODE : WAIT / RESET COUNT / DECREMENT COUNT
#define TWI_DL  5   // DELAY TO WAIT
#define TWI_NS2 5   // ALTENATE NEXT STATE
#define TWI_SR2 6   // ALTENATE STATUS REGISTER

///////////////////  Special code contains:
// 0x80 Wait request
// 0x40 Reset pointer, get size and target
// 0x20 SLA+W update count, and if 0, then twi_state++   (Master Transmits)
// 0x10 SLA+R update count, and if 0, then twi_state++   (Slave Transmits)
// 0x0F Identifies which index to imcrement (debug counter)  0..15

// the background fills or decodes this buffer
// command buffers ; format : TWI_TARGET_ADDRESS, DATA, DATA ..., CHECKSUM     (note BYTE_COUNT is the count of DATA bytes plus 2...the address and the checksum)
//                  example : 0x20 , 0x0B, 0x12, 0x34, 0x56, 0x78 , 0x12, 0x34, 0x56, 0x78 , 0xCD   <- The sum of the checksum and the data = 0  (address is not part of the checksum)
unsigned char twi_tx_buf[40];
unsigned char twi_rx_buf[40];
// As for the data, the format is the same for master and slave:
// BYTE_COUNT, RXID, TX_ID, TXDATA, TXDATA ... from the master to the slave, TXID identifies that the data belongs to, and RXID asks the slave what to prepate
// BYTE_COUNT, RXID, TX_ID, RXDATA, RXDATA ... from the slave to the master, RXID is the requested data ID, RXDATA is the requested data, and TXID is a confirmation of the last data that the slave received

 
#ifdef AT_SLAVE
PROGMEM char twi_states[] =
//      NEXT   ERROR  ON     DO     SPECIAL NEXT   ON
//      STATE  STATE  TWSR   TWCR   CODE    STATE2 TWSR2            //   TWI SLAVE STATES
    {   0x01 , 0x00 , 0x60 , 0xC4 , 0x00 ,  0x06 , 0xA8             //   0  Wait for SLA+W or SLA+R  
    ,   0x02 , 0x04 , 0x80 , 0xC4 , 0xE0 ,  0x00 , 0xFF             //   1  SLA+W get the size byte, init counter
    ,   0x02 , 0x04 , 0x80 , 0xC4 , 0xA0 ,  0x00 , 0xFF             //   2  SLA+W get the data, update counter 
    ,   0x00 , 0x04 , 0xA0 , 0xC4 , 0x80 ,  0x00 , 0xFF             //   3  SLA+W get the last data byte 
    ,   0x00 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //   4  SLA+W Error 

    ,   0x06 , 0x08 , 0xB8 , 0xC4 , 0xB0 ,  0x00 , 0xFF             //   5  SLA+R Init the Counter 
    ,   0x06 , 0x0B , 0xB8 , 0xC4 , 0x90 ,  0x00 , 0xFF             //   6  SLA+R Send Data, update counter
    ,   0x00 , 0x0C , 0xC0 , 0xC4 , 0x10 ,  0x00 , 0xFF             //   7  SLA+R Send the last data byte
    ,   0x00 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //   8  SLA+R Error TWSR=0x00 seems to hapen when slave thinks it must send more, but master sends STOP
    ,   0x09 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //   9  STOP
    ,   0x00 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //  10  SLA+R Error 
    ,   0x00 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //  11  SLA+R Error TWSR=0x00 seems to hapen when slave thinks it must send more, but master sends STOP
    ,   0x00 , 0xFF , 0xFF , 0xC4 , 0x00 ,  0x00 , 0xFF             //  12  SLA+R Error TWSR=0x00 seems to hapen when slave thinks it must send more, but master sends STOP
    };
   #define TWI_ROW 7   // 8 data per line
#else                
PROGMEM char twi_states[] =
//      NEXT   ERROR  ON     DO     SPECIAL DELAY
//      STATE  STATE  TWSR   TWCR   CODE    TO WAIT                 //   TWI MASTER STATES
    {   0x01 , 0xFF , 0xF8 , 0xA4 , 0xC0 ,  0x00   //   0  The bus is available ->  lets transmit a START , and initialize the count and pointer
    ,   0x02 , 0xFF , 0x08 , 0x84 , 0xA0 ,  0x00   //   1  START sent -> Send SLA address
    ,   0x03 , 0x05 , 0x18 , 0x84 , 0xA0 ,  0x00   //   2  SLA sent -> send data , decrement count
    ,   0x03 , 0x08 , 0x28 , 0x84 , 0xA0 ,  0x00   //   3  data sent -> continue until count is zero in which case twi_state++
    ,   0x09 , 0x07 , 0x28 , 0x94 , 0x00 ,  0x50   //   4  Last byte transmit -> send STOP, no wait, then go to state 9  (wait 5ms for slave to process data)
    ,   0x00 , 0xFF , 0xFF , 0x94 , 0x00 ,  0x03   //   5  On error, come here -> on any status, send a stop, no wait, then go to state 0
    ,   0x00 , 0x00 , 0x00 , 0x00 , 0xFF ,  0x00   //   6 
    ,   0x00 , 0xFF , 0xFF , 0x94 , 0x00 ,  0x00   //   7  STOP
    ,   0x00 , 0xFF , 0xFF , 0x94 , 0x00 ,  0x00   //   8  STOP
    ,   0x0A , 0xFF , 0xF8 , 0xA4 , 0x80 ,  0x00   //   9  The bus is available ->  lets transmit a START , and initialize the count and pointer
    ,   0x0B , 0xFF , 0x08 , 0x84 , 0xD0 ,  0x00   //   A  START sent -> Send SLA address
    ,   0x0C , 0x13 , 0x40 , 0xC4 , 0xC8 ,  0x00   //   B  SLA sent -> receive data , decrement count
    ,   0x0C , 0x11 , 0x50 , 0xC4 , 0x88 ,  0x00   //   C  data received. -> continue until count is zero in which case twi_state++
    ,   0x0E , 0x12 , 0x50 , 0x84 , 0x88 ,  0x00   //   D  receiving last byte -> send Nack
    ,   0x0F , 0x11 , 0x58 , 0x94 , 0x08 ,  0x00   //   E  Reached the end of the bytes to receive -> send STOP, no wait, then go to state 0
    ,   0x00 , 0x11 , 0xF8 , 0x84 , 0x00 ,  0x03   //   F  STOP Sent
    ,   0x10 , 0xFF , 0xFF , 0x84 , 0x00 ,  0x00   //  10  STOP
    ,   0x11 , 0xFF , 0xFF , 0x94 , 0x00 ,  0x03   //  11  On error, come here -> on any status, send a stop, no wait, then go to state 0
    ,   0x12 , 0xFF , 0xFF , 0x84 , 0x00 ,  0x00   //  12  STOP
    ,   0x09 , 0xFF , 0xFF , 0x94 , 0x00 ,  0x03   //  13  On error, come here -> on any status, send a stop, no wait, then go to state 0
    };
   #define TWI_ROW 6   // 8 data per line
#endif  // AT_MASTER

// a full message with a valid checksum was received in twi_tx_buf ...process it
//#define TWI_C1 32
#define TWI_C1 32
void twi_rx(void)
{
unsigned char *cra = (unsigned char *)& loc_ra_correction;
unsigned char *cde = (unsigned char *)& loc_dec_correction;
unsigned char *pd  = (unsigned char *)& dd_v[DDS_CUR_STAR];
unsigned char *sp  = (unsigned char *)& dd_v[DDS_STAR_DEC_POS];
unsigned char iii;
#ifdef AT_SLAVE
unsigned char *pc  = (unsigned char *)& dd_v[DDS_CUR_STAR_REQ]  ;
unsigned char *pp  = (unsigned char *)  twi_pos;
unsigned char sum=0,found=0;
if ( twi_tx_buf[3] == 0xC0 )  // Current position
   {
   for ( iii=1 ; iii<TWI_C1 ; iii++ ) twi_rx_buf[iii]=0xFA;

   pc[0] = twi_tx_buf[4];   // Current requested star
   pc[1] = twi_tx_buf[5];   // Current requested star
   if ( debug_page != twi_tx_buf[7] ) for ( iii=0 ; iii<32 ; iii++ ) dd_v[DDS_DEBUG + iii] = 0; // Debug info
   debug_page = twi_tx_buf[7];
   for ( iii=0 ; iii<8      ; iii++ ) pp[iii] = twi_tx_buf[8+iii]; // update current position
   ra->pos  = twi_pos[0];
   dec->pos = twi_pos[1];

   if ( twi_seq != twi_tx_buf[6] )  // Add the current star to the list of corrected star position
      {
      for ( iii = 10 ; iii<20 && (found==0); iii++ )  // for all slots
         {
         if ( saved[iii].ref_star==dd_v[DDS_CUR_STAR] ) found=iii;  // Found a slots when we previously recorded a correction to that same star
         if ( saved[iii].ref_star==0 )                  found=iii;  // New star corrected pos
         }
      record_pos(found); 
      if ( (twi_seq!=255) && (twi_tx_buf[6]==255) )    do_polar();    // 255 means do polar , 0 in feedback meeds that it's done
      twi_seq = twi_tx_buf[6]; 
      }
   }
   dcay           = twi_tx_buf[17];          
   effectiv_torq  = twi_tx_buf[18]; 
   motor_disable  = twi_tx_buf[19]; 
   twi_star_ptr   = twi_tx_buf[20];   // Value fromm master
   cmd_state      = twi_tx_buf[21];   // Flash the Yello led
   mosaic_twi     = twi_tx_buf[22];   // Bits 210 : 
   ///////// Prepare responce
   twi_rx_buf[1]  = twi_fb_count; 
   twi_rx_buf[2]  = twi_tx_buf[2];  // command feedback
   twi_rx_buf[3]  = twi_tx_buf[3];  // data feedback 
   if ( twi_star_ptr >= STAR_NAME_LEN+CONSTEL_NAME_LEN )
      {
      twi_rx_buf[4]  = 0;                     // Send to Master
      twi_rx_buf[5]  = current_star_name[0];  // Send to Master the Current Character
      }
   else
      {
      twi_rx_buf[4]  = twi_star_ptr+1;                     // Send to Master
      twi_rx_buf[5]  = current_star_name[twi_star_ptr+1];  // Send to Master the Current Character
      }
   twi_rx_buf[6] = pd[0];   // Return Slave's active star
   twi_rx_buf[7] = pd[1];   // Return Slave's active star
   for ( iii=0 ; iii<8      ; iii++ ) twi_rx_buf[ 8+iii] = sp[iii];  // update current position

   for ( iii=0 ; iii<4      ; iii++ ) twi_rx_buf[16+iii] = cra[iii]; // update current polar correction
   for ( iii=0 ; iii<4      ; iii++ ) twi_rx_buf[20+iii] = cde[iii]; // update current polar correction

   twi_rx_buf[24] = use_polar; 
   twi_rx_buf[25] = 161;     // Debug Marker
   twi_rx_buf[26] = 162;     // Debug Marker

for ( iii=1 ; iii<TWI_C1 ; iii++ ) sum +=twi_rx_buf[iii];
twi_rx_buf[TWI_C1] = -sum;   // Checksum


//IF_NOT_CONDITION_PORT_BIT( dcay&0x01 , PORTB , DO_PB7_GREEN    );
//IF_NOT_CONDITION_PORT_BIT( dcay&0x02 , PORTD , DO_PD5_YELLOW );
//IF_NOT_CONDITION_PORT_BIT( dcay&0x04 , PORTD , DO_PD6_RED    );

IF_CONDITION_PORT_BIT( effectiv_torq&0x01 , PORTB , DO_PB0_LCD_B4 );
IF_CONDITION_PORT_BIT( effectiv_torq&0x02 , PORTB , DO_PB1_LCD_B5 );
IF_CONDITION_PORT_BIT( effectiv_torq&0x04 , PORTB , DO_PB2_LCD_B6 );
IF_CONDITION_PORT_BIT( effectiv_torq&0x08 , PORTB , DO_PB3_LCD_B7 );

// TEST IF_CONDITION_PORT_BIT( effectiv_torq&0x01 , PORTC , DO_PC2_LCD_RS );
// TEST IF_CONDITION_PORT_BIT( effectiv_torq&0x02 , PORTC , DO_PC1_LCD_RW );
// TEST IF_CONDITION_PORT_BIT( effectiv_torq&0x04 , PORTC , DO_PC0_LCD_EE );
// not working: IF_NOT_CONDITION_PORT_BIT( ssec&0x02              , PORTC , DO_PC2_LCD_RS    );  // Buzzer
IF_NOT_CONDITION_PORT_BIT( ( ( mosaic_twi & 2 ) == 0 )  , PORTC , DO_PC0_LCD_EE    );  // Secondary click Remote Shutter  Clicks when condition=0)
IF_NOT_CONDITION_PORT_BIT( ( ( mosaic_twi & 4 ) == 0 )  , PORTC , DO_PC1_LCD_RW    );  // Primary click Remote Shutter  Clicks when condition=0)

dd_v[DDS_DEBUG + 0x1E] = ((long)effectiv_torq<<16) + dcay;

#endif  // AT_SLAVE
#ifdef AT_MASTER

twi_star_ptr = twi_rx_buf[4];   // Master: set twi_star_ptr to what the Slave says it should be
current_star_name[twi_star_ptr] = twi_rx_buf[5];  // Master set the sting...
pd[0] = twi_rx_buf[6]; // Slave's active star
pd[1] = twi_rx_buf[7]; // Slave's active star
for ( iii=0 ; iii<8      ; iii++ ) sp[iii+0] = twi_rx_buf[8+iii]; // update current position
for ( iii=4 ; iii<8      ; iii++ ) sp[iii+4] = twi_rx_buf[8+iii]; // update current position

for ( iii=0 ; iii<4      ; iii++ ) cra[iii] = twi_rx_buf[16+iii]; // update current polar correction
for ( iii=0 ; iii<4      ; iii++ ) cde[iii] = twi_rx_buf[20+iii]; // update current polar correction
use_polar = twi_rx_buf[24];
if ( use_polar && (align_state==11)) align_state=12; 
// if ( twi_star_ptr >= STAR_NAME_LEN+CONSTEL_NAME_LEN-1 ) 
//    { 
//    pd[0] = twi_rx_buf[6];   // Update Master Current Star
//    pd[1] = twi_rx_buf[7];   // Update Master Current Star
//    }
#endif  // AT_MASTER
}

// Prepare the next message to transmit
void twi_tx(void)
{
#ifdef AT_MASTER
//unsigned char *pc = (unsigned char*)&dd_v[DDS_CUR_STAR];
unsigned char *pc = (unsigned char *)& dd_v[DDS_CUR_STAR_REQ]  ;
unsigned char *pp = (unsigned char *)twi_pos;
unsigned char iii,sum=0;

twi_hold = 1;         // Tell foreground that if it interrupts us, it must not drive the twi position because we are reading them...
for ( iii=1 ; iii<TWI_C1 ; iii++ ) twi_tx_buf[iii]=0xAA;

twi_tx_buf[0]  = 0x20;   // Slave address
twi_tx_buf[1]  = TWI_C1+1;     // Byte count
twi_tx_buf[2]  = 0xD0;   // Req Data : tell the Slave what we want to receive next  
twi_tx_buf[3]  = 0xC0;   // Sup Data : tell the Slave what we are about to supply in terms of data in this packet
twi_tx_buf[4]  = pc[0];   // Requested Star (LSB)
twi_tx_buf[5]  = pc[1];   // Requested Star (MSB)
twi_tx_buf[6]  = twi_seq;   // Seq: sequence counter , incremented each time we record a new corrected star position
twi_tx_buf[7]  = debug_page;   // for the display
for ( iii=0 ; iii<8      ; iii++ ) twi_tx_buf[8+iii] = pp[iii];   // LAT/LON
twi_tx_buf[16]  = standby_goto;   // fix error before move request
twi_tx_buf[17]  = dcay;   // test LED
twi_tx_buf[18]  = effectiv_torq;   // test LCD
twi_tx_buf[19]  = motor_disable; 
twi_tx_buf[20]  = twi_star_ptr;   // let the slave know where we are...
twi_tx_buf[21]  = cmd_state;      // Flash YELLOW led
// twi_tx_buf[22]  = (mosaic_seconds!=-1)&&(mosaic_seconds<30)&&(moving==0); // Flash RED led if in mosaic mode  // Bits: 210 : Primary, Secondary, Mosaic active
mosaic_twi = 0;
//if ( (mosaic_seconds !=-1 )&&( mosaic_seconds<30)&&(moving==0) ) mosaic_twi |= 1;  // Bit 0 : Mosaic Active, Flash the red Led
if ( (mosaic_seconds >  2 )&&( mosaic_seconds+1 < mosaic_dt  ) ) mosaic_twi |= 2;  // Bit 1 : Secondary 
if ( (mosaic_seconds >  4 )&&( mosaic_seconds+1 < mosaic_dt  ) ) mosaic_twi |= 4;  // Bit 2 : Primary 
twi_tx_buf[22]  = mosaic_twi;
 
// 8 bytes available here (32 max)
twi_tx_buf[23]  = 160;   // debug marker
twi_tx_buf[24]  = 161;   // debug marker

for ( iii=1 ; iii<TWI_C1 ; iii++ ) sum +=twi_tx_buf[iii];
twi_tx_buf[TWI_C1] = -sum;   // Checksum

twi_rx_buf[0]  = 0x21;   // Slave address READ Request
twi_fb_count   = TWI_C1+1;    // hardcoded for now, but it could depens on what the Master wants in return
twi_hold = 0;         // Tell foreground that if it interrupts us, it must not drive the twi position because we are reading them...
#elif AT_SLAVE
if ( twi_fb_count==0 ) twi_fb_count   = TWI_C1;    // TODO: remove ... this should be set by the reception SLA+W
#endif
}

static unsigned char twi_enable=0;
// Two Wire Interface Telescope Tricks
void twitt(void)
{
static unsigned char twi_state=0, wait=0 , twi_idx, lstate=0;
static unsigned char sequence=8;   // what to request from slave : 1- current selected catalog star, 2- polar correction ... 
static unsigned short *aa=(unsigned short*)&dd_v[DDS_DEBUG + 0x01];
#ifdef AT_SLAVE
unsigned char SR2;
#else 
unsigned char CR;
#endif
unsigned char SC=0,SR,ES;
unsigned char *p;

#ifdef AT_MASTER
if ( ddll != 0 ) return ; // wait a bit
#endif
unsigned char twcr = TWCR & wait; 
unsigned char twsr = TWSR&0xF8;  // flush the prescaler bits

if ( !twi_enable ) return;

p = (unsigned char*)&dd_v[DDS_DEBUG + 0x00]; p[3] = wait; p[2] = TWCR; p[1] = twsr; p[0] = twi_state;

if ( twcr )    // I seem to get called with Status=0xF8 ... why ?  ... because I was reading TWCR after reading twsr ... so I could get premature statuses
   {
   wait=0;
   twsr = TWSR&0xF8;  // flush the prescaler bits
   }
else if (wait) return;  // still waiting
#ifdef AT_MASTER
//if ( twi_success_rx > 2 && twi_state==0 ) return;   // To debug, stop after a few transmit
#endif

sequence++;
if ( debug_page==0 ) { p = (unsigned char*)&dd_v[DDS_DEBUG + 0x08 + twi_state]; p[3]=sequence; p[2]=lstate; p[1]++; p[0] = twsr; } // Use DEBUG 0x10 through 0x08 to debug TWI states ############################################
lstate = twi_state;

#ifdef AT_MASTER
   SR = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_SR ] );
   if ( (twsr == SR) || (SR == 0xFF) )   // TWI operation complete, check the status result
      {                                  // Matched the expected status register value for the current state , perfom next operation
      SC = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_SC ] );

      wait = SC & 0x80 ;                 // 0x80 : Set the wait flag if one requested for the current state
      if ( SC & 0x40 ) twi_idx = 0;      // 0x40 : Bit that ways that we should initialize the Counter
      if ( SC & 0x38 )                   // 0x20 : Bit that ways that we drive TWDR and update the counter
         {
         if      ( SC & 0x20 ) TWDR = twi_tx_buf[twi_idx++];   // 0x20 : Bit that says that we drive TWDR and update the counter
         else if ( SC & 0x10 ) TWDR = twi_rx_buf[twi_idx++];   // 0x10 : Bit that says that we drive TWDR only for the Slave address, then read
         else                  twi_rx_buf[twi_idx++] = TWDR;   // 0x08 : Bit that says that we read TWDR
         }
      CR = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_CR ] ); 
      TWCR = CR;
//      if ( CR&0x20) PORTD |=  DO_PD4_TWI_START; // Set   pin PD4 when START bit is set   to thlp debug with the scope
//      else          PORTD &= ~DO_PD4_TWI_START; // Clear pin PD4
      IF_CONDITION_PORT_BIT( CR&0x20 , PORTD , DO_PD4_TWI_START  );

      if      ( (SC & 0x20)!=0 && (twi_idx==twi_tx_buf[1])  ) { twi_state++; }       // when 0x20 (count mode) go to next state when count is reached
      else if ( (SC & 0x18)!=0 && (twi_idx==twi_fb_count-1) ) { twi_state++; }       // when 0x20 (count mode) go to next state when count is reached
      else twi_state = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_NS ] );  // otherwise, go to specified next state
      }  
   else                                  // we did not match the expected status register value for the current state, go to ERROR STATE
      {                                  // required otherwise we can jam here
      ES = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_ES ] );
      if ( ES !=0xFF )  twi_state = ES;   // if a valid new state, use it
      }
   ddll = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_DL ] );
   if ( twi_state== 0x04 )   // TODO this is in SLAVE RECEIVE ... this is hardcoded
      {
      if ( debug_page==0 ) aa[1]++;  // Nb messages sent so far...
      }
   else if ( twi_state== 0x0F )  // TODO this is in SLAVE TRANSMIT ... this is hardcoded to the state
      {
      unsigned char sum=0,iii;
      if ( debug_page==0 ) aa[0]++;  // Nb messages received so far...

      for ( iii=1 ; iii<=twi_rx_buf[1] ; iii++ ) sum +=twi_rx_buf[iii];  // Check for valid Checksum
      if ( debug_page==2 ) 
         {
      /* DEBUG */ p = (unsigned char*)&dd_v[DDS_DEBUG + 0x1F]; 
      /* DEBUG */ p[0] = sum;
      /* DEBUG */ if ( sum == 0 ) p[1] +=0x10;
      /* DEBUG */ else            p[2] ++;
         }

      if ( sum == 0 ) twi_rx(); // Master to process message from slave
      }
   if ( twi_state== 0 ) twi_tx();
#endif

#ifdef AT_SLAVE
SR  = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_SR ] );
SR2 = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_SR2 ] );
if ( (twsr == SR) || (twsr == SR2) || (SR == 0xFF) )   // TWI operation complete, check the status result
   {
   SC = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_SC ] );
   if ( twsr == SR2  && twi_fb_count!=0 ) 
      {              // when receiving a SLA+R, then here, lets init the counter and send the first one
      twi_idx = 1;   // at this point TWDR contains the Slave's address...
      SC |=0x10;
      }
   if ( SC & 0x40 ) twi_idx = 1;      // 0x40 : Bit that ways that we should initialize the Counter
   if ( SC & 0x30 )                   
      {
      if ( SC & 0x20 ) twi_tx_buf[twi_idx++] = TWDR;  // 0x20 : Bit that ways that we read TWDR and update the counter
      else             TWDR = twi_rx_buf[twi_idx++];  // 0x10 : Bit that ways that we Write to TWDR and update the counter
      }
   TWCR = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_CR ] );

   if ( (SC & 0x20)!=0 && (twi_idx==twi_tx_buf[1]) )  // Message complete
      {
      unsigned char sum=0,iii;
      for ( iii=1 ; iii<=twi_tx_buf[1] ; iii++ ) sum +=twi_tx_buf[iii];  // Check for valid Checksum
//      /* DEBUG */ p = (unsigned char*)&dd_v[DDS_DEBUG + 0x0E]; 
//      /* DEBUG */ p[0] = sum;
//      /* DEBUG */ if ( sum == 0 ) p[1] ++;
//      /* DEBUG */ else            p[2] ++;

      if ( sum == 0 ) twi_rx();
      twi_state++;     // when 0x20 (count mode) go to next state when count is reached
      if ( debug_page==0 ) aa[1]++;  // High part          // Nb bytes sent so far...
      }
   else if ( (SC & 0x10)!=0 && (twi_idx==twi_fb_count+2) )  // Message complete  +2
      {
      //twi_rx();
      twi_state++;     // when 0x10 (count mode) go to next state when count is reached
      if ( debug_page==0 ) aa[0]++;  // High part          // Nb bytes received so far...
      }
   else 
      {
      if ( twsr == SR2 ) twi_state = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_NS2 ] );  // otherwise, go to specified next state
      else               twi_state = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_NS ] );  // otherwise, go to specified next state
      }
   }
else                                  // we did not match the expected status register value for the current state, go to ERROR STATE
   {                                  // required otherwise we can jam here
   ES = pgm_read_byte ( &twi_states[ twi_state*TWI_ROW + TWI_ES ] );
   if ( ES !=0xFF ) twi_state = ES;   // if a valid new state, use it
   }
wait = 0x80;

     if ( twi_state== 0 ) twi_tx();  // TODO not sure it's the right place

#endif  // AT_SLAVE
if ( debug_page==1 ) { p = (unsigned char*)&dd_v[DDS_DEBUG + 0x08]; for ( ES = 0 ; ES <= TWI_C1 ; ES++ ) p[ES] = twi_tx_buf[ES]; }
if ( debug_page==2 ) { p = (unsigned char*)&dd_v[DDS_DEBUG + 0x08]; for ( ES = 0 ; ES <= TWI_C1 ; ES++ ) p[ES] = twi_rx_buf[ES]; }

if ( debug_page==2 ) dd_v[DDS_DEBUG + 0x06] = ra_correction;
if ( debug_page==2 ) dd_v[DDS_DEBUG + 0x07] = dec_correction;
//if ( (PINC & 0x20) == 0) aa[0]++;
//if ( (PINC & 0x10) == 0) aa[1]++;
}  // twitt()

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
d_TIMER0++;
TIMSK0 = 0;     // timer0 interrupt disable (prevent re-entrant)
sei();
display_next();
while ( TCNT1*100UL > F_CPU_K*9UL ) ; // if SP0 is about to start, wait... I dont want to be interruped in the next iterations  -> while(TCNT1 > 90% of target)
TCNT0 = 0;                            // Make sure I'm not called in the next iterations by my own AP0 interrupt
TIMSK0 = 1;     // timer0 interrupt enable
}

ISR(TIMER1_COMPB_vect)   // Clear the Step outputs
{
return;
}

#ifdef AT_MASTER
void close_loop(void)  // Clear the Step outputs
{
long temp;
// too long to complete !! set_digital_output(DO_PC0_RA_STEP,0);    // eventually, I should use my routines...flush polopu...
// too long to complete !! set_digital_output(DO_PC2_DEC_STEP,0);   // eventually, I should use my routines...flush polopu...
//FAST_SET_RA_STEP(0);
//FAST_SET_DEC_STEP(0);

if ( set_ra_armed )  // we are about to set the initial RA
   {
   if (dd_v[DDS_CUR_STAR_REQ] == dd_v[DDS_CUR_STAR])  
      {
      ra->pos_dem = ra->pos = dd_v[DDS_STAR_RA_POS];
      add_value_to_pos(-ra->pos_earth,&ra->pos_dem); 
      ra->pos_cor = ra->pos_hw = ra->pos_dem;                        

      dec->pos_cor = dec->pos_hw = dec->pos_dem = dec->pos = dd_v[DDS_STAR_DEC_POS]; 
      set_ra_armed=0;
   }  }

if ( motor_disable ) return; 

temp = ra->pos_hw-ra->pos_cor;
if ( temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_PC1_RA_DIR,0); // go backward
   //FAST_SET_RA_DIR(0);
   ra->direction=0x00;
   add_value_to_pos(-TICKS_P_STEP,&ra->pos_hw);
   ra->next=DO_PC2_RA_STEP;
   }
else if ( -temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_PC1_RA_DIR,1); // go forward
   //FAST_SET_RA_DIR(1);
   ra->direction=DO_PC3_RA_DIR;
   add_value_to_pos(TICKS_P_STEP,&ra->pos_hw);
   ra->next=DO_PC2_RA_STEP;
   }
else { ra->next=0; }

temp = dec->pos_hw-dec->pos_cor;
if ( temp >= (2*TICKS_P_STEP) )      // The DEC axis has 2 times less teeths 
   {
   // too long to complete !!set_digital_output(DO_PC3_DEC_DIR,0); // go backward
   //FAST_SET_DEC_DIR(0);
   dec->direction=0x00;
   add_value_to_pos(-(2*TICKS_P_STEP),&dec->pos_hw);
   dec->next=DO_PC0_DEC_STEP;
   }
else if ( -temp >= (2*TICKS_P_STEP) )
   {
   // too long to complete !!set_digital_output(DO_PC3_DEC_DIR,1); // go forward
   //FAST_SET_DEC_DIR(1);
   dec->direction=DO_PC1_DEC_DIR;
   add_value_to_pos( (2*TICKS_P_STEP),&dec->pos_hw);
   dec->next=DO_PC0_DEC_STEP;
   }
else { dec->next=0; }

//dd_v[DDS_DEBUG + 0x02]=ra->direction;
//dd_v[DDS_DEBUG + 0x0A]=dec->direction;

PORTC = ra->direction | dec->direction;    // I do this to optimize execution time

}
#endif

#ifdef AT_SLAVE
//ISR(TWI_vect)    // The slave used the interrupt vector ... I think that without setting the vecter, the TWI does not start
//{
//twitt();
//dd_v[DDS_DEBUG + 0x1D]++;
//}
#endif


//  IR CODE:
//  The code is in the delay between "1"
//  __-----__-__-_-_-__-__-__-   
//  At 10 Khz sampling with the Proscan remote, with an average of 3 iteration 
//  The Signal stays at 1 for 3~4 iterations ; only the start bit is longer than 3~4 iterations
//  Then goes to 0 for either 0x0B~0x0C : Logic 0    or either 0x15~0x16 : Logic 1
//  The  Start bit is a "1" for ~ 0x2A then a "0" for !0x2A
//  The code with Proscan is about 25 bits of information
#define KEY_OFF 1000
ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
unsigned long histo;
#ifdef AT_MASTER
//static char TMP1;
static unsigned char banding=0;
static char IR0,IR1,IR2,IR,l_IR,code_started=0,count_bit;
static short earth_comp=0;
static short earth_skip=0;
static short count_0;
static long loc_ir_code;
static unsigned short ir_key_off=0;  // limit the inputs to 4 per seconds

// These takes too long to complete !!!  set_digital_output(DO_PC0_RA_STEP ,ra->next);     // eventually, I should use my routines...flush polopu...
// These takes too long to complete !!!  set_digital_output(DO_PC2_DEC_STEP,dec->next);    // eventually, I should use my routines...flush polopu...
PORTC = ra->next | dec->next | ra->direction | dec->direction;    // I do this to optimize execution time   activate the STEP CLOCK OUTPUT
#else
char current_pwm=0;
char loc_cmd_state = cmd_state;
#endif

if ( ddll > 0 ) ddll--;  // test delay to reduce the responce speed of the slave

// this takes a lot of time . . . .: ssec = (ssec+1)%10000;   
ssec++; if ( ssec == 10000 ) ssec = 0;
if ( ssec == 0) dd_v[DDS_SECONDS]++;
d_TIMER1++;             // counts time in 0.1 ms

#ifdef AT_SLAVE
// Drive LEDs in 1 second PWM to indicate various states
//IF_NOT_CONDITION_PORT_BIT( dcay&0x01 , PORTD , DO_PD6_RED    );
//IF_NOT_CONDITION_PORT_BIT( dcay&0x02 , PORTD , DO_PD5_YELLOW );

if ( mosaic_twi&4 ) { IF_NOT_CONDITION_PORT_BIT( ssec&0x200 , PORTD , DO_PD6_RED    );  } // when in mosaic mode, flash the red led

if ( cmd_state!=0 ) loc_cmd_state +=3;
IF_NOT_CONDITION_PORT_BIT( (ssec&0x3FF)>0x40*loc_cmd_state , PORTD , DO_PD5_YELLOW );

// Motor Current setting:
if      ( motor_disable ){                    ;              }
else if ( effectiv_torq==3 )      {                    current_pwm=1; }
else if ( effectiv_torq==2 )      { if ( ssec < 7500 ) current_pwm=1; }
else if ( effectiv_torq==1 )      { if ( ssec < 5000 ) current_pwm=1; }
else                              { if ( ssec < 2000 ) current_pwm=1; }
if ( align_state==11 )  { IF_NOT_CONDITION_PORT_BIT( ssec&0x200  , PORTB , DO_PB7_GREEN    ); } // when doing polar align, flash the green
else                    { IF_NOT_CONDITION_PORT_BIT( current_pwm , PORTB , DO_PB7_GREEN    ); }
#endif

#ifdef AT_MASTER
//PORTB = TMP1++;   // to test TORQ pins
//if ( TMP1 & 0x10 ) PORTD |=  DO_PD5_DECAYA1;
//else               PORTD &= ~DO_PD5_DECAYA1; 
//if ( TMP1 & 0x08 ) PORTD |=  DO_PD6_DECAYA2;
//else               PORTD &= ~DO_PD6_DECAYA2; 
//if ( TMP1 & 0x04 ) PORTD |=  DO_PD7_DECAYB1;
//else               PORTD &= ~DO_PD7_DECAYB1; 


//////////////////////// Process IR ///////////////////////
IR2  = IR1;
IR1  = IR0;
/// - IR0  = is_digital_input_high(DI_PD2_REMOTE);
IR0  = (PIND & DI_PD2_REMOTE) !=0;
IR   = IR0 & IR1 & IR2;
if ( ir_key_off < KEY_OFF) ir_key_off++;
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
         if ( count_0 < 0x10 ) loc_ir_code|=1;   // set the "1" bit 
         count_0 = 0;
         }
      else count_bit++;
      }
   if ( count_0 > 0x40 ) //  || count_bit==0x1A) // code over
      {
      if ( dd_v[DDS_IR_COUNT] == l_ir_count && ir_key_off>=KEY_OFF )  // wait for bg to process the code
         {
         if ( count_bit == 0x001A )  // a complete code....
            {
            dd_v[DDS_IR_L_CODE]   = dd_v[DDS_IR_CODE];
            dd_v[DDS_IR_CODE]     = loc_ir_code;
            dd_v[DDS_IR_COUNT]++;
            }
         dd_v[DDS_DEBUG + 0x1F]=count_bit;
         }
      ir_key_off = 0;  // reset time since last valid code...
      count_0 = code_started = count_bit = loc_ir_code = 0;
      }
   }
   

if ( ! motor_disable )    //////////////////// motor disabled ///////////
   {
   ///////////// Process earth compensation
   // this takes a lot of time . . . .:earth_comp = (earth_comp+1)%EARTH_COMP;
   earth_comp++ ; if ( earth_comp == EARTH_COMP ) earth_comp = 0;
   if ( earth_comp == 0 && earth_tracking !=0 ) 
      {
      earth_skip++ ; 
      if ( earth_skip == EARTH_SKIP ) earth_skip = 0;
      else add_value_to_pos(TICKS_P_STEP,&ra->pos_earth);    // Correct for the Earth's rotation but skip every 1674
      }
   
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

   if      ( moving )                       minumum_torq = 2;  // If we are doing a GOTO or a SLEW, set the torq to a minimum of 75%
   else if ( earth_tracking || use_polar  ) minumum_torq = 1;  // If we are tracking , set the torq to a minimum of 50%
   else                                     minumum_torq = 0;  // No minimum required
   effectiv_torq = torq;
   if ( effectiv_torq < minumum_torq ) effectiv_torq = minumum_torq;

// dd_v[DDS_DEBUG + 0x0B] = minumum_torq;
// dd_v[DDS_DEBUG + 0x0C] = effectiv_torq;

//   dd_v[DDS_DEBUG + 0x12]=moving;
//   dd_v[DDS_DEBUG + 0x1A]=moving;

   if ( moving ) slew_cmd=goto_cmd=0;                   // command received

   if      ( banding & 1 )
      { // also to avoid overruns   // APPLY POLAR CORRECTION
      if ( ra_correction > loc_ra_correction )   { if ( ra_correction      - loc_ra_correction  > 100 ) ra_correction  -= 100; }
      else                                       { if ( loc_ra_correction  - ra_correction      > 100 ) ra_correction  += 100; }
      }
   else 
      {
      if ( dec_correction > loc_dec_correction ) { if ( dec_correction     - loc_dec_correction > 100 ) dec_correction -= 100; }
      else                                       { if ( loc_dec_correction - dec_correction     > 100 ) dec_correction += 100; }
      }
   }

// 40 times per seconds: update the position so that the background can use a snapshot of the position
if      ( banding == 0x30 ) 
   { if ( (twi_hold==0) && (twi_test==0) ) twi_pos[0]  = ra->pos; }
else if ( banding == 0x40 ) 
   { if ( (twi_hold==0) && (twi_test==0) ) twi_pos[1] = dec->pos; }

banding = banding+1;  // 256 leg banding ...thats ~40hz

close_loop();
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section terminates the interrupt routine and will help identify how much CPU time we use in the real-time interrupt
//   histo = TCNT1; 
//   histo *= 15 * 10;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
//   histo /= F_CPU_K;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
histo = TCNT1 >> 7;  // This is an approximation of the above because the division takes a lot of cycles ... about 40% of one iteration !
if ( histo > 15 ) histo=15;
dd_v[DDS_HISTO + histo]++;
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
// dont drive the pins TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
TCCR1A  = 0x03;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
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

#ifdef AT_SLAVE

#endif
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


/// align debug section
PROGMEM const char pgm_debug       []="Breakpoint #:";
void fake_align(short id,short rec_pos,long p_ra,long p_dec)
{
dd_v[DDS_CUR_STAR_REQ] = id;  // ask slave to get position # 62
wait(1,SEC);
goto_pgm_pos(id);
display_data((char*)console_buf,0,20,pgm_debug ,1 ,FMT_FP + FMT_CONSOLE + 8);   // case #1
while (  moving || goto_cmd || standby_goto ) wait(500,MSEC);  // wait for the end of displacement
display_data((char*)console_buf,0,20,pgm_debug ,2 ,FMT_FP + FMT_CONSOLE + 8);   // case #1
wait(1,SEC);
ra->pos_target  = p_ra;
dec->pos_target = p_dec;
goto_cmd = 1;
while (  moving || goto_cmd  ) wait(500,MSEC);
display_data((char*)console_buf,0,20,pgm_debug ,3 ,FMT_FP + FMT_CONSOLE + 8);   // case #1
wait(1,SEC); record_pos(rec_pos); twi_seq+=2; wait(1,SEC);
}



int main(void)
{
long iii;
///////////////////////////////// Init /////////////////////////////////////////
ra  = (AXIS*)&dd_v[DDS_RA];   // init pointers
dec = (AXIS*)&dd_v[DDS_DEC];  // init pointers
dd_v[DDS_CUR_STAR_REQ] = STARS_COORD_TOTAL-1;  // requested new star start with the last one

init_rs232();
init_disp();

ps("\033[2J");

//set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();

#ifdef AT_MASTER
   PRR   = 0x00;  // enable TWI and all other systems
   TWSR  = 0x01;  // Set the Prescaler to 64;
   TWBR  = 23;   // 4 x 50 = 200 thus TWI clock is 100khz
   TWAR  = 0x10;  // TWI Address 0x10 + 1 for General Call (Broadcasts)
   TWCR  = 0x44;  // TWEA & TWEN -> activate the address

   DDRB  = 0x3F; // p0:CLKOUT> p1:TORQA1> p2:TORQA2> p3:TORQB1> p4:TORQB2>   p5:DECAYB2>

// DDRC  = 0x3F; // set pins 0 1 2 3 of PORTC C as output     (logic 1 = output)    >> STEP AND DIR   plus TWI pins
   DDRC  = DO_PC4_TWI_SDA | DO_PC5_TWI_SCL | DO_PC0_DEC_STEP | DO_PC1_DEC_DIR | DO_PC2_RA_STEP | DO_PC3_RA_DIR ; 
   PORTC = 0x3F;         // Set outputs to 1...this is to avoid a glitch on the scope when monitoring TWI signals
   
// DDRD = 0x06; // set pins 1 and 2 of PORTD D as output     (logic 1 = output)    >> RS232 TX and DO DISABLE
   PORTD &= ~(DO_PD3_DISABLE);  // Start with motor disabled disabled and reset
   DDRD  = 0x1A; // set pins 1 and 3 and 4 of PORTD D as output     (logic 1 = output)    >> p0:RS232 rx p1:RS232 TX  p2:IR  p3: Enable  p4: TWI_START
#endif

#ifdef AT_SLAVE
   PRR   = 0x00;  // enable TWI and all other systems
   TWSR  = 0x01;  // Set the Prescaler to 64;
   TWBR  = 23;   // 4 x 50 = 200 thus TWI clock is 100khz
//   TWAMR = 0xFE;  // TWI Address 0x20 + 1 for General Call (Broadcasts)
   TWAR  = 0x20;  // TWI Address 0x20 + 1 for General Call (Broadcasts)
   TWCR  = 0x44;  // TWEA & TWEN -> activate the address  +1 for interrupt enable - in which case ISR(TWI_vect) must be defined

   PORTC = 0x30; // Set outputs to 1...this is to avoid a glitch on the scope when monitoring TWI signals

   DDRB  = DO_PB7_GREEN   ; // start as inputs :  | DO_PB0_LCD_B4  | DO_PB1_LCD_B5  | DO_PB2_LCD_B6  | DO_PB3_LCD_B7 ;
   DDRC  = DO_PC4_TWI_SDA | DO_PC5_TWI_SCL | DO_PC2_LCD_RS  | DO_PC1_LCD_RW  | DO_PC0_LCD_EE ;   // set pins 4 5 of PORTC C as output     (logic 1 = output)    >>  TWI pins
   DDRD  = DO_PD5_YELLOW  | DO_PD6_RED   ; 

#endif
sei();         //enable global interrupts

display_data((char*)console_buf,0,20,pgm_starting,d_ram,FMT_NO_VAL + FMT_CONSOLE + 8);

#ifdef DISPLAY_HELP
   #ifdef POLOLU
      while (console_special); /* wait for ready */ console_special = pololu;
   #else
      while (console_special); /* wait for ready */ console_special = dip328p;
   #endif
#endif

display_data((char*)console_buf,0,20,pgm_free_mem,d_ram,FMT_HEX + FMT_CONSOLE + 8);

for ( iii=0 ; iii<31 ; iii++ ) dd_v[DDS_DEBUG + iii] = iii * 0x100;

         motor_disable = 1;
         if ( motor_disable ) PORTD &= ~DO_PD3_DISABLE;  // set the pin to 0 V
         else                 PORTD |=  DO_PD3_DISABLE;


wait(2,SEC);
#ifdef AT_SLAVE
twi_enable = 1;  // slave starts first
#else
wait(2,SEC);  // make sure the master starts after the slave
twi_enable = 1;  // Master starts later
#endif

#ifdef AT_SLAVE
wait(1,SEC);
   TWAR  = 0x20;  // TWI Address 0x20 + 1 for General Call (Broadcasts)
   TWCR  = 0x44;  // TWEA & TWEN -> activate the address
wait(1,SEC);
   TWAR  = 0x20;  // TWI Address 0x20 + 1 for General Call (Broadcasts)
   TWCR  = 0x84;  // TWEA & TWEN -> activate the address
wait(1,SEC);
   TWAR  = 0x20;  // TWI Address 0x20 + 1 for General Call (Broadcasts)
   TWCR  = 0x44;  // TWEA & TWEN -> activate the address

#endif

d_now   = d_TIMER1;
//for(iii=0;iii<16;iii++) dd_v[DDS_HISTO + iii]=0;
//dd_v[DDS_SECONDS] = ssec = 0;

//--motor_disable = 0;   // Stepper motor enabled...
//--set_digital_output(DO_PD3_DISABLE  ,motor_disable);   

// Corrected star positions with a polar error that I introduced
/*
Free Memory:00000226               
Recorded position: 00000002        
RA :F433C764                       
DEC:031455D5                       
ref:00000008                       
Recorded position: 00000001        
RA :0C77551F                       
DEC:030A8BC5                       
ref:00000009                       
Recorded position: 00000003        
RA :0C6BD457                       
DEC:F35CD11C                       
ref:0000000B                       
Recorded position: 00000004        
RA :F431271C                       
DEC:F36EFB73                       
ref:0000000C                       
Recorded position: 00000007        
RA :3C629AA5                       
DEC:0510E764                       
ref:00000001                       
Recorded position: 00000008        
RA :353C32E1                       
DEC:F1A45541                       
ref:00000002                       
Recorded position: 00000009        
RA :3739E783                       
DEC:046A5F79                       
ref:00000004                      

Slave Result:  ... to be compared with previous system all in one CPU
Data: 7FF6A59A  0.99971456 
Data: 020E0355  0.01605264 
Data: FDBC30AD  -.01769439 
Data: FDF2C11F  -.01602922 
Data: 7FFBC13E  0.99987044 
Data: 002FFD4C  0.00146451 
Data: 0244814E  0.01771560 
Data: FFD95173  -.00118047 
Data: 7FFAD5AF  0.99984236 
*/
#ifdef AT_MASTER_DISABLED
twi_test = 1;
wait(500,MSEC);
twi_pos[0]    = 0xF433C764;
twi_pos[1]    = 0x031455D5;
DDS_CUR_STAR_req    = 0x08;  // 19 psc
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0x0C77551F;
twi_pos[1]    = 0x030A8BC5;
DDS_CUR_STAR_req    = 0x09;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0x0C6BD457;
twi_pos[1]    = 0xF35CD11C;
DDS_CUR_STAR_req    = 0x0B;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0xF431271C;
twi_pos[1]    = 0xF36EFB73;
DDS_CUR_STAR_req    = 0x0C;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0x3C629AA5;
twi_pos[1]    = 0x0510E764;
DDS_CUR_STAR_req    = 0x01;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0x353C32E1;
twi_pos[1]    = 0xF1A45541;
DDS_CUR_STAR_req    = 0x02;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position

twi_pos[0]    = 0x3739E783;
twi_pos[1]    = 0x046A5F79;
DDS_CUR_STAR_req    = 0x04;
wait(500,MSEC); twi_seq+=2; wait(500,MSEC);   // Tell the slave to record that position
wait(500,MSEC); twi_seq=255; wait(500,MSEC);   // Tell the slave to do a Polar align
twi_test=0;
#endif


#ifdef AT_MASTER

while ( align_state==0 ) wait(500,MSEC);  // wait for the first position

//----
//fake_align(62,10,0x87FBEF3A,0xC7880FA4);
//fake_align(71,11,0xA01A6DE7,0x07EA89E9);
//fake_align(45,12,0x5DB37BDA,0x0C337D63);
//fake_align(41,13,0x5148CC6E,0x0DC58F81);
//fake_align(51,14,0x6D180F80,0x0709A4F3);

//fake_align(41,10,0x5105BE60,0x0C72900B);
//fake_align(45,11,0x5D9BB04A,0x0B16367F);
//fake_align(69,12,0x949C1600,0x019E07F2);
//fake_align(73,13,0xA604930A,0x042CE999);
//fake_align(51,14,0x6D180F80,0x061D35F0);

// while ( align_state!=11 ) wait(500,MSEC);  // wait for the first
// wait(20,SEC);
// 
// ////////// end
// dd_v[DDS_CUR_STAR_REQ] = 51;  // ask slave to get position # 62
// display_data((char*)console_buf,0,20,pgm_debug ,100 ,FMT_FP + FMT_CONSOLE + 8);   // case #1
// wait(4,SEC);
// goto_pgm_pos(51);
// display_data((char*)console_buf,0,20,pgm_debug ,101 ,FMT_FP + FMT_CONSOLE + 8);   // case #1

#endif


#ifdef AT_SLAVE

/// wait(5,SEC);
/// loc_dec_correction = TICKS_P_DEG;   // to test the polar correctin
/// wait(50,SEC);
/// loc_dec_correction = 0;


// Test the LCD display
// Note: I'm an idiot and wired B0 to B3 instead of B4 to B7 which are the pins used in 4 bits mode
//
#ifdef ASASAS

//////////////////////////////////////////////////////////////////////////////////////////////////////

// Wait for the busy flag to clear on a 4-bit interface
// This is necessarily more complicated than the 8-bit interface
// because E must be strobed twice to get the full eight bits
// back from the LCD, even though we're only interested in one
// of them.
void busyWait()
{
uint8_t temp_ddr, data;

temp_ddr = DDRD; // Save our DDR information 
DDRD &= ~(LCD_PORTD_MASK); // Set up the data DDR for input 
// Set up RS and RW to read the state of the LCD's busy flag
LCD_RS_E_PORT &= ~(1 << LCD_RS);
LCD_RW_PORT |= (1 << LCD_RW);

do
   {
   LCD_RS_E_PORT |= (1 << LCD_E); // Bring E high 
   delayMicroseconds(1); // Wait at least 120ns (1us is excessive) 
   data = PIND & LCD_PORTD_MASK; // Get the data back from the LCD 
   // That excessive delay means our cycle time on E cannot be
   // shorter than 1000ns (500ns being the spec), so no further
   // delays are required

   LCD_RS_E_PORT &= ~(1 << LCD_E); // Bring E low 
   delayMicroseconds(1); // Wait a small bit 
   // Strobe out the 4 bits we don't care about:

   LCD_RS_E_PORT |= (1 << LCD_E); // Bring E high 
   delayMicroseconds(1); // Wait at least 120ns (1us is excessive) 
   LCD_RS_E_PORT &= ~(1 << LCD_E); // Bring E low
}
while (data & (1 << LCD_BF));

// To reach here our busy flag must be zero, meaning the LCD is free
DDRD = temp_ddr;
// Restore our DDR information
}



// Send four bits out the 4-bit interface.  This assumes the busy flag
// is clear, that our DDRs are all set, etc.  Basically all it does is
// line up the bits and shove them out the appropriate I/O lines.
void sendNibble(unsigned char nibble)
{
PORTB = (PORTB & ~LCD_PORTB_MASK) | LCD_PORTB_DATA(nibble);
PORTD = (PORTD & ~LCD_PORTD_MASK) | LCD_PORTD_DATA(nibble);

// At this point the four data lines are set, so the Enable pin 
// is strobed to let the LCD latch them.

LCD_RS_E_PORT |= (1 << LCD_E); // Bring E high 
delayMicroseconds(1); // Wait => 450ns (1us is excessive) 
LCD_RS_E_PORT &= ~(1 << LCD_E); // nibble on the falling edge of E 
delayMicroseconds(1); 
// Dropping out of the routine will take at least 10ns, the time
// given by the datasheet for the LCD controller to read the
// nibble on the falling edge of E

}


// Send either data or a command on a 4-bit interface

void send(unsigned char data, unsigned char rs)
{
unsigned char temp_ddrb, temp_portb, temp_ddrd, temp_portd;

init();  // initialize the LCD if we haven't already

// Wait until the busy flag clears
busyWait();

// Save our DDR and port information
temp_ddrb = DDRB;
temp_portb = PORTB;
temp_ddrd = DDRD;
temp_portd = PORTD;

// Clear RW and RS
LCD_RS_E_PORT &= ~(1 << LCD_RS);
LCD_RW_PORT &= ~(1 << LCD_RW);

// Set RS according to what this routine was told to do
LCD_RS_E_PORT |= (rs << LCD_RS);

// Set the data pins as outputs
DDRB |= LCD_PORTB_MASK;
DDRD |= LCD_PORTD_MASK;

// Send the high 4 bits
sendNibble(data >> 4);

// Send the low 4 bits
sendNibble(data & 0x0F);

// Restore our DDR and port information
PORTD = temp_portd;
DDRD = temp_ddrd;
PORTB = temp_portb;
DDRB = temp_ddrb;
}

void init2()
{
        // Set up the DDR for the LCD control lines
        LCD_RS_E_DDR |= (1 << LCD_RS) | (1 << LCD_E);
        LCD_RW_DDR |= (1 << LCD_RW);

        delay(20); // Wait >15ms 
        send_cmd(0x30); // Send 0x3 (last four bits ignored) 
        delay(6); // Wait >4.1ms 
        send_cmd(0x30); // Send 0x3 (last four bits ignored) 
        delay(2); // Wait >120us 
        send_cmd(0x30); // Send 0x3 (last four bits ignored) 
        delay(2); // Wait >120us 
        send_cmd(0x20); // Send 0x2 (last four bits ignored)  Sets 4-bit mode 
        delay(2); // Wait >120us 
        send_cmd(0x28); // Send 0x28 = 4-bit, 2-line, 5x8 dots per char 
        // Busy Flag is now valid, so hard-coded delays are no longer required.  
        send_cmd(0x08); // Send 0x08 = Display off, cursor off, blinking off 
        send_cmd(0x01); // Send 0x01 = Clear display 
        send_cmd(0x06); // Send 0x06 = Set entry mode: cursor shifts right, don't scroll 
        send_cmd(0x0C); // Send 0x0C = Display on, cursor off, blinking off
}
// disable LCD for now  #endif
 
unsigned char lcd_state=0;
while (1)
   {
   // PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS , DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS);
   unsigned char pvalh,pvall;

   if ( lcd_state>3 )
      {
      wait(1,MSEC); 
      DDRB  = DO_PB7_GREEN   ; // start as inputs :  | DO_PB0_LCD_B4  | DO_PB1_LCD_B5  | DO_PB2_LCD_B6  | DO_PB3_LCD_B7 ;    // Port set as input
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,               DO_PC1_LCD_RW              );         
      wait(1,MSEC); 
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS , DO_PC0_LCD_EE|DO_PC1_LCD_RW              );           // Clock E _/-\_
      wait(1,MSEC); 
      pvalh = PINB;
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,               DO_PC1_LCD_RW              );         
      wait(1,MSEC); 
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS , DO_PC0_LCD_EE|DO_PC1_LCD_RW              );           // Clock E _/-\_
      wait(1,MSEC); 
      pvall = PINB;
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,               DO_PC1_LCD_RW              );         
      dd_v[DDS_DEBUG + 0x18] = (((long)pvalh)<<16) | pvall;
      dd_v[DDS_DEBUG + 0x19] = lcd_state;
      }

   if ( (lcd_state<=3) || ((pvalh & DO_PB3_LCD_B7) == 0) ) // if not busy flag
      {
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,               0                          );         
      DDRB  = DO_PB7_GREEN | DO_PB0_LCD_B4  | DO_PB1_LCD_B5  | DO_PB2_LCD_B6  | DO_PB3_LCD_B7 ;                           // Port set as output
      if ( lcd_state< 3 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,    DO_PB1_LCD_B5           );         // Write 0x0010xxxx  :  which will set the mode to 4 bit
      if ( lcd_state==3 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,    DO_PB1_LCD_B5           );         // Write 0x0010xxxx  :  which will set the mode to 4 bit
      if ( lcd_state==4 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,                0           );         // Write 0x0000xxxx  :  
      if ( lcd_state==5 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,                0           );         // Write 0x0000xxxx  :  

      wait(1,MSEC); 
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS , DO_PC0_LCD_EE                            );         // Clock E _/-\_
      wait(1,MSEC); 
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,                              0           );         //
      if ( lcd_state==3 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,    DO_PB3_LCD_B7           );         // Write 0xxxxx1000  :  which will set 2 line 5x8 dots
      if ( lcd_state==4 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 , DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4);         // Write 0xxxxx1111  :  Display on Curson, flashing
      if ( lcd_state==5 )  PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 ,               DO_PB2_LCD_B6|DO_PB1_LCD_B5              );         // Write 0xxxxx1111  :  Display on Curson, flashing
   
      if ( lcd_state>2  ) 
         {
         wait(1,MSEC); 
         PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS , DO_PC0_LCD_EE                            );         // Clock E _/-\_
         wait(1,MSEC); 
         PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,                              0           );         //
         DDRB  = DO_PB7_GREEN   ; // start as inputs :  | DO_PB0_LCD_B4  | DO_PB1_LCD_B5  | DO_PB2_LCD_B6  | DO_PB3_LCD_B7 ;    // Port set as input
         }
      else  
         wait(4,MSEC);


      if ( lcd_state==5 )  
         {
         lcd_state=0;
         PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 , DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 );
         wait(50,MSEC); 
         }
      else lcd_state++; 
      }
   else 
      {
      PORT_MASK_BIT(PORTB,DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 , DO_PB3_LCD_B7|DO_PB2_LCD_B6|DO_PB1_LCD_B5|DO_PB0_LCD_B4 );
      PORT_MASK_BIT(PORTC,DO_PC0_LCD_EE|DO_PC1_LCD_RW|DO_PC2_LCD_RS ,               DO_PC1_LCD_RW              );         
      DDRB  = DO_PB7_GREEN   ; // start as inputs :  | DO_PB0_LCD_B4  | DO_PB1_LCD_B5  | DO_PB2_LCD_B6  | DO_PB3_LCD_B7 ;    // Port set as input
      wait(100,MSEC);
      lcd_state=0;
      }
   }
#endif



  #if(TEST_POLAR)
  //            POLAR_RA      POLAR_DEC      TIME_SHIFT_RA
  // test_polar(0*TICKS_P_DEG,10*TICKS_P_DEG,0*TICKS_P_DEG);
  // test_polar(45*TICKS_P_DEG,10*TICKS_P_DEG,45*TICKS_P_DEG);
     test_polar(10*TICKS_P_DEG,10*TICKS_P_DEG,10*TICKS_P_DEG);
  #endif
#endif

/*
Polar Correction Matrix:
Pass #:+001                         
Error :006D4CA2                     
Hour   :00h00m00.00s                
Declin :(N) 01°00'00.0"             
Hour   :01h00m00.00s                
Declin :(N) 01°00'00.0"             
Pass #:+002                         
Error :006C3772                     
Hour   :00h37m30.00s                
Declin :(N) 01°07'30.0"             
Hour   :00h07m30.00s                
Declin :(N) 00°07'30.0"             
Pass #:+003                         
Error :006C3356                     
Hour   :00h36m33.75s                
Declin :(N) 01°06'33.7"             
Hour   :00h00m56.25s                
Declin :(N) 00°00'56.2"             
Pass #:+004                         
Error :006C331E                     
Hour   :00h35m51.56s                
Declin :(N) 01°06'19.6"             
Hour   :00h00m07.03s                
Declin :(N) 00°00'07.0"             
Pass #:+005                         
Error :006C331A                     
Hour   :00h35m54.19s                
Declin :(N) 01°06'20.5"             
Hour   :00h00m00.87s                
Declin :(N) 00°00'00.8"             
Polar Matrix: 7FFFD9F4  0.99999546  
Polar Matrix: 0000F0B9  0.00002869  
Polar Matrix: 0062A78D  0.00301069  
Polar Matrix: 0000F0B9  0.00002869  
Polar Matrix: 7FFA0BF8  0.99981832  
Polar Matrix: FD8F6A37  -.01906082  
Polar Matrix: FF9D5873  -.00301069  
Polar Matrix: 027095C9  0.01906082  
Polar Matrix: 7FF9E5F8  0.99981379  
FFFF73B4  00037818  FFEA3660 

*/

#ifdef ASFAS 
   
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
// go the other way around
//ra->pos_target  = pgm_read_dword(&pgm_stars_pos[cur_star*2+0]);
//dec->pos_target = pgm_read_dword(&pgm_stars_pos[cur_star*2+1]);
//dd_v[DDS_DEBUG + 0x10]=ra->pos_target;
//dd_v[DDS_DEBUG + 0x18]=dec->pos_target;
//goto_cmd = 1;  
//while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);


while (1 ) wait(1,SEC);  // This last while is important, because it calls display next....which is AP0
return 0;
}


/*
$Log: telescope2.c,v $
Revision 1.92  2012/04/18 03:35:50  pmichel
few nice improvements:
-goto here (play play)
-star 98 (last one) returns 0,0 and returns the polar error once aligned

Revision 1.91  2012/04/15 18:28:36  pmichel
########## Ready for mosaic ##########
reduced mechanical friction correction

Revision 1.90  2012/04/15 18:11:45  pmichel
Mosaic is working . . .
fg

Revision 1.89  2012/04/14 14:35:48  pmichel
#### Milestone ####
the alignment is complete  (but the fake align needs to be removed)

Revision 1.88  2012/04/14 13:58:23  pmichel
This version should be 100% for the polar correction
but I will simplify it one bit.

Revision 1.87  2012/04/14 12:27:41  pmichel
RA works from 45 deg to -45 deg

Revision 1.86  2012/04/14 11:47:47  pmichel
DEC probably 100% ok now

Revision 1.85  2012/04/14 11:10:44  pmichel
found a big error with fp_sin() -> a factor had to be adjusted when I changed the nb of tick per day
some errors remains in the arcsin
I will change all the arc sin methods in the next version

Revision 1.84  2012/04/13 15:50:39  pmichel
First version Master/Slave where the polar align works
there is still a little glitch in the arcsin arccos...we see it when slewing
need to output better feedback

Revision 1.83  2012/04/12 21:18:40  pmichel
back home . . . restarting the alignment tests

Revision 1.82  2012/03/31 05:15:18  pmichel
After power up,
we can slew around
when we do PLAY X X X   it tells the controler where we are pointing
and thus the controler sets the current position to the one where we are pointing
we can do this a few times
once we do RECORD ANTENA X  which records the first corrected star position
then we switch mode to "star correction"

Revision 1.81  2012/03/28 07:48:26  pmichel
Star name and position sent from slave to master

Revision 1.80  2012/03/28 07:31:51  pmichel
Using monkey method to find if the star string should be drawn
Need to remove complicated logic on slave

Revision 1.79  2012/03/11 11:00:09  pmichel
Slave can use the new Star format
Remains to transmit to Master for display

Revision 1.78  2012/03/09 11:19:28  pmichel
Few fixes, Star names are ok on the slave side...

Revision 1.77  2012/03/07 10:05:01  pmichel
Ready to try to compile

Revision 1.76  2012/02/21 01:10:38  pmichel
Found big bug... the RA and DEC axis dont have 3 and 6 deg per turn
which would mean 120 and 60 teeth for the gears
instead, the geers seems to have 130 and 65 teeth

Revision 1.75  2012/02/12 20:50:27  pmichel
Implemented minimum torq for movement and tracking

Revision 1.74  2012/02/12 18:14:13  pmichel
### Important milestone
Fixed torq problem
Reversed RA axis to match earth's rottation
Tested LEDs and LCD outputs
First test outside at -20 !

Revision 1.73  2012/01/29 05:21:50  pmichel
Last version on SUSE11b... both laptops can now compile

Revision 1.72  2012/01/25 04:05:51  pmichel
Slave does polar align
Slave: 25.8K of flash used
Master 18.8K of flash used
I think I might put the star catalog on the master...

Revision 1.71  2012/01/23 03:32:29  pmichel
reordering

Revision 1.69  2012/01/22 21:33:47  pmichel
Not a major improvement, down to 18000 bytes (from 18400)
but the important thing is that now both IR and Keyboard go through the same logic

Revision 1.68  2012/01/22 19:34:31  pmichel
Hex : 18404 bytes
About to change the way I process IR commands, in hope of freeying flash

Revision 1.67  2012/01/22 19:07:03  pmichel
Found why the output gave the impression that only 20 bytes were sent
all is of, 32 bytes are exchanged . . .

Revision 1.66  2012/01/21 17:06:06  pmichel
########################
Nice Split, both Master and Slaves uses about 19k of Flash
Slaves has all the Polar Math minus the Motor logic
Master has the Motor logic minus the Polar Math
>>> Remains to add the logic so that the Master asks for a Pola
>>> Remains to clean-up the code and but all the different parts together: Display / Motor / Polar Math

Revision 1.65  2012/01/21 03:39:50  pmichel
This is exponential groath,
from zero progress in 2 weeks to a state when I feel the need to checkin every 5 minutes because so much progress was made
-> Exchanging 32 bytes
-> Telescope pos, debug_page, current star, all sent to slave

Revision 1.64  2012/01/21 03:10:27  pmichel
Few display changes to debug TWI

Revision 1.63  2012/01/21 02:47:33  pmichel
#################################3
Master and Slave Write works in sequence
Since this is becoming more and more complicated, added a "debug_page" that decides what is shown in the debug section

Revision 1.62  2012/01/21 02:13:08  pmichel
Tested Master WRITE again

Revision 1.61  2012/01/21 01:44:20  pmichel
#####################################################################################
SLAVE WRITE works - Ready to combine MASTER and SLAVE WRITE

Revision 1.60  2012/01/21 00:05:28  pmichel
##################################################3
Finally, The Slave Transmit works in continous

Revision 1.59  2012/01/19 06:15:39  pmichel
Feedback started to work, lots of stuff hardcoded...

Revision 1.58  2012/01/19 04:03:45  pmichel
TWI Tx works well,
Transmitting:
Current Star
Current Pos
Store Corrected pos

Now I need to ask for feedback from slave...

Revision 1.57  2012/01/15 20:58:42  pmichel
Slave now uses a state machine aswell,
Little tuning required, because the checksum does not come through

Revision 1.56  2012/01/15 20:44:55  pmichel
Corretions tested ok:
first send the Byte count, then RXID, then TXID, then DATA, then Checksum

Revision 1.55  2012/01/15 05:55:42  pmichel
Hum, lots of success in a short time
the Fucking short version works almost first shot
I transmit 9 bytes, and my rate doubbled
I can still get stuck on startup

Revision 1.54  2012/01/14 22:28:09  pmichel
Found big problem,
I was storing the status before reading the TWI done flag
so when operation done, the status I was using was old
because by the time I stored the status and read the int
the status changed
#### Very good baseline ###
ready to optimise

Revision 1.53  2012/01/14 21:00:28  pmichel
Second fast version,
lots of glitches seems to remain
but it's time to make a small state machine

Revision 1.52  2012/01/14 05:00:29  pmichel
Quite fast version
sending ~ 0x1000 bytes per second
sometimes I get stuck in state 6F

Revision 1.51  2012/01/13 21:48:28  pmichel
TWI working in polling

Revision 1.50  2012/01/13 21:30:55  pmichel
small improvement

Revision 1.49  2012/01/13 20:50:21  pmichel
First BYTE transmitted via TWI
ouf...

Revision 1.48  2012/01/03 22:18:20  pmichel
Small patch to correct the polar error before the goto

Revision 1.47  2012/01/03 21:07:05  pmichel
### DONE ###
It finally works
New star positions can be entered
Polar error is calculated
Polar is used to correct demanded position
############
The only drawback is that the correction is done off-line
and inhibitted while GOTO are in progress
so the friction removal of the GOTO has no effect when the correction takes effect after the move
the only solution is to calculate the correction before the move, apply the correction, then start the GOTO
....Or change the position method completely and work in h/w position all the time...correct only displayed position

Revision 1.46  2012/01/03 07:16:26  pmichel
I think the delta RA/DEC parameter will work well
remains:
mode to disactivate polar correction
methor to slowly activate/disactivate polar

Revision 1.45  2012/01/03 05:15:44  pmichel
saved 1K if Flash by optimizing the display() function calls that sends to sonsole

Revision 1.44  2012/01/03 00:41:11  pmichel
I think the Polar correction works now
But,
I'm stuck with no FLASH left and a result in X Y Z
I need to convert in delta-TICKS on both axis

Revision 1.43  2012/01/02 22:47:24  pmichel
Matrix tested ok,
now even better I fix 2 issues properly, and at the same time

Revision 1.42  2012/01/02 21:05:31  pmichel
Milestone:
Matrix seems to be ok
Note:
The rotation is about the X axis,
and turns from Y axis towards Z axis

Revision 1.41  2012/01/02 20:26:48  pmichel
Polar correction in progress, about to test rotation matrix to be sure

Revision 1.40  2011/12/31 21:09:17  pmichel
Polar Error seems to start working
I need to change the way I scan for the solution

Revision 1.39  2011/12/31 18:23:25  pmichel
Before starting FMT_FP

Revision 1.38  2011/12/30 22:26:20  pmichel
Debut of the polar correction s/w

Revision 1.37  2011/12/29 22:49:41  pmichel
More decoding, plus improved IR decodong

Revision 1.36  2011/12/29 21:08:55  pmichel
Complex decoding well under way
the only issue is the IR decoding, I need to detect key-off

Revision 1.35  2011/12/28 23:29:58  pmichel
Completed the optimization
plus removed the second interrupt handler
now the histogram really shows everything that happens in SP0

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

