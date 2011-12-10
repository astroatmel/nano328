#define MAIN_TELESCOPE
//#define MAIN_VT_TEST

// This is a Little Endian CPU
// Notes
// in minicom, to see the degree character, call minicom: minicom -t VT100 
#include <pololu/orangutan.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>



#define DO_DISABLE       IO_D2
#define DO_RA_DIR        IO_D4
#define DO_RA_STEP       IO_D7
#define DO_DEC_STEP      IO_B4
#define DO_10KHZ         IO_B2

#define ADC6C            6
#define AI_COMMAND       ADC6C
 
////////////////////////////////// DEFINES /////////////////////////////////////   RA_ONE_STEP * 16 * GEAR_BIG/GEAR_SMALL * STEP_P_REV / RA_DEG_P_REV
#define F_CPU_K       20000
#define FRAME         10000
#define baud          9600
#define GEAR_BIG      120
#define GEAR_SMALL    24
#define STEP_P_REV    200
#define RA_DEG_P_REV  3
#define DEC_DEG_P_REV 6
#define POLOLU        1
#define ACCEL_PERIOD     100
// 450 = nb cycles per day / nb steps per full turn  = 24*60*60*10000Hz / [ (360/RA_DEG_P_REV) * 200 steps per motor turn * (GEAR_BIG/GEAR_SMALL) * 16 microstep
#define EARTH_COMP    450    
// Number of step per full circle on RA axis
// one turn on RA is 3 deg, so 360 deg is 120 turns
// the gear ration is 120 for 24, so the ration is 5 (5 turn of the stepper moter is one turn on the RA axis)
// one turn is 200 steps, each steps is devided in 16 by the stepper controler
// so, there are 120 * 5 * 200 * 16  steps on the RA axis : 1920000 : 0x1D4C00
// so , each step is 0x100000000 / 0x1D4C00 = 2236.962133 = 0x8BD
/// 2237 * 1920000 = 4 295 040 000
/// 2^32           = 4 294 967 296
///                         72 704   which is 0.99998 which means an error of 0.001 %
#define RA_ONE_STEP   2237
#define DEC_ONE_STEP  (RA_ONE_STEP*2)
#define RA_MAX_SPEED  (RA_ONE_STEP-RA_ONE_STEP/EARTH_COMP-2)
#define DEC_MAX_SPEED (DEC_ONE_STEP-2)

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
DEC_STEP   PB4  < 06   IO         AI/IO   19 >  PC4 (ADC4/SDA)\012\015\
           PB5  < 07   IO         AI/IO   18 >  PC3 (ADC3)\012\015\
DI RX232   PD0  < 08   IO         AI/IO   17 >  PC2 (ADC2)\012\015\
DO TX232   PD1  < 09   IO         AI/IO   16 >  PC1 (ADC1)\012\015\
DISABLE    PD2  < 10   IO         AI/IO   15 >  PC0 (ADC0)\012\015\
RA_DIR     PD4  < 11   IO         AI      14 >      (ADC6)\012\015\
RA_STEP    PD7  < 12   IO                 13 >  PC6 (RESET)\012\015\
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

short kkkk;
PROGMEM const char str_debug[][10]={ " debug0:", " debug1:"," debug2:"," debug3:"," debug4:"," debug5:"," debug6:"," debug7:"," debug8:"," debug9:"};
PROGMEM const char str_hist[][11]={" Hist0:", " Hist1:"," Hist2:"," Hist3:"," Hist4:"," Hist5:"," Hist6:"," Hist7:"," Hist8:"," Hist9:"," Hist10:"};
PROGMEM const char pgm_free_mem[]="Free Memory:";
PROGMEM const char pgm_starting[]="Telescope Starting...";
PROGMEM const char pgm_ascii[]="ascii:";
PROGMEM const char pgm_display_big[]="DISPLAY routines PROBLEM with value (FMT_NE/EW):";
PROGMEM const char pgm_display_big_ra[]="DISPLAY routines PROBLEM with value (FMT_RA):";
PROGMEM const char pgm_display_bug[]="Display routines problem with value (FMT_NE/EW):";
PROGMEM const char pgm_display_bug_ra[]="Display routines problem with value (FMT_RA):";
volatile unsigned long d_TIMER0;
volatile unsigned long d_TIMER1;
volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;
unsigned long d_now;
long seconds=0,l_seconds;
long d_debug[16], l_debug[16];
unsigned short histogram[16],l_histogram[16],histo_sp0=1; // place to store histogram of 10Khz interrupt routine

