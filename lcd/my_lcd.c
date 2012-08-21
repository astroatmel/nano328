/*
$Author: pmichel $
$Date: 2012/07/16 05:26:47 $
$Id: my_lcd.c,v 1.4 2012/07/16 05:26:47 pmichel Exp pmichel $
$Locker: pmichel $
$Revision: 1.4 $
$Source: /home/pmichel/project/my_lcd/RCS/my_lcd.c,v $
*/

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>


unsigned long d_ram;
unsigned long d_state;

volatile unsigned long d_TIMER0;
volatile unsigned long d_TIMER1;
volatile unsigned long d_USART_RX;
volatile unsigned long d_USART_TX;
unsigned long d_now;

volatile unsigned short msec=0;
volatile unsigned char sec=0,min=0,hour=0;


 
////////////////////////////////// DEFINES /////////////////////////////////////   TICKS_P_STEP * 16 * GEAR_BIG/GEAR_SMALL * STEP_P_REV / RA_DEG_P_REV
#define F_CPU_K          20000
#define baud             9600

#define SEC 1000
#define MSEC 1


void wait_us(unsigned short time);
void wait_ms(unsigned long time);
void wait(unsigned short time);

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


ISR(TIMER0_OVF_vect)     // sp0c0
{                       
d_TIMER0++;
}

