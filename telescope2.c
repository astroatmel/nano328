/*
$Author: pmichel $
$Date: 2011/12/15 05:39:46 $
$Id: telescope.c,v 1.19 2011/12/15 05:39:46 pmichel Exp pmichel $
$Locker: pmichel $
$Revision: 1.19 $
$Source: /home/pmichel/project/telescope/RCS/telescope.c,v $

TODO:
- add a panic button that stops a movement (without the need to reset and loose everything)
- add a way to change the direction of movement, when going to far positions, the tube might start to turn the wring way (shortedt path)
- process RETURN as command complete
- Display Galaxy and Input text only on change
- decode modes:  GOTO / MOVEMENT / DIRCTORY
- decode command directory_mode       [SEARCH]              (DIRECTORY MODE : search in the star catalog)
- decode command next_star            [VOL +-]              (DIRECTORY MODE)
- decode command next_constellation   [CH  +-]              (DIRECTORY MODE) 
- decode command go_star              [PLAY]                (DIRECTORY MODE : go to the selected star) 
- decode command set_star             [RECORD]              (DIRECTORY MODE : the current H/W position is the corrected position for the active star) 
- decode command accel_mode           [SPEED + use arrows]  (MOVEMENT MODE : slew position)
- decode command stop                 [OK]                  (MOVEMENT MODE : slew to a stop)
- decode command next_star            [VOL +-]              (MOVEMENT MODE : slew to the next start)
- decode command next_constellation   [CH  +-]              (MOVEMENT MODE : slew to the next start in the next constellation) 
- decode command go_mode              [GO BACK]             (GOTO MODE : enter manual coordinate)
- decode command go_ra                [VOL+- + numbers]     (GOTO MODE : manual goto ex: CH+ 0130 means : North 1 deg 30 minutes 00.00 sec)
- decode command go_dec               [CH+- + numbers]      (GOTO MODE)
- decode IR
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
#define DI_REMOTE        IO_D7
#define DO_DISABLE       IO_D4

#define DO_RA_DIR        IO_C3  // 0x11   17
#define DO_RA_STEP       IO_C2  // 0x10   16
#define DO_DEC_DIR       IO_C1  // 0x0F   15
#define DO_DEC_STEP      IO_C0  // 0x0E   14

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
#define DEAD_BAND        (147767296)                        // 0x8CEC000      // This is 0x100000000 - TICKS_P_DAY = 147767296
#define TICKS_P_STEP     (6*360)                            // 2160           // 2160 which is easy to sub divide without fractions
#define TICKS_P_DAY      (9600UL*200UL*TICKS_P_STEP      )  // 4147200000     // One day = 360deg = 9600*200 micro steps
#define TICKS_P_DEG      (9600UL*200UL*TICKS_P_STEP/360UL)  // 11520000       // One day = 360deg = 9600*200 steps
#define TICKS_P_180_DEG  (9600UL*200UL*TICKS_P_STEP/2UL  )  // 2073600000     // One day = 360deg = 9600*200 micro steps
#define TICKS_P_DEG_MIN  (TICKS_P_DEG/60UL)                 // 192000         // 
#define TICKS_P_DEG_SEC  (TICKS_P_DEG/3600UL)               // 3200           // 
#define TICKS_P_HOUR     (9600UL*200UL*TICKS_P_STEP/24UL)   // 172800000      // 
#define TICKS_P_MIN      (TICKS_P_HOUR/60UL)                // 2880000        // 
#define TICKS_P_SEC      (TICKS_P_HOUR/3600UL)              // 48000          // 

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

typedef struct   // those values are required per axis to be able to execute goto commands
   {             // I use a structure so that both DEC and RA axis can use the exact same routines
   long  pos_initial;      // units: ticks
   long  pos_target;       // units: ticks
   long  pos_delta;        // units: ticks (it's the required displacement: pos_target-pos_initial)
   long  pos;              // units: ticks (this is the current sky position)
   long  pos_dem;          // units: ticks (demanded position = sky position + Earth compensation)
   long  pos_cor;          // units: ticks (corrected demmanded position using polar error)
   long  pos_hw;           // units: ticks (current position of the tube, this value should be the same as the corrected position) 
   long  pos_earth;        // units: ticks (it's the calculation of the ticks caused by the earth's rotation)
   long  pos_part1;        // units: ticks (it's ths total amount of displacement during acceleration)
   long  speed;            // units: ticks per iteration
   long  last_high_speed;  // units: ticks per iteration 
   char  accel;            // units: nanostep per iteration per iteration
   char  accel_seq;        // no units (it's the sequence counter)
// char  goto_cmd;         // no units (it's to tell the function to start a goto command)
   char  state;            // no units                     
   unsigned char  next;             // no units (should there be a step at the next iteration ? )                    
   unsigned char  direction;        // no units (which direction ? )                    
   } AXIS;

AXIS ra;
AXIS dec;


enum 
   {
   FMT_NO_VAL,
   FMT_HEX,
   FMT_DEC,   
   FMT_HMS,   
   FMT_NS,   
   FMT_EW,   
   FMT_RA,   
   PGM_POLOLU,
   PGM_DIPA328P, 
   PGM_STR, 
   FMT_STR, 
   FMT_ASC
   };

PROGMEM const char NLNL[]="\012\015";
PROGMEM const char pololu[]={"\
                ------------------------------\012\015\
           M1B  < 01                      24 >  VCC\012\015\
           M1A  < 02                      23 >  GND\012\015\
           PB0  < 03   IO/CLK/TIMER       22 >  M2BN\012\015\
           PB1  < 04   IO/PWM             21 >  M2A\012\015\
10Khz Out  PB2  < 05   IO/PWM     AI/IO   20 >  PC5 (ADC5/SCL)\012\015\
           PB4  < 06   IO         AI/IO   19 >  PC4 (ADC4/SDA)\012\015\
           PB5  < 07   IO         AI/IO   18 >  PC3 (ADC3) RA_DIR\012\015\
DI RX232   PD0  < 08   IO         AI/IO   17 >  PC2 (ADC2) RA_STEP\012\015\
DO TX232   PD1  < 09   IO         AI/IO   16 >  PC1 (ADC1) DEC_DIR\012\015\
           PD2  < 10   IO         AI/IO   15 >  PC0 (ADC0) DEC_STEP\012\015\
DISABLE    PD4  < 11   IO         AI      14 >      (ADC6) BATTERY MONITOR\012\015\
IR REMOTE  PD7  < 12   IO                 13 >  PC6 (RESET)\012\015\
                ------------------------------\012\015\
"};

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

#define STAR_NAME_LEN 23
short cur_star=2,l_cur_star;   // currently selected star
PROGMEM const char pgm_stars_name[]   =  // format: Constellation:Star Name , the s/w will use the first 2 letters to tell when we reach the next constellation
                    {
                     "Orion:Betelgeuse (Red)\0"\
                     "Orion:Rigel (Blue)    \0"\
                     "Pisces:19psc          \0"\
                     "Lyra:Vega             \0"\
                     "Gynus:Deneb           \0"\
                     "Gynus:Sadr            \0"\
                    };
PROGMEM const unsigned long pgm_stars_pos[] =    // Note, the positions below must match the above star names
                    {                  //   RA                                                DEC
                      ( 5*TICKS_P_HOUR+55*TICKS_P_MIN+10.3*TICKS_P_SEC), (  7*TICKS_P_DEG+24*TICKS_P_DEG_MIN+25  *TICKS_P_DEG_SEC),     // Orion:Betelgeuse (Red)
                      ( 5*TICKS_P_HOUR+14*TICKS_P_MIN+32.3*TICKS_P_SEC), (  8*TICKS_P_DEG+12*TICKS_P_DEG_MIN+ 6  *TICKS_P_DEG_SEC),     // Orion:Rigel (Blue)
                      (23*TICKS_P_HOUR+46*TICKS_P_MIN+23.5*TICKS_P_SEC), (  3*TICKS_P_DEG+29*TICKS_P_DEG_MIN+12  *TICKS_P_DEG_SEC),     // Pisces:19psc
                      (18*TICKS_P_HOUR+36*TICKS_P_MIN+56.3*TICKS_P_SEC), ( 38*TICKS_P_DEG+47*TICKS_P_DEG_MIN+ 3  *TICKS_P_DEG_SEC),     // Lyra:Vega
                      (20*TICKS_P_HOUR+41*TICKS_P_MIN+25.9*TICKS_P_SEC), ( 45*TICKS_P_DEG+16*TICKS_P_DEG_MIN+49  *TICKS_P_DEG_SEC),     // Gynus:Deneb
                      (20*TICKS_P_HOUR+22*TICKS_P_MIN+13.7*TICKS_P_SEC), ( 40*TICKS_P_DEG+15*TICKS_P_DEG_MIN+24  *TICKS_P_DEG_SEC),     // Gynus:Sadr
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
long seconds=0,l_seconds;
volatile long d_debug[32], l_debug[32];
unsigned short histogram[16],l_histogram[16],histo_sp0=1; // place to store histogram of 10Khz interrupt routine

long l_ra_pos1,l_ra_pos2,l_dec_pos,l_ra_pos_hw,l_ra_pos_hw1,l_ra_pos_hw2,l_dec_pos_hw;
long l_ra_pos_cor1,l_ra_pos_cor2,l_dec_pos_cor;
long g_goto_state;
long g_ra_pos_part1;

volatile char moving=0;
volatile char motor_disable=1;       // Stepper motor Disable
volatile char goto_cmd=0;            // 1 = goto goto_ra_pos


/////////////////////////////////////////// RS232 INPUTS ///////////////////////////////////////////////////////////
#define RS232_RX_BUF 32
unsigned char rs232_rx_buf[RS232_RX_BUF] = {0};
unsigned char rs232_rx_idx=0;                          // <- always points on the NULL
unsigned char l_rs232_rx_idx=0;                       // 
volatile unsigned char rs232_rx_clr=0;               // set by foreground to tell ap0c0 that it can clear the buffer
/////////////////////////////////////////// RS232 OUTPUTS //////////////////////////////////////////////////////////
#define RS232_TX_BUF 64
unsigned char rs232_tx_buf[RS232_TX_BUF] = {0}; // buffer that contains the proper thing to display based on current d_task
unsigned char rs232_tx_idx=0;
///////////////////////////////////////// CONSOLE OUTPUTS //////////////////////////////////////////////////////////
#define CONSOLE_BUF 132
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
//28|       NEAREST STAR: BETLEGEUSE       / ORION         
//29|   NEAREST STAR POS: (N)-90 59'59.0"  (W)-179 59'59.0"  23h59m59.000s   
//30|   IR CODE RECEIVED: DECODED STRING                                                                                       
//31|   IR CODE RECEIVED: 12345678 12345678 / 1234              GOTO STATE:
//32| PREV CODE RECEIVED: 12345678 12345678                          ACCEL:
//33| PREV PREV RECEIVED: 12345678 12345678                          SPEED:  
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
       NEAREST STAR: \012\015\
   NEAREST STAR POS: \012\015\
   IR CODE RECEIVED: \012\015\
   IR CODE RECEIVED: \012\015\
 PREV CODE RECEIVED: \012\015\
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
// PRINT FORMATS:
// LAT : (N)-90 59'59.00"
// LON : (W)-179 59'59.00"
// RA  : 23h59m59.000s
////////////////////////////////////////// DISPLAY SECTION //////////////////////////////////////////

void pc(char ccc);
void ps(char *ssz);
void pgm_ps(const char *ssz);
void ps08x(char *string,long value);

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

void p01s(char *buf,char val)
{
buf[0] = val;
buf[1] = 0;
}

char *psss(char *dst,char *src)
{
while ( *src ) *dst++ = *src++; 
*dst=0;
return dst;
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

void p16x(char *bof,long msb, long lsb)
{
p08x(&bof[0],msb);
p08x(&bof[9],lsb);
bof[8]= ' ';
}

void ps08x(char *string,long value)
{
char bof[9]; 
ps(string);
p08x(bof,value);
ps(bof);
}

void ps04x(char *string,short value)
{
char bof[5];
unsigned char *p_it = (unsigned char*) &value;
ps(string);
p02x(&bof[2],*p_it++);
p02x(&bof[0],*p_it++);
bof[4]=0;
ps(bof);
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
      if(console_go==0) display_data((char*)console_buf,0,20,pgm_display_bug,prob,FMT_HEX,8);  console_go = 1;
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

void display_next(void);
void display_next_bg(void) 
{
d_debug[12]++;
if ( AP0_DISPLAY == 0 ) display_next();  // if not printing from AP0, then print here
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


static unsigned char  d_task=0;
void display_next(void)
{
static unsigned char  first=1;
static unsigned char  cycle=0;
static unsigned char  console_started=0;
static unsigned char  console_special_started=0;
static unsigned short d_idx=0;
unsigned char Next=0;
unsigned char CCC;
long ltmp,tmp=0;
short stmp=0;

d_debug[4]=ra.speed;
d_debug[5]=ra.pos;
d_debug[6]=ra.pos_dem;
d_debug[13]=goto_cmd + 0x100*moving;
d_debug[7]=dec.state;
d_debug[15]=ra.state;

// process RX
if ( (UCSR0A & 0x80) != 0)    // ********** CA CA LIT BIEN...
   {
   CCC = UDR0;
   if ( rs232_rx_clr )
      {
      rs232_rx_clr = rs232_rx_idx = 0;
      rs232_rx_buf[rs232_rx_idx] = 0;
      }
   if ( CCC == 0x08 ) // backspace
      {
      if ( rs232_rx_idx > 0 ) rs232_rx_idx--;
      rs232_rx_buf[rs232_rx_idx] = 0;
      }
   else
      {
      rs232_rx_buf[rs232_rx_idx++] = CCC;
      if ( rs232_rx_idx >= RS232_RX_BUF ) rs232_rx_idx=RS232_RX_BUF-1;
      rs232_rx_buf[rs232_rx_idx] = 0;
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
   
         #define DISPLAY_DATA(XXX,YYY,XPGM_STR,FMT,SIZE,VALUE,L_VALUE)            \
         {                                                                       \
         if ( L_VALUE != VALUE || first)                                         \
            {                                                                    \
            display_data((char*)rs232_tx_buf,XXX,YYY,XPGM_STR,VALUE,FMT,SIZE);    \
            L_VALUE = VALUE;                                                     \
            }                                                                    \
         else d_task ++;                                                         \
         }
    
         d_task ++ ;
         if ( d_task >=  NB_DTASKS+1  && d_task <= NB_DTASKS+16 )    
            {
            if ( d_task == NB_DTASKS + 1  ) DISPLAY_DATA( 22,36,0,FMT_HEX,8,d_debug[0] ,l_debug[0]);
            if ( d_task == NB_DTASKS + 2  ) DISPLAY_DATA( 32,36,0,FMT_HEX,8,d_debug[1] ,l_debug[1]);
            if ( d_task == NB_DTASKS + 3  ) DISPLAY_DATA( 42,36,0,FMT_HEX,8,d_debug[2] ,l_debug[2]);
            if ( d_task == NB_DTASKS + 4  ) DISPLAY_DATA( 52,36,0,FMT_HEX,8,d_debug[3] ,l_debug[3]);
            if ( d_task == NB_DTASKS + 5  ) DISPLAY_DATA( 64,36,0,FMT_HEX,8,d_debug[4] ,l_debug[4]);
            if ( d_task == NB_DTASKS + 6  ) DISPLAY_DATA( 74,36,0,FMT_HEX,8,d_debug[5] ,l_debug[5]);
            if ( d_task == NB_DTASKS + 7  ) DISPLAY_DATA( 84,36,0,FMT_HEX,8,d_debug[6] ,l_debug[6]);
            if ( d_task == NB_DTASKS + 8  ) DISPLAY_DATA( 94,36,0,FMT_HEX,8,d_debug[7] ,l_debug[7]);
            if ( d_task == NB_DTASKS + 9  ) DISPLAY_DATA( 22,37,0,FMT_HEX,8,d_debug[8] ,l_debug[8]);
            if ( d_task == NB_DTASKS + 10 ) DISPLAY_DATA( 32,37,0,FMT_HEX,8,d_debug[9] ,l_debug[9]);
            if ( d_task == NB_DTASKS + 11 ) DISPLAY_DATA( 42,37,0,FMT_HEX,8,d_debug[10],l_debug[10]);
            if ( d_task == NB_DTASKS + 12 ) DISPLAY_DATA( 52,37,0,FMT_HEX,8,d_debug[11],l_debug[11]);
            if ( d_task == NB_DTASKS + 13 ) DISPLAY_DATA( 64,37,0,FMT_HEX,8,d_debug[12],l_debug[12]);
            if ( d_task == NB_DTASKS + 14 ) DISPLAY_DATA( 74,37,0,FMT_HEX,8,d_debug[13],l_debug[13]);
            if ( d_task == NB_DTASKS + 15 ) DISPLAY_DATA( 84,37,0,FMT_HEX,8,d_debug[14],l_debug[14]);
            if ( d_task == NB_DTASKS + 16 ) DISPLAY_DATA( 94,37,0,FMT_HEX,8,d_debug[15],l_debug[15]);
            }
         if ( d_task >=  NB_DTASKS+17  && d_task <= NB_DTASKS+32 )    
            {
            if ( d_task == NB_DTASKS + 17 ) DISPLAY_DATA( 22,35,0,FMT_HEX,4,histogram[ 0],l_histogram[ 0]);
            if ( d_task == NB_DTASKS + 18 ) DISPLAY_DATA( 27,35,0,FMT_HEX,4,histogram[ 1],l_histogram[ 1]);
            if ( d_task == NB_DTASKS + 19 ) DISPLAY_DATA( 32,35,0,FMT_HEX,4,histogram[ 2],l_histogram[ 2]);
            if ( d_task == NB_DTASKS + 20 ) DISPLAY_DATA( 37,35,0,FMT_HEX,4,histogram[ 3],l_histogram[ 3]);
            if ( d_task == NB_DTASKS + 21 ) DISPLAY_DATA( 42,35,0,FMT_HEX,4,histogram[ 4],l_histogram[ 4]);
            if ( d_task == NB_DTASKS + 22 ) DISPLAY_DATA( 47,35,0,FMT_HEX,4,histogram[ 5],l_histogram[ 5]);
            if ( d_task == NB_DTASKS + 23 ) DISPLAY_DATA( 52,35,0,FMT_HEX,4,histogram[ 6],l_histogram[ 6]);
            if ( d_task == NB_DTASKS + 24 ) DISPLAY_DATA( 57,35,0,FMT_HEX,4,histogram[ 7],l_histogram[ 7]);
            if ( d_task == NB_DTASKS + 25 ) DISPLAY_DATA( 64,35,0,FMT_HEX,4,histogram[ 8],l_histogram[ 8]);
            if ( d_task == NB_DTASKS + 26 ) DISPLAY_DATA( 69,35,0,FMT_HEX,4,histogram[ 9],l_histogram[ 9]);
            if ( d_task == NB_DTASKS + 27 ) DISPLAY_DATA( 74,35,0,FMT_HEX,4,histogram[10],l_histogram[10]);
            if ( d_task == NB_DTASKS + 28 ) DISPLAY_DATA( 79,35,0,FMT_HEX,4,histogram[11],l_histogram[11]);
            if ( d_task == NB_DTASKS + 29 ) DISPLAY_DATA( 84,35,0,FMT_HEX,4,histogram[12],l_histogram[12]);
            if ( d_task == NB_DTASKS + 30 ) DISPLAY_DATA( 89,35,0,FMT_HEX,4,histogram[13],l_histogram[13]);
            if ( d_task == NB_DTASKS + 31 ) DISPLAY_DATA( 94,35,0,FMT_HEX,4,histogram[14],l_histogram[14]);
            if ( d_task == NB_DTASKS + 32 ) DISPLAY_DATA( 99,35,0,FMT_HEX,4,histogram[15],l_histogram[15]);
            }
         if ( d_task >=  NB_DTASKS+33  && d_task <= NB_DTASKS+48 )    
            {
            if ( d_task == NB_DTASKS + 33 ) DISPLAY_DATA( 22,38,0,FMT_HEX,8,d_debug[16],l_debug[16]);
            if ( d_task == NB_DTASKS + 34 ) DISPLAY_DATA( 32,38,0,FMT_HEX,8,d_debug[17],l_debug[17]);
            if ( d_task == NB_DTASKS + 35 ) DISPLAY_DATA( 42,38,0,FMT_HEX,8,d_debug[18],l_debug[18]);
            if ( d_task == NB_DTASKS + 36 ) DISPLAY_DATA( 52,38,0,FMT_HEX,8,d_debug[19],l_debug[19]);
            if ( d_task == NB_DTASKS + 37 ) DISPLAY_DATA( 64,38,0,FMT_HEX,8,d_debug[20],l_debug[20]);
            if ( d_task == NB_DTASKS + 38 ) DISPLAY_DATA( 74,38,0,FMT_HEX,8,d_debug[21],l_debug[21]);
            if ( d_task == NB_DTASKS + 39 ) DISPLAY_DATA( 84,38,0,FMT_HEX,8,d_debug[22],l_debug[22]);
            if ( d_task == NB_DTASKS + 40 ) DISPLAY_DATA( 94,38,0,FMT_HEX,8,d_debug[23],l_debug[23]);
            if ( d_task == NB_DTASKS + 41 ) DISPLAY_DATA( 22,39,0,FMT_HEX,8,d_debug[24],l_debug[24]);
            if ( d_task == NB_DTASKS + 42 ) DISPLAY_DATA( 32,39,0,FMT_HEX,8,d_debug[25],l_debug[25]);
            if ( d_task == NB_DTASKS + 43 ) DISPLAY_DATA( 42,39,0,FMT_HEX,8,d_debug[26],l_debug[26]);
            if ( d_task == NB_DTASKS + 44 ) DISPLAY_DATA( 52,39,0,FMT_HEX,8,d_debug[27],l_debug[27]);
            if ( d_task == NB_DTASKS + 45 ) DISPLAY_DATA( 64,39,0,FMT_HEX,8,d_debug[28],l_debug[28]);
            if ( d_task == NB_DTASKS + 46 ) DISPLAY_DATA( 74,39,0,FMT_HEX,8,d_debug[29],l_debug[29]);
            if ( d_task == NB_DTASKS + 47 ) DISPLAY_DATA( 84,39,0,FMT_HEX,8,d_debug[30],l_debug[30]);
            if ( d_task == NB_DTASKS + 48 ) DISPLAY_DATA( 94,39,0,FMT_HEX,8,d_debug[31],l_debug[31]);
            }

         if ( d_task == NB_DTASKS + 65 ) DISPLAY_DATA( 35,22,0,FMT_HMS,8,seconds,l_seconds);

         if ( d_task == NB_DTASKS + 66 ) DISPLAY_DATA( 22,25,0,FMT_NS,0,dec.pos,l_dec_pos);           // SKY POSITION
         if ( d_task == NB_DTASKS + 67 ) DISPLAY_DATA( 39,25,0,FMT_EW,0,ra.pos,l_ra_pos1);            // SKY POSITION
         if ( d_task == NB_DTASKS + 68 ) DISPLAY_DATA( 57,25,0,FMT_RA,0,ra.pos,l_ra_pos2);            // SKY POSITION

         if ( d_task == NB_DTASKS + 69 ) DISPLAY_DATA( 22,26,0,FMT_NS,0,dec.pos_cor,l_dec_pos_cor);   // CORRECTED STAR POSITION
         if ( d_task == NB_DTASKS + 70 ) DISPLAY_DATA( 39,26,0,FMT_EW,0,ra.pos_cor,l_ra_pos_cor1);    // CORRECTED STAR POSITION
         if ( d_task == NB_DTASKS + 71 ) DISPLAY_DATA( 57,26,0,FMT_RA,0,ra.pos_cor,l_ra_pos_cor2);    // CORRECTED STAR POSITION

         if ( d_task == NB_DTASKS + 72 ) DISPLAY_DATA( 22,27,0,FMT_NS,0,dec.pos_hw,l_dec_pos_hw);     // TELESCOPE POSITION
         if ( d_task == NB_DTASKS + 73 ) DISPLAY_DATA( 39,27,0,FMT_EW,0,ra.pos_hw,l_ra_pos_hw1);      // TELESCOPE POSITION
         if ( d_task == NB_DTASKS + 74 ) DISPLAY_DATA( 57,27,0,FMT_RA,0,ra.pos_hw,l_ra_pos_hw2);      // TELESCOPE POSITION

         if ( d_task == NB_DTASKS + 75 ) 
            {
            tmp = stmp = (short)rs232_rx_buf;
            if ( rs232_rx_idx != l_rs232_rx_idx) tmp=0;   // force an update of the command line
            DISPLAY_DATA( 14,44,0,FMT_STR,0,stmp,tmp);
            l_rs232_rx_idx = rs232_rx_idx;
            }

         if ( d_task == NB_DTASKS + 76 ) DISPLAY_DATA( 22,28,pgm_stars_name + cur_star*STAR_NAME_LEN ,FMT_NO_VAL,0,cur_star,l_cur_star);

         ltmp = pgm_read_dword(&pgm_stars_pos[cur_star*2+1]);  tmp=0;
         if ( d_task == NB_DTASKS + 77 ) DISPLAY_DATA( 22,29,0,FMT_NS,0,ltmp,tmp);      // NEAREAST STAR POSITION
         ltmp = pgm_read_dword(&pgm_stars_pos[cur_star*2+0]);  tmp=0;
         if ( d_task == NB_DTASKS + 78 ) DISPLAY_DATA( 39,29,0,FMT_EW,0,ltmp,tmp);      // NEAREAST STAR POSITION
                                                              tmp=0;
         if ( d_task == NB_DTASKS + 79 ) DISPLAY_DATA( 57,29,0,FMT_RA,0,ltmp,tmp);      // NEAREAST STAR POSITION


         if ( d_task == NB_DTASKS + 80 ) { first = 0 ; d_task = NB_DTASKS; }
         else                            Next = rs232_tx_buf[rs232_tx_idx++];
         }
      }
   } 
if ( Next != 0 ) UDR0 = Next;
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
   histogram[histo]++;
   }

TIMSK0 = 1;     // timer0 interrupt enable
}

ISR(TIMER1_COMPB_vect)   // Clear the Step outputs
{
long temp;
// too long to complete !! set_digital_output(DO_RA_STEP,0);    // eventually, I should use my routines...flush polopu...
// too long to complete !! set_digital_output(DO_DEC_STEP,0);   // eventually, I should use my routines...flush polopu...
//FAST_SET_RA_STEP(0);
//FAST_SET_DEC_STEP(0);

temp = ra.pos_hw-ra.pos_cor;
if ( temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_RA_DIR,0); // go backward
   //FAST_SET_RA_DIR(0);
   ra.direction=0x00;
   ADD_VALUE_TO_POS(-TICKS_P_STEP,ra.pos_hw);
   ra.next=0x04;
   }
else if ( -temp >= TICKS_P_STEP )
   {
   // too long to complete !!set_digital_output(DO_RA_DIR,1); // go forward
   //FAST_SET_RA_DIR(1);
   ra.direction=0x08;
   ADD_VALUE_TO_POS(TICKS_P_STEP,ra.pos_hw);
   ra.next=0x04;
   }
else { ra.next=0; }

temp = dec.pos_hw-dec.pos_cor;
if ( temp >= (2*TICKS_P_STEP) )      // The DEC axis has 2 times less teeths 
   {
   // too long to complete !!set_digital_output(DO_DEC_DIR,0); // go backward
   //FAST_SET_DEC_DIR(0);
   dec.direction=0x00;
   ADD_VALUE_TO_POS(-(2*TICKS_P_STEP),dec.pos_hw);
   dec.next=0x01;
   }
else if ( -temp >= (2*TICKS_P_STEP) )
   {
   // too long to complete !!set_digital_output(DO_DEC_DIR,1); // go forward
   //FAST_SET_DEC_DIR(1);
   dec.direction=0x02;
   ADD_VALUE_TO_POS( (2*TICKS_P_STEP),dec.pos_hw);
   dec.next=0x01;
   }
else { dec.next=0; }

PORTC = ra.direction | dec.direction;    // I do this to optimize execution time

}

short process_goto(AXIS *axis,char ra_axis)
{
long temp_abs;

if      ( axis->state == 0 ) // idle
   {
   if ( goto_cmd ) axis->state++; // new goto command
   }
else if ( axis->state == 1 ) // setup the goto
   {
   axis->pos_initial  = axis->pos; 
   axis->pos_delta    = axis->pos_target - axis->pos_initial;
   axis->accel_seq    = 0;  // reset the sequence 
   if ( axis->pos_target - axis->pos_initial > 0 ) axis->accel =  10; // going forward
   else                                            axis->accel = -10; // going backward
   if ( axis->pos_delta > 0 ) axis->pos_delta =  axis->pos_delta;
   else                       axis->pos_delta = -axis->pos_delta;
   axis->state++;
   if ( axis->pos_delta == 0 ) axis->state=6; // No movement required
   }
else if ( axis->state == 2 )  // detect max speed, or halway point
   {
   axis->pos_part1 = axis->pos - axis->pos_initial;
   if ( axis->pos_part1 > 0 ) temp_abs =  2*axis->pos_part1;
   else                       temp_abs = -2*axis->pos_part1;

   if ( temp_abs >= axis->pos_delta ) // we reached the mid point
      {
      axis->state = 4; // decelerate
      axis->accel_seq = ACCEL_PERIOD - axis->accel_seq;  // reverse the last plateau
      }
   if ( axis->speed == MAX_SPEED || -axis->speed == MAX_SPEED ) // reached steady state
      {
      axis->state = 3; // cruise
      }
   else axis->last_high_speed = axis->speed;
   }
else if ( axis->state == 3 )  // cruise speed
   {
   temp_abs = (axis->pos_part1 + axis->pos - axis->pos_initial);
   if ( temp_abs < 0 ) temp_abs = -temp_abs;
   if ( temp_abs >= axis->pos_delta ) // we reached the end of the cruise period
      {
      axis->state = 4; // decelerate
      axis->accel_seq = 0;  // reset the sequence 
      }
   }
else if ( axis->state == 4 )  // set deceleration
   {
   if ( axis->pos_target - axis->pos_initial > 0 ) axis->accel = -10; // going forward but decelerate
   else                                            axis->accel =  10; // going backward but decelerate
   axis->state++;
   axis->speed = axis->last_high_speed;  // restore last high speed
   }
else if ( axis->state == 5 )  // deceleration
   {
   if ( axis->last_high_speed * axis->speed < 0 ) axis->state = 6;
//   if ( axis->accel > 0 && axis->speed > 0 ) axis->state = 6;
//   if ( axis->accel < 0 && axis->speed < 0 ) axis->state = 6;
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

g_goto_state = axis->state; // set the global goto state
g_ra_pos_part1 = axis->pos_part1;

/////// do the math...
axis->accel_seq++;
if ( axis->accel_seq > ACCEL_PERIOD )
   {
   axis->accel_seq = 0;
   axis->speed += axis->accel;
   if (  axis->speed > MAX_SPEED && axis->accel > 0 ) axis->accel =  0;
   if (  axis->speed > MAX_SPEED )                    axis->speed =  MAX_SPEED;
   if ( -axis->speed > MAX_SPEED && axis->accel < 0 ) axis->accel =  0;
   if ( -axis->speed > MAX_SPEED )                    axis->speed = -MAX_SPEED;
   }
ADD_VALUE_TO_POS(axis->speed,axis->pos);

axis->pos_dem = axis->pos;
if ( ra_axis ) ADD_VALUE_TO_POS(axis->pos_earth,axis->pos_dem);   // add the earth's rotation only on the RA axis

axis->pos_cor = axis->pos_dem;

//ra.pos_dem = ra.pos_earth + ra.pos;
//ra.pos_cor = ra.pos_dem;  // for now, no correction

return axis->state;
}


ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
unsigned long histo;
static short earth_comp=0;

// this takes a lot of time . . . .: ssec = (ssec+1)%10000;   
ssec++; if ( ssec == 10000 ) ssec = 0;
if ( ssec == 0) seconds++;

d_TIMER1++;             // counts time in 0.1 ms
if ( ! motor_disable )    //////////////////// motor disabled ///////////
   {
// These takes too long to complete !!!  set_digital_output(DO_RA_STEP ,ra.next);     // eventually, I should use my routines...flush polopu...
// These takes too long to complete !!!  set_digital_output(DO_DEC_STEP,dec.next);    // eventually, I should use my routines...flush polopu...
//   FAST_SET_RA_STEP(ra.next);    //   >
//   FAST_SET_DEC_STEP(dec.next);  //   > 1/16
//   if ( ra.next )  fast_portc |= (1<<DO_RA_STEP);
//   if ( dec.next ) fast_portc |= (1<<DO_DEC_STEP);
//   PORTC = ra.next | dec.next;
   PORTC = ra.next | dec.next | ra.direction | dec.direction;    // I do this to optimize execution time
 
   ///////////// Process earth compensation
   // this takes a lot of time . . . .:earth_comp = (earth_comp+1)%EARTH_COMP;
   earth_comp++ ; if ( earth_comp == EARTH_COMP ) earth_comp = 0;
   if ( earth_comp == 0 ) ADD_VALUE_TO_POS(TICKS_P_STEP,ra.pos_earth);    // Correct for the Earth's rotation
   
   ///////////// Process goto

   process_goto(&ra ,1); // 1 : specify that this is the RA axis     >> 2/16
   process_goto(&dec,0); // 0 : this is the DEC axis
//   if ( d_TIMER1 & 1 ) process_goto(&ra ,1); // 1 : specify that this is the RA axis     >> 2/16
//   else                process_goto(&dec,0); // 0 : this is the DEC axis
 
   d_debug[0 ]=ra.pos_initial;
   d_debug[8 ]=dec.pos_initial;
   
   d_debug[1 ]=ra.pos_target;
   d_debug[9 ]=dec.pos_target;
   
   d_debug[2 ]=ra.pos_delta;
   d_debug[10]=dec.pos_delta;
   
   d_debug[3 ]=ra.state;
   d_debug[11]=dec.state;
   
   if ( (ra.state == 0) && (dec.state == 0)) moving=0;
   else                                      moving=1;  // we are still moving 
   if ( moving ) goto_cmd=0;                            // command received
   }

// This section terminates the interrupt routine and will help identify how much CPU time we use in the real-time interrupt
if ( histo_sp0 == 1 ) // if we are not monitoring SP0
   {  
   histo = TCNT1;
   histo *= 15 * 10;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
   histo /= F_CPU_K;  // TCNT1 counts from 0 to F_CPU_K/10 (see OCR1A) 
   if ( histo > 15 ) histo=15;
   histogram[histo]++;
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
// - used by timer routines - d_debug[3]++;
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

TIMSK1 |= 1 << OCIE1B;    // timer1 interrupt when Output Compare B Match
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
init_rs232();
init_disp();


ps("\033[2J");
ps("telescope starting...");

set_digital_output(DO_DISABLE  ,motor_disable);   // Start disabled
set_digital_output(DO_RA_DIR   ,PULL_UP_ENABLED);
set_digital_output(DO_RA_STEP  ,PULL_UP_ENABLED);    
set_digital_output(DO_DEC_STEP ,PULL_UP_ENABLED);    
set_digital_output(DO_DEC_STEP ,PULL_UP_ENABLED);    
set_digital_output(DO_10KHZ    ,PULL_UP_ENABLED);    // Set 10 Khz pwm output of OC1B (from TIMER 1) used to control the steps
//DDRB = 0xFF; // All pins of port B as output

set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();
sei();         //enable global interrupts

wait(1,SEC);
while (console_go) display_next_bg(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_starting,d_ram,FMT_NO_VAL,8);  console_go = 1;
wait(1,SEC);
while (console_go) display_next_bg(); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_free_mem,d_ram,FMT_HEX,8);  console_go = 1;
wait(1,SEC);

#ifdef POLOLU
while (console_special); /* wait for ready */ console_special = pololu;
#else
while (console_special); /* wait for ready */ console_special = dip328p;
#endif