//   pos       = a long word : 0x00000000 = 0 deg  0x80000000 = 180 deg
//   pos_hw    = current hardware pos and a pulse is generated each time a change is required
//   pos_cor   = corrected pos = correction(pos_dem)  -> this compensate for the mis-polar-alignment of the telesctope
//   pos_dem   = demmanded pos = pos + earth correction
//   pos_earth = correction due to earth movement
long  ra_pos=0, ra_pos_hw=0, ra_pos_cor=0, ra_pos_dem=0, ra_pos_earth=0, ra_pos_star=0, ra_accel=0, ra_speed=0;   // Right Assension parameters
long dec_pos=0,dec_pos_hw=0,dec_pos_cor=0,dec_pos_dem=0,dec_pos_earth=0,dec_pos_star=0,dec_accel=0,dec_speed=0;   // Declinasion parameters        
long l_ra_pos,l_ra_pos_hw,l_ra_pos_hw1,l_ra_pos_hw2;
long  ra_unit_step;  // One step on the step motor is equal to what in 32 bit angle
long dec_unit_step;  // One step on the step motor is equal to what in 32 bit angle
long g_goto_state;
long g_ra_pos_part1;
char  ra_next=0, dec_next=0;  // Next state for the step command, we do this to allow DIR to settle

/////////////////////////////////////////// RS232 INPUTS ///////////////////////////////////////////////////////////
#define RS232_RX_BUF 32
unsigned char rs232_rx_buf[RS232_RX_BUF] = {0};
unsigned char rs232_rx_idx=0;               // <- always points on the NULL
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
//23|   CURRENT LOCATION: (N)-90 59'59.00" / (W)-179 59'59.00"                                                                                 
//24|        POLAR ERROR: (N)-90 59'59.00" / (W)-179 59'59.00"                                                                
//25|      STAR POSITION: (N)-90 59'59.00" / (W)-179 59'59.00"  23h59m59.000s    0x12345678 / 0x12345678                                         
//26| CORRECTED STAR POS: (N)-90 59'59.00" / (W)-179 59'59.00"  23h59m59.000s    0x12345678 / 0x12345678                                       
//27|      TELESCOPE POS: (N)-90 59'59.00" / (W)-179 59'59.00"  23h59m59.000s    0x12345678 / 0x12345678                                       
//28|       NEAREST STAR: BETLEGEUSE       / ORION                            
//29|   IR CODE RECEIVED: DECODED STRING                                                                                       
//30|   IR CODE RECEIVED: 12345678 12345678 / 1234              GOTO STATE:
//31| PREV CODE RECEIVED: 12345678 12345678                          ACCEL:
//32| PREV PREV RECEIVED: 12345678 12345678                          SPEED:  
//33|    BATTERY VOLTAGE: 12.0V
//34|          HISTOGRAM: 1234 1234 1234 1234 1234 1234 1234 1234  1234 1234 1234 1234 1234 1234 1234 1234            <-- once one reaches 65535, values are latched and printed 
//35|              DEBUG: 12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//36|                     12345678  12345678  12345678  12345678   12345678  12345678  12345678  12345678                                               
//37|                LCD: |1234567812345678|    MODE:                                                                                          
//38|                     |1234567812345678|                                                                                                               
//39|_______________________________________________________________________________________________________
//40|    IR-PLUS> GO  23h59m59.000s  90 59'59.00"  (assembled command from IR)
//41| RS232-PLUS> GO  23h59m59.000s  90 59'59.00"  (assembled command from RS232)

PROGMEM const char display_clear_screen[]     = "\033c\033c\033[2J\033[7h"; // reset reset cls line wrap 
PROGMEM const char display_define_scrolling[] = "\033[r\033[0;20r";         // enable scrolling + define scrolling
PROGMEM const char display_sa[]               = "\033[20;0H";                           // this is the last scrooling line
PROGMEM const char display_ns_line[]          = "\033[21;0H_______________________________________________________________________________________________________";