ISR(TIMER1_COMPB_vect)   // Clear the Step outputs
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
ISR(TIMER1_OVF_vect)    // my SP0C0 @ 10 KHz
{
static char tog=0;
d_TIMER1++;

tog++;
if ( rs232_to==0 )
   {
   if ( tog & 0x01 ) PORTD |=  0x40; 
   else              PORTD &= ~0x40;
   }
else
   {
   if ( tog & 0x02 ) PORTD |=  0x40; 
   else              PORTD &= ~0x40;
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

#define TIME_BASE F_CPU_K
void init_disp(void)
{
TIMSK1 |= 1 <<  TOIE1;    // timer1 interrupt when Overflow                    ///////////////// SP0C0
// dont drive the pins TCCR1A  = 0xA3;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
TCCR1A  = 0x03;           // FAST PWM, Clear OC1A/OC1B on counter match, SET on BOTTOM
TCCR1B  = 0x19;           // Clock divider set to 1 , FAST PWM, TOP = OCR1A
OCR1A   = TIME_BASE/1;      // Clock divider set to 1, so F_CPU_K/1 is CLK / 1000 /1 thus, every 1000 there will be an interrupt : 1Khz
OCR1B   = TIME_BASE/2;     // By default, set the PWM to 50%
}

void wait_us(unsigned short time)     /// TO BE TESTED... with the scope TODO
{
unsigned short now = TCNT1;           // read timer1 counter...
unsigned long  target = now + time*(F_CPU_K/1000);
if ( time > TIME_BASE )               // the amount of time to wait is so long that we should use wait_ms()
   {                                  // this part is written in case... but really, wait_us() should be called with small values
   unsigned long d_now = d_TIMER1;
   unsigned long d_time = time/1000;
   while ( d_TIMER1-d_now < d_time ) ;
   return;
   }

if ( target > TIME_BASE )             // TCNT1 never goes over TIME_BASE ...
   { 
   target -= TIME_BASE;               // Set the target for after the loop around. . . 
   while ( TCNT1 >= now ) ;           // wait until it starts over
   }

while ( TCNT1 < target ) ;            // wait until we reach the count. . .
}

void wait_ms(unsigned long time)
{
unsigned long d_now = d_TIMER1;
while ( d_TIMER1-d_now < time ) ;
}
void wait(unsigned short time)
{
unsigned long d_now = d_TIMER1;
while ( d_TIMER1-d_now < time*1000 ) ;
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

unsigned char lcd_requires_reset=1;
#define LCD_SET_E_HIGH     PORTD |=  0x10
#define LCD_SET_E_LOW      PORTD &= ~0x10

#define LCD_SET_RW_HIGH    PORTB |=  0x01
#define LCD_SET_RW_LOW     PORTB &= ~0x01

#define LCD_SET_RS_HIGH    PORTD |=  0x04
#define LCD_SET_RS_LOW     PORTD &= ~0x04

#define LCD_SET_DATA_DIR_OUTPUT           \
{                                         \
DDRB |= 0x32;                             \
DDRD |= 0x80;                             \
}

#define LCD_SET_DATA_DIR_INPUT            \
{                                         \
DDRB &= ~0x32;                            \
DDRD &= ~0x80;                            \
}

unsigned char debug[20];   // temp

unsigned char lcd_read_4_bits(void)
{
unsigned char pb;
unsigned char pd;
unsigned char value=0;

LCD_SET_DATA_DIR_INPUT
wait_us(1);
LCD_SET_E_HIGH;
wait_us(2);
pb=PINB;
pd=PIND;
LCD_SET_E_LOW;

if ( pb & 0x02 ) value |= 0x01;
if ( pb & 0x10 ) value |= 0x02;
if ( pb & 0x20 ) value |= 0x04;
if ( pd & 0x80 ) value |= 0x08;
return value;
}


void lcd_write_4_bits(unsigned char data) // use lower 4 bits of data to drive the port of the LCD
{
unsigned char pb=PORTB & ~0x32;
unsigned char pd=PORTD & ~0x80;
if ( data & 0x01 ) pb |= 0x02;
if ( data & 0x02 ) pb |= 0x10;
if ( data & 0x04 ) pb |= 0x20;
if ( data & 0x08 ) pd |= 0x80;
LCD_SET_DATA_DIR_OUTPUT
PORTB = pb;
PORTD = pd;
wait_us(1);
LCD_SET_E_HIGH;
wait_us(4);
LCD_SET_E_LOW;
wait_us(1);
}

void lcd_wait_busy(void)
{
unsigned char HH;
volatile long timeout=10000;
if ( lcd_requires_reset ) return;
LCD_SET_DATA_DIR_INPUT;
LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_HIGH;  

do {
   timeout--;
   HH=lcd_read_4_bits();
      lcd_read_4_bits();
   } while ( (HH==0x08) && (timeout!=0) ) ;
if ( timeout==0 ) lcd_requires_reset=1;
}


void lcd_wait_send(unsigned char data, unsigned char ms)   // send comman function that does not check for the busy flag
{
wait_ms(ms);
LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_LOW;

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_send(unsigned char data)
{
lcd_wait_busy();

LCD_SET_E_LOW;
LCD_SET_RS_LOW;
LCD_SET_RW_LOW;

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_text(unsigned char data)
{
lcd_wait_busy();

LCD_SET_E_LOW;
LCD_SET_RS_HIGH;  // text
LCD_SET_RW_LOW;   // write

LCD_SET_DATA_DIR_OUTPUT;
lcd_write_4_bits(data>>4);
lcd_write_4_bits(data);
LCD_SET_DATA_DIR_INPUT;
}

void lcd_init(void)
{
lcd_requires_reset=0;
lcd_wait_send(0x30,20);   // Send 0x3 (last four bits ignored)
lcd_wait_send(0x30,6);    // Send 0x3 (last four bits ignored)
lcd_wait_send(0x30,2);    // Send 0x3 (last four bits ignored)
lcd_wait_send(0x20,2);    // Send 0x2 (last four bits ignored)  Sets 4-bit mode
lcd_wait_send(0x28,2);    // Send 0x28 = 4-bit, 2-line, 5x8 dots per char
// Busy Flag is now valid
lcd_send(0x08); // Send 0x08 = Display off, cursor off, blinking off
lcd_send(0x01); // Send 0x01 = Clear display
wait_us(500);  // LCD executiuon time
lcd_send(0x02); // Send 0x02 = Cursor Home
wait_us(500);  // LCD executiuon time
lcd_send(0x06); // Send 0x06 = Set entry mode: cursor shifts right, don't scroll
wait_us(500);  // LCD executiuon time
lcd_send(0x0C); // Send 0x0C = Display on, cursor off, blinking off
}


void lcd_print_str(char *STR)
{
if ( lcd_requires_reset ) 
   {
   ps("\012\015LCD requires Reset...");
   wait(1);
   lcd_init();
   }

while (*STR) lcd_text(*STR++);
}

void lcd_goto(unsigned char row,unsigned char col)
{
if ( lcd_requires_reset ) return;

if ( row==0 ) lcd_send(0x80+col);
else          lcd_send(0xC0+col);
wait_us(40);  // LCD executiuon time
}

int main(void)
{
char buf[16];
long val=0;
long lon=0;
///////////////////////////////// Init /////////////////////////////////////////

init_rs232();
init_disp();

ps("\033[2JLCD Tests...");

//set_analog_mode(MODE_8_BIT);                         // 8-bit analog-to-digital conversions
d_ram = get_free_memory();

PRR   = 0; // Enable all systems

LCD_SET_DATA_DIR_INPUT;

LCD_SET_E_LOW;
LCD_SET_RS_HIGH;

DDRD = 0x54;   // E and RS (outputs)   0x40 is PD6 test
DDRB = 0x01;  // RW (output)

sei();         //enable global interrupts

//lcd_init();

while(1)
   {
   val++;
   lon+=16;
   lcd_goto(0,0);
   lcd_print_str("Star:");
   pxxd(buf,val,6);
   lcd_print_str(buf);
   lcd_goto(1,0);
   lcd_print_str("Long:  ");
   p04x(buf,lon);
   lcd_print_str(buf);

   LCD_SET_DATA_DIR_INPUT;
   wait(1);

   lon = lon & 0xFFF0;
   if ((PINB & 0x02) == 0 ) lon |= 0x01;
   if ((PINB & 0x10) == 0 ) lon |= 0x02;
   if ((PINB & 0x20) == 0 ) lon |= 0x04;
   if ((PIND & 0x80) == 0 ) lon |= 0x08;
   ps("\012\015...");
   if ((PINB & 0x20) == 0 ) lcd_requires_reset=1;

   }
}


/*
$Log: my_lcd.c,v $
Revision 1.4  2012/07/16 05:26:47  pmichel
Version that uses the timeout on busy to decide to reset the LCD

Revision 1.3  2012/07/16 04:18:40  pmichel
Nice stand alone version that can also read 4 dips from the Data lines

Revision 1.2  2012/07/16 03:25:06  pmichel
First pmichel version that does work using the same pins as pololu
now I can change and use my pins, the logic is good...

Revision 1.1  2012/07/14 03:29:30  pmichel
Initial revision


*/