d_debug[14]=0x3333;
wait(2,SEC);

d_now   = d_TIMER1;
for(iii=0;iii<16;iii++) histogram[iii]=0;
seconds = ssec = 0;
motor_disable = 0;   // Stepper motor enabled...
set_digital_output(DO_DISABLE  ,motor_disable);   

d_debug[14]=0x4444;
wait(5,SEC);

d_debug[14]=0x5555;
//ra.pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 10UL; // thats 30 degrees , thats 10 turns, thats 357920000 , thats 0x15556D00     got:1555605F
ra.pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 3UL; 
dec.pos_target = 0;
goto_cmd = 1;  
d_debug[14]=0x6666;
while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);


d_debug[14]=0x7777;
// go the other way around
ra.pos_target = TICKS_P_STEP * 200UL * 5UL * 16UL * 2UL;
dec.pos_target = 0;
goto_cmd = 1;  
while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);

d_debug[14]=0x8888;
// go the other way around
ra.pos_target = TICKS_P_DAY -TICKS_P_STEP * 200UL * 5UL * 16UL * 3UL;
dec.pos_target = 0;
goto_cmd = 1;  
while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);


d_debug[14]=0x9999;
// go the other way around
ra.pos_target  = pgm_read_dword(&pgm_stars_pos[cur_star*2+0]);
dec.pos_target = pgm_read_dword(&pgm_stars_pos[cur_star*2+1]);
goto_cmd = 1;  
while ( moving || goto_cmd ) display_next_bg();  // wait for the reposition to be complete
wait(5,SEC);


while (1 )
   {
   d_debug[14]=0xAAAA;
   d_now   = d_TIMER1;
   wait(1,SEC);
   d_debug[14]=0xBBBB;
   d_now   = d_TIMER1;
   d_debug[3] = d_now;
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