PROGMEM const char display_main[]={"\012\015\
          DATE/TIME: \012\015\
   CURRENT LOCATION: \012\015\
        POLAR ERROR: \012\015\
      STAR POSITION: \012\015\
 CORRECTED STAR POS: \012\015\
      TELESCOPE POS: \012\015\
       NEAREST STAR: \012\015\
   IR CODE RECEIVED: \012\015\
   IR CODE RECEIVED: \012\015\
 PREV CODE RECEIVED: \012\015\
 PREV PREV RECEIVED: \012\015\
    BATTERY VOLTAGE: \012\015\
      SP0 HISTOGRAM: \012\015\
              DEBUG: \012\015\
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
char tc = pgm_read_byte(ssz++);;
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
void display_data(char *str,short xxx,short yyy,const char *pgm_str,long value,char fmt,char size)
{
short iii=0;
long  jjj=0;
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
   str[iii++] = '(';
   if ( value >= 0 ) 
      {
      if ( fmt == FMT_NS ) str[iii++] = 'N';
      else                 str[iii++] = 'E';
      str[iii++] = ')'; 
      str[iii++] = ' '; 
      }
   else
      {
      if ( fmt == FMT_NS ) str[iii++] = 'S';
      else                 str[iii++] = 'W';
      str[iii++] = ')'; 
      str[iii++] = '-'; 
      } 
   if ( fmt == FMT_NS || fmt == FMT_EW ) 
      {
      short vvv=0,prob=0;
      if ( value < 0 ) value = -value;  // abs value

//    jjj    =  (RA_ONE_STEP * 16UL * (GEAR_BIG/GEAR_SMALL) * STEP_P_REV ) / RA_DEG_P_REV;    // jjj = nb of nano steps per deg    = 35792000/3   |||| target: 35791394.13/3 = 11930464.71
      jjj    =  1193046471UL;    // jjj = nb of nano steps per deg    = 35792000/3   |||| target: 35791394.13/3 = 11930464.71   so 119304647UL gives 100 degrees
      vvv    = value / jjj ;                                                 // jjj = nb of deg that fits in what remains of value
      value -= vvv * jjj;  
      if ( vvv>9 ) {vvv=9;prob|=0x800;}
      if ( fmt == FMT_EW )str[iii++] = vvv + '0';     // 100 deg

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x400;}
      str[iii++] = vvv + '0';                         // 10 deg

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x200;}
      str[iii++] = vvv + '0';                         // 1 deg

      str[iii++] = 0xC2;   // degree character
      str[iii++] = 0xB0;   // degree character

      jjj   /= 6;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>5 ) {vvv=5;prob|=0x100;}
      str[iii++] = vvv + '0';                         // 10 min

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x080;}
      str[iii++] = vvv + '0';                         // 1 min

      str[iii++] = '\'';  
      
      jjj   /= 6;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>5 ) {vvv=5;prob|=0x040;}
      str[iii++] = vvv + '0';                         // 10 sec

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x020;}
      str[iii++] = vvv + '0';                         // 1 sec

      str[iii++] = '.';  

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x010;}
      str[iii++] = vvv + '0';                         // 10th sec

      jjj   /= 10;
      vvv    = value / jjj ;
      value -= vvv * jjj;
      if ( vvv>9 ) {vvv=9;prob|=0x008;}
      str[iii++] = vvv + '0';                         // 100th sec
      str[iii++] = '"';  
      str[iii]   = 0;  

      if ( prob ) 
         {
         if(console_go==0) display_data((char*)console_buf,0,20,pgm_display_bug,prob,FMT_HEX,8);  console_go = 1;
         }
      }
   
   }
else if ( fmt == FMT_RA )  // Right Assention    23h59m59.000s
   {
   short vvv=0,prob=0;
   if ( value < 0 ) value = -value;  // abs value

// jjj    =  (RA_ONE_STEP * 16UL * (GEAR_BIG/GEAR_SMALL) * STEP_P_REV * 5);    // jjj = nb of nano steps per deg    = 35792000*5   |||| target: 35791394.13*5 = 178956970.7
   jjj    =  1789569707UL ;    // jjj = nb of nano steps per deg    =  35792000*5   |||| target: 35791394.13*5 = 178956970.7   so 1789569707 is in 10h
   vvv    = value / jjj ;                                                 // jjj = nb of deg that fits in what remains of value
   if ( vvv>2 ) {vvv=2;prob|=0x800;}
   str[iii++] = vvv + '0';                         // 10 hour

   jjj   /= 10;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>9 ) {vvv=9;prob|=0x400;}
   str[iii++] = vvv + '0';                         // 1 hour

   str[iii++] = 'h';   // degree character

   jjj   /= 6;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>5 ) {vvv=5;prob|=0x200;}
   str[iii++] = vvv + '0';                         // 10 min

   jjj   /= 10;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>9 ) {vvv=9;prob|=0x100;}
   str[iii++] = vvv + '0';                         // 1 min

   str[iii++] = 'm';  

   jjj   /= 6;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>5 ) {vvv=5;prob|=0x080;}
   str[iii++] = vvv + '0';                         // 10 sec

   jjj   /= 10;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>9 ) {vvv=9;prob|=0x040;}
   str[iii++] = vvv + '0';                         // 1 sec

   str[iii++] = '.';  

   jjj   /= 10;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>9 ) {vvv=9;prob|=0x020;}
   str[iii++] = vvv + '0';                         // 10th sec

   jjj   /= 10;
   vvv    = value / jjj ;
   value -= vvv * jjj;
   if ( vvv>9 ) {vvv=9;prob|=0x010;}
   str[iii++] = vvv + '0';                         // 100th sec sec

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
else if ( fmt == FMT_ASC )  // ASCII
   {
   str[iii++] = '[';  
   str[iii++] = value;  
   str[iii++] = ']';  
   str[iii]   = 0;  
   }
}

