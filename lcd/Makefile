DEVICE = atmega328p
#  avrdude: Device signature = 0x1e9514  A328-PU
#  avrdude: Device signature = 0x1e950f  A328P-PU
AVRDUDE_DEVICE = m328
#AVRDUDE_DEVICE = m328p
# DEVICE ?= atmega168
# AVRDUDE_DEVICE ?= m168

# -fverbose-asm
#CFLAGS=-g -Wall -mcall-prologues -mmcu=$(DEVICE) -Os
CFLAGS=-g -Wall -mcall-prologues -mmcu=$(DEVICE) -O2 -I /opt/cross/avr/include
CC=avr-gcc
OBJ2HEX=avr-objcopy 
#LDFLAGS=-Wl,-gc-sections -lpololu_$(DEVICE) -Wl,-relax
LDFLAGS=-Wl,-gc-sections -Wl,-relax
ASFLAGS=-Wa,-ah -Wa,-D -fverbose-asm -dA -S 

PORT ?= /dev/ttyACM0
AVRDUDE=avrdude

TARGET=my_lcd
OBJECT_FILES=my_lcd.o 

all: my_lcd.hex

clean:
	rm -f *.o *.hex *.obj *.hex *.s

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

my_lcd.o: my_lcd.c

my_lcd.obj: my_lcd.c 
	$(CC) $(CFLAGS) $(ASFLAGS) my_lcd.c
	$(CC) $(CFLAGS) my_lcd.c $(LDFLAGS) -o $@

program: 
	./chmod_ttyACM0
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex

mst: 
	avrdude -v -p m328p -c jtag2isp -P usb -U flash:w:my_lcd.hex



# Normal Fuses for Polopu A328P
#
#  avrdude: Device signature = 0x1e950f
#  avrdude: safemode: lfuse reads as F6  CKDIV8=0x80 >>> No DIV8   CKOUT=0c40 >> No CLK OUT  CKSEL 0x03 =    SUT 0x30 >> Ceramic Crystal, fast rising
#  avrdude: safemode: hfuse reads as D9
#  avrdude: safemode: efuse reads as 7

#  For the Telescope Board, the low fuse has to be:
#  MASTER : avrdude: safemode: lfuse reads as B6  CKDIV8=0x80 >>> No DIV8   CKOUT=0x40 >> CLK OUT ACTIVE  SUT 0x30 : slow rising    CKSEL 0x06 =   Ceramic Crystal
#  SLAVE  : avrdude: safemode: lfuse reads as E0  CKDIV8=0x80 >>> No DIV8   CKOUT=0x40 >> NO CLK OUT      SUT 0x20 : slow rising    CKSEL 0x00 =   External clock
#  >>>>>    once programmed as a slave, then it must have an external clock to survive -> a slave will always need a clock source to change the fuse
#  For "Breadboard" minimal setup (no crystal, the low fuse has to be:
#  BREADBOARD : avrdude: lfuse reads as 0x62  CKDIV8=0x80 >>> DIV8   CKOUT=0x40 >> CLK OUT INACTIVE  SUT 0x30 : slow rising    CKSEL 0x02 =   Internal 8Mhz
#                        hfuse reads as 0xD9
#                        efuse reads as 0x07
#
#  BREADBOARD with crystal test:  lfuse reads as 0x66  CKDIV8=0x80 >>> DIV8   CKOUT=0x40 >> CLK OUT INACTIVE  SUT 0x30 : slow rising    CKSEL 0x06 =   External 20Mhz
#  avrdude -v -p m328 -c avrisp2 -P /dev/ttyACM0 -U lfuse:w:0x66:m
#


# to read the Fuses: to the files high and low :
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0  -U hfuse:r:high:h -U lfuse:r:low:h

# to change the fuse setting:
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0 -U lfuse:w:0xb6:m

# test:
# avrdude -v -p x128a1 -c jtag2 -P usb -U flash:w:timer.hex