#define SEC 10000
#define MSEC 10
void wait(long time,long mult)
{
long d_now = d_TIMER1;
while ( d_TIMER1-d_now < time*mult ); 
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
long tmp=0;
short stmp=0;

// process RX
if ( (UCSR0A & 0x80) != 0)    // ********** CA CA LIT BIEN...
   {
   CCC = UDR0;
   d_debug[13]--;
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
            if ( d_task == NB_DTASKS + 1  ) DISPLAY_DATA( 22,35,0,FMT_HEX,8,d_debug[0] ,l_debug[0]);
            if ( d_task == NB_DTASKS + 2  ) DISPLAY_DATA( 32,35,0,FMT_HEX,8,d_debug[1] ,l_debug[1]);
            if ( d_task == NB_DTASKS + 3  ) DISPLAY_DATA( 42,35,0,FMT_HEX,8,d_debug[2] ,l_debug[2]);
            if ( d_task == NB_DTASKS + 4  ) DISPLAY_DATA( 52,35,0,FMT_HEX,8,d_debug[3] ,l_debug[3]);
            if ( d_task == NB_DTASKS + 5  ) DISPLAY_DATA( 64,35,0,FMT_HEX,8,d_debug[4] ,l_debug[4]);
            if ( d_task == NB_DTASKS + 6  ) DISPLAY_DATA( 74,35,0,FMT_HEX,8,d_debug[5] ,l_debug[5]);
            if ( d_task == NB_DTASKS + 7  ) DISPLAY_DATA( 84,35,0,FMT_HEX,8,d_debug[6] ,l_debug[6]);
            if ( d_task == NB_DTASKS + 8  ) DISPLAY_DATA( 94,35,0,FMT_HEX,8,d_debug[7] ,l_debug[7]);
            if ( d_task == NB_DTASKS + 9  ) DISPLAY_DATA( 22,36,0,FMT_HEX,8,d_debug[8] ,l_debug[8]);
            if ( d_task == NB_DTASKS + 10 ) DISPLAY_DATA( 32,36,0,FMT_HEX,8,d_debug[9] ,l_debug[9]);
            if ( d_task == NB_DTASKS + 11 ) DISPLAY_DATA( 42,36,0,FMT_HEX,8,d_debug[10],l_debug[10]);
            if ( d_task == NB_DTASKS + 12 ) DISPLAY_DATA( 52,36,0,FMT_HEX,8,d_debug[11],l_debug[11]);
            if ( d_task == NB_DTASKS + 13 ) DISPLAY_DATA( 64,36,0,FMT_HEX,8,d_debug[12],l_debug[12]);
            if ( d_task == NB_DTASKS + 14 ) DISPLAY_DATA( 74,36,0,FMT_HEX,8,d_debug[13],l_debug[13]);
            if ( d_task == NB_DTASKS + 15 ) DISPLAY_DATA( 84,36,0,FMT_HEX,8,d_debug[14],l_debug[14]);
            if ( d_task == NB_DTASKS + 16 ) DISPLAY_DATA( 94,36,0,FMT_HEX,8,d_debug[15],l_debug[15]);
            }
         if ( d_task >=  NB_DTASKS+17  && d_task <= NB_DTASKS+32 )    
            {
            if ( d_task == NB_DTASKS + 17 ) DISPLAY_DATA( 22,34,0,FMT_HEX,4,histogram[ 0],l_histogram[ 0]);
            if ( d_task == NB_DTASKS + 18 ) DISPLAY_DATA( 27,34,0,FMT_HEX,4,histogram[ 1],l_histogram[ 1]);
            if ( d_task == NB_DTASKS + 19 ) DISPLAY_DATA( 32,34,0,FMT_HEX,4,histogram[ 2],l_histogram[ 2]);
            if ( d_task == NB_DTASKS + 20 ) DISPLAY_DATA( 37,34,0,FMT_HEX,4,histogram[ 3],l_histogram[ 3]);
            if ( d_task == NB_DTASKS + 21 ) DISPLAY_DATA( 42,34,0,FMT_HEX,4,histogram[ 4],l_histogram[ 4]);
            if ( d_task == NB_DTASKS + 22 ) DISPLAY_DATA( 47,34,0,FMT_HEX,4,histogram[ 5],l_histogram[ 5]);
            if ( d_task == NB_DTASKS + 23 ) DISPLAY_DATA( 52,34,0,FMT_HEX,4,histogram[ 6],l_histogram[ 6]);
            if ( d_task == NB_DTASKS + 24 ) DISPLAY_DATA( 57,34,0,FMT_HEX,4,histogram[ 7],l_histogram[ 7]);
            if ( d_task == NB_DTASKS + 25 ) DISPLAY_DATA( 64,34,0,FMT_HEX,4,histogram[ 8],l_histogram[ 8]);
            if ( d_task == NB_DTASKS + 26 ) DISPLAY_DATA( 69,34,0,FMT_HEX,4,histogram[ 9],l_histogram[ 9]);
            if ( d_task == NB_DTASKS + 27 ) DISPLAY_DATA( 74,34,0,FMT_HEX,4,histogram[10],l_histogram[10]);
            if ( d_task == NB_DTASKS + 28 ) DISPLAY_DATA( 79,34,0,FMT_HEX,4,histogram[11],l_histogram[11]);
            if ( d_task == NB_DTASKS + 29 ) DISPLAY_DATA( 84,34,0,FMT_HEX,4,histogram[12],l_histogram[12]);
            if ( d_task == NB_DTASKS + 30 ) DISPLAY_DATA( 89,34,0,FMT_HEX,4,histogram[13],l_histogram[13]);
            if ( d_task == NB_DTASKS + 31 ) DISPLAY_DATA( 94,34,0,FMT_HEX,4,histogram[14],l_histogram[14]);
            if ( d_task == NB_DTASKS + 32 ) DISPLAY_DATA( 99,34,0,FMT_HEX,4,histogram[15],l_histogram[15]);
            }

         if ( d_task == NB_DTASKS + 33 ) DISPLAY_DATA( 35,22,0,FMT_HMS,8,seconds,l_seconds);
         if ( d_task == NB_DTASKS + 34 ) DISPLAY_DATA( 22,25,0,FMT_NS,0,ra_pos,l_ra_pos);
         if ( d_task == NB_DTASKS + 35 ) DISPLAY_DATA( 22,27,0,FMT_NS,0,ra_pos_hw,l_ra_pos_hw);
         if ( d_task == NB_DTASKS + 36 ) DISPLAY_DATA( 41,27,0,FMT_EW,0,ra_pos_hw,l_ra_pos_hw1);
         if ( d_task == NB_DTASKS + 37 ) DISPLAY_DATA( 45,25,0,FMT_RA,0,ra_pos_hw,l_ra_pos_hw2);
d_debug[10] = ra_pos;
d_debug[11] = ra_pos_hw;

         stmp = (short)rs232_rx_buf;
         if ( d_task == NB_DTASKS + 38 ) DISPLAY_DATA( 14,41,0,FMT_STR,0,stmp,tmp);

         if ( d_task == NB_DTASKS + 39 ) { first = 0 ; d_task = NB_DTASKS; }
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

char motor_disable=1;       // Stepper motor Disable
char goto_cmd=0;    // 1 = goto goto_ra_pos
long goto_ra_pos=0;


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
   if ( d_debug[8] < histo ) d_debug[8] = histo;  // temp read the highest value
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
set_digital_output(DO_RA_STEP,0);    // eventually, I should use my routines...flush polopu...
set_digital_output(DO_DEC_STEP,0);   // eventually, I should use my routines...flush polopu...
set_digital_output(DO_DISABLE  ,motor_disable);   

temp = ra_pos_hw-ra_pos_cor;
if ( temp >= RA_ONE_STEP )
   {
   set_digital_output(DO_RA_DIR,0); // go backward
   ra_pos_hw -= RA_ONE_STEP;
   ra_next=1;
   }
else if ( -temp >= RA_ONE_STEP )
   {
   set_digital_output(DO_RA_DIR,1); // go forward
   ra_pos_hw += RA_ONE_STEP;
   ra_next=1;
   }
else
   {
   ra_next=0;
   }
}

typedef struct   // those values are required per axis to be able to execute goto commands
   {             // I use a structure so that both DEC and RA axis can use the exact same routines
   short max_speed;        // units: nanostep per iteration       // A step is a pure step from the motor (only one coil is 100% ON at a time)
   short one_step;         // units: nanostep                     // A microstep is 1/16th of a step and is managed by the stepper motor controller
   char  deg_per_rev;      // units: deg/knob rev ; when the axis's know is turned one full turn, how much does the telescope move 
   char  accel_period;     // no units (it how many iteration between speed re-calculation)
   char  accel;            // units: nanostep per iteration per iteration
   char  accel_seq;        // no units (it's the sequence counter)
   long  pos;              // units: nanostep
   long  pos_initial;      // units: nanostep
   long  pos_target;       // units: nanostep
   long  pos_delta;        // units: nanostep (it's the required displacement: pos_target-pos_initial)
   long  pos_part1;        // units: nanostep (it's ths total amount of displacement during acceleration)
   long  speed;            // units: nanostep per iteration
   long  last_high_speed;  // units: nanostep per iteration       // I define:       [of course the "nanostep" below is not really nano (10^-9)   ]
   char  goto_cmd;         // no units (it's to tell the function to start a goto command)
   char  state;            // no units                            // A nanostep is the full telescope 360/2^32 ... on the RA axis, 1 microstep ~= 2236.962 nanosteps
   } AXIS;
AXIS ra ={RA_MAX_SPEED ,RA_ONE_STEP ,RA_DEG_P_REV ,ACCEL_PERIOD};
AXIS dec={DEC_MAX_SPEED,DEC_ONE_STEP,DEC_DEG_P_REV,ACCEL_PERIOD};

short process_goto(AXIS *axis, long goto_pos, char goto_cmd)
{
long temp_abs;
//axis->max_speed    = RA_MAX_SPEED;
//axis->one_step     = RA_ONE_STEP;
//axis->accel_period = ACCEL_PERIOD;

if      ( axis->state == 0 ) // idle
   {
   if ( goto_cmd ) axis->state++; // new goto command
   }
else if ( axis->state == 1 ) // setup the goto
   {
   axis->pos_initial  = axis->pos; 
   axis->pos_target   = goto_pos;
   axis->pos_delta    = axis->pos_target - axis->pos_initial;
   axis->accel_period = 0;  // reset the sequence 
   if ( axis->pos_target - axis->pos_initial > 0 ) axis->accel =  10; // going forward
   else                                            axis->accel = -10; // going backward
   if ( axis->pos_delta > 0 ) axis->pos_delta =  axis->pos_delta;
   else                       axis->pos_delta = -axis->pos_delta;
   axis->state++;
   }
else if ( axis->state == 2 )  // detect max speed, or halway point
   {
   axis->pos_part1 = axis->pos - axis->pos_initial;
   if ( axis->pos_part1 > 0 ) temp_abs =  2*axis->pos_part1;
   else                       temp_abs = -2*axis->pos_part1;
d_debug[1] = axis->pos_part1;

   if ( temp_abs >= axis->pos_delta ) // we reached the mid point
      {
      axis->state = 4; // decelerate
      axis->accel_period = axis->accel_period - axis->accel_seq;  // reverse the last plateau
      }
   if ( axis->speed == axis->max_speed || -axis->speed == axis->max_speed ) // reached steady state
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
      axis->accel_period = 0;  // reset the sequence 
      }
d_debug[2] = axis->pos - d_debug[1];
d_debug[4] = axis->pos_delta;
d_debug[8] = temp_abs;
   }
else if ( axis->state == 4 )  // set deceleration
   {
   if ( axis->pos_target - axis->pos_initial > 0 ) axis->accel = -10; // going forward but decelerate
   else                                            axis->accel =  10; // going backward but decelerate
   axis->state++;
   axis->speed = axis->last_high_speed;  // restore last high speed
d_debug[7] = axis->speed;
   }
else if ( axis->state == 5 )  // deceleration
   {
   if ( axis->last_high_speed * axis->speed < 0 ) axis->state = 6;
//   if ( axis->accel > 0 && axis->speed > 0 ) axis->state = 6;
//   if ( axis->accel < 0 && axis->speed < 0 ) axis->state = 6;
d_debug[3]=axis->pos - d_debug[2] - d_debug[1];
d_debug[9]=axis->speed;
   }
else if ( axis->state == 6 )  // done, since the math is far from perfect, we often are not at the exact spot
   {
   long tmp = axis->pos - axis->pos_target;
   long abs = tmp;

   axis->accel = axis->speed = 0;
   if ( abs < 0 )              abs = -abs;
   if ( tmp > 0 )              axis->speed = -100; // we went a bit too far, slowly go back
   else                        axis->speed =  100; // we stopped short, sloly go forward
   if ( abs < axis->one_step ) axis->state = 7;    // we are very close, we are done
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
if ( axis->accel_seq > axis->accel_period )
   {
   axis->accel_seq = 0;
   axis->speed += axis->accel;
   if (  axis->speed > axis->max_speed && axis->accel > 0 ) axis->accel = 0;
   if (  axis->speed > axis->max_speed )                    axis->speed = axis->max_speed;
   if ( -axis->speed > axis->max_speed && axis->accel < 0 ) axis->accel = 0;
   if ( -axis->speed > axis->max_speed )                    axis->speed = -axis->max_speed;
   }
axis->pos += axis->speed;

return axis->state;
}


ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
static short ssec=0;
unsigned long histo;
//static char  goto_state=0;    // 1 = goto goto_ra_pos
static short earth_comp=0;
static short earth_comp2=0;
//static short accel_period=0;
//static long  ra_pos_initial,ra_pos_target,ra_pos_delta,ra_pos_delta_abs,ra_pos_part1,ra_last_high_speed,temp_abs;

if ( ssec == 10000) {ssec = 0; seconds++;}
ssec++;

d_TIMER1++;             // counts time in 0.1 ms
if ( ! motor_disable )    //////////////////// motor disabled ///////////
   {
   set_digital_output(DO_RA_STEP,ra_next);    // eventually, I should use my routines...flush polopu...
   
   ///////////// Process earth compensation
   earth_comp++;
   if ( earth_comp > EARTH_COMP )
      {
      earth_comp = 0; 
      earth_comp2++;
      ra_pos_earth += RA_ONE_STEP;    // Correct for the Earth's rotation
      }
   
   ///////////// Process goto
   
   process_goto(&ra,goto_ra_pos,goto_cmd);
   
   ra_pos = ra.pos;
   ra_accel = ra.accel;
   ra_speed = ra.speed;
   if ( ra.state !=0 ) goto_cmd=0;
   
   ra_pos_dem = ra_pos_earth + ra.pos;
   ra_pos_cor = ra_pos_dem;  // for now, no correction
   
   // This below does work and proves that the Step outputs do work
   // if ( ! (d_TIMER1&3) ) set_digital_output(DO_RA_STEP,1);    // use my routines...flush polopu...
   // if ( ! (d_TIMER1&1) ) set_digital_output(DO_DEC_STEP,1);   // use my routines...flush polopu...
   }
// This section terminates the interrupt routine and will help identify how much CPU time we use in the real-time interrupt
if ( histo_sp0 == 1 ) // if we are not monitoring SP0
   {  
   histo = TCNT1;
   if ( d_debug[8] < histo ) d_debug[8] = histo;  // temp read the highest value
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
   OCR1B   = F_CPU_K/40;     // By default, set the PWM to 25%

   TIMSK0 |= 1 << TOIE0;     // timer0 interrupt when overvlow                    ///////////////// APOCO
   TCCR0A  = 0x03;           // FAST PWM, OC0A/OC0B pins, not driven
   TCCR0B  = 0x0A;           // WGM=111 Clock divider set to 8 , this creates a PMW wave at 40Khz
   OCR0A   = F_CPU_K/(10*8); // Clock divider set to 1, so F_CPU_K/10/8 is CLK / 1000 / 10 thus, every 10000 there will be an interrupt : 10Khz
   OCR0B   = F_CPU_K/(40*8); // By default, set the PWM to 25%
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


#ifdef MAIN_VT_TEST
int main_vt_test(void)     // To test VT100 codes
{
short iii,xxx=0,yyy=111;

// long temp = F_CPU/(16UL*baud)-1;  //set uart baud rate register
// UBRR0H = (temp >> 8);
// UBRR0L = (temp & 0xFF);
// UCSR0B = (1 <<RXEN0 | 1 << TXEN0 ); // enable RX and TX
// UCSR0C = (3<<UCSZ00);  // 8 bits, no parity, 1 stop

init_rs232();

ps("\033[2J");
ps("starting...\n");
for ( iii=0 ; iii<200 ; iii++ )
   {
   if(iii>10) xxx= 0x0F00;
   if(iii>20) xxx=-0x0F00;
   ps("\033[M");  //scroll up
   ps08x("\012\015 Filtered value=",yyy);
   ps08x("  input=",xxx);
   ps08x("  iii=",iii);
   }
while(1) ps("A5A5\012\015");
return 0;
}
#endif


#ifdef MAIN_TELESCOPE
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
set_digital_output(DO_10KHZ    ,PULL_UP_ENABLED);    // Set 10 Khz pwm output of OC1B (from TIMER 1) used to control the steps
set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();
sei();         //enable global interrupts

d_now   = d_TIMER1;
while ( d_TIMER1-d_now < 10000 )   // Async section
   {
   d_debug[0] = d_TIMER1;
   d_debug[1] = d_now;
   }
d_now   = d_TIMER1;
d_debug[8]=0;
while ( d_TIMER1-d_now < 100000 )   // Async section
   {
   d_debug[0] = d_TIMER1;
   d_debug[1] = d_now;
   }

while (console_go); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_starting,d_ram,FMT_NO_VAL,8);  console_go = 1;
wait(1,SEC);
while (console_go); /* wait for ready */ display_data((char*)console_buf,0,20,pgm_free_mem,d_ram,FMT_HEX,8);  console_go = 1;

d_now   = d_TIMER1;
while ( d_TIMER1-d_now < 10000 )   // Async section
   {
   d_debug[0] = d_TIMER1;
   d_debug[1] = d_now;
   }
d_now   = d_TIMER1;

#ifdef POLOLU
while (console_special); /* wait for ready */ console_special = pololu;
#else
while (console_special); /* wait for ready */ console_special = dip328p;
#endif

while ( d_TIMER1-d_now < 10000 )   // Async section
   {
   d_debug[0] = d_TIMER1;
   d_debug[1] = d_now;
   }
d_now   = d_TIMER1;
for(iii=0;iii<16;iii++) histogram[iii]=0;
seconds = 0;
motor_disable = 0;   // Stepper motor enabled...

while ( d_TIMER1-d_now < 10000 )   // Async section
   {
   d_debug[0] = d_TIMER1;
   d_debug[1] = d_now;
   }

while (1 )
   {
   d_now   = d_TIMER1;
   d_debug[2] = d_now;
   while ( d_TIMER1-d_now < 10000 );   // Async section
   d_now   = d_TIMER1;
   d_debug[3] = d_now;
   while ( d_TIMER1-d_now < 10000 );   // Async section
   }

d_state = 0;       // simple state machine to test some logic
   
while(1);

 
while(1)
   {
   // RS232 commands received
   // RS232 output
   set_digital_output(DO_DISABLE  ,motor_disable);   

   p_str_hex(" goto state:",g_goto_state,0);
   p_str_hex(" ra_pos_cor:",ra_pos_cor,0);
   p_str_hex(" ra_pos:",ra_pos,0);
   p_str_hex(" accel:",ra_accel,0);
   p_str_hex(" speed:",ra_speed,0);
   p_str_hex(" part1:",g_ra_pos_part1,1);

   if ( d_state == 0 )  // Wait 5 sec before start
      {
      if (d_TIMER1-d_now>10000) // Wait 1 sec before start
         {
         motor_disable = 0;   // Stepper motor enabled...
         d_state ++;
         }
      }
   else if ( d_state == 1 )  // goto command 
      {
      goto_ra_pos = RA_ONE_STEP * 200UL * 5UL * 16UL * 10UL; // thats 30 degrees , thats 10 turns, thats 357920000 , thats 0x15556D00     got:1555605F
      goto_cmd = 1;  // 1553FC53
      d_state ++;
      }
   else if ( d_state == 2 )  // wait for command to complete
      {
      d_now   = d_TIMER1;
      if ( g_goto_state == 0 ) d_state ++;
      }
   else if ( d_state == 3 )  // wait for command to complete
      {
      if (d_TIMER1-d_now>10000) d_state ++; // Wait 5 sec before end
      }
   else if ( d_state == 4 )  // goto command 
      {
      goto_ra_pos = RA_ONE_STEP * 200UL * 5UL * 16UL * 5UL; // thats 15 degrees, thats 0AAAB680    got: 0AAAAB2E   (and means that we need to go backward)
      goto_cmd = 1;  // 1553FC53
      d_state ++;
      }
   else if ( d_state == 5 )  // wait for command to complete
      {
      d_now   = d_TIMER1;
      if ( g_goto_state == 0 ) d_state ++;
      }
   else if ( d_state == 6 )  // wait for command to complete
      {
      if (d_TIMER1-d_now>10000) // Wait 5 sec before end
         {
         motor_disable = 1;   // Stepper motor disable...
         d_state ++;
         }
      }
   else if ( d_state == 7 )
      {
      return 0; //end of program
      }
   };

return 0;
}
#endif

int main(void)
{
#ifdef MAIN_TELESCOPE
main_telescope();  // Logic Detector
#endif
#ifdef MAIN_VT_TEST
main_vt_test();    // To test VT100 codes
#endif
return 0;
}

/*
$author:$
$version:$
$log:$

*/
