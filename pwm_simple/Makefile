DEVICE = atmega328p
#  avrdude: Device signature = 0x1e9514  A328-PU
#  avrdude: Device signature = 0x1e950f  A328P-PU
#AVRDUDE_DEVICE = m328
AVRDUDE_DEVICE = m328p
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

TARGET=pwmd
OBJECT_FILES=pwmd.o

all: pwmd.hex pwmd.hex

clean:
	rm -f *.o *.hex *.obj *.hex *.s

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

pwmd.o: pwmd.c 

pwmd.obj: pwmd.c
	$(CC) $(CFLAGS) $(ASFLAGS) pwmd.c
	$(CC) $(CFLAGS)            pwmd.c $(LDFLAGS) -o $@

program: $(TARGET).hex
	#./chmod_ttyACMx
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex


# Normal Fuses for Polopu A328P
#
#  avrdude: Device signature = 0x1e950f
#  avrdude: safemode: lfuse reads as E6  CKDIV8=0x80 >>> No DIV8   CKOUT=0c40 >> No CLK OUT  CKSEL 0x03 =    SUT 0x30 >> Ceramic Crystal, fast rising
#  avrdude: safemode: hfuse reads as D9
#  avrdude: safemode: efuse reads as 7

#  For the Telescope Board, the low fuse has to be:
#  MASTER : avrdude: safemode: lfuse reads as B6  CKDIV8=0x80 >>> No DIV8   CKOUT=0x00 >> CLK OUT ACTIVE  SUT 0x30 : slow rising    CKSEL 0x06 =   Ceramic Crystal
#  SLAVE  : avrdude: safemode: lfuse reads as E0  CKDIV8=0x80 >>> No DIV8   CKOUT=0x40 >> NO CLK OUT      SUT 0x20 : slow rising    CKSEL 0x00 =   External clock
#  >>>>>    once programmed as a slave, then it must have an external clock to survive -> a slave will always need a clock source to change the fuse


# to read the Fuses: to the files high and low :
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0  -U hfuse:r:high:h -U lfuse:r:low:h

# to change the fuse setting:
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0 -U lfuse:w:0xb6:m
#
# This will set the a328p for: Use internal clock 8Mhz, no clock divider, with clock out  0xE2   see p33
# avrdude -v -p m328  -c avrisp2 -P /dev/ttyACM0 -U lfuse:w:0xe2:m    

# test:
# avrdude -v -p x128a1 -c jtag2 -P usb -U flash:w:pwmd.hex
###############################################################################################
# A328p extended fuse
# -----------------
# | | | | | |0|1|0|   <<< default value 0x07    (p323)
#            ^-------- BODLEVEL2  | | | | | |1|1|1| : BOD Disabled
#              ^------ BODLEVEL1  | | | | | |1|1|0| : 1.7v/1.8v/2.0v
#                ^---- BODLEVEL0  | | | | | |1|0|1| : 2.5v/2.7v/2.9v
#                                 | | | | | |1|0|0| : 4.1v/4.3v/4.5v
#                                 | | | | | |0|x|x| : reserved
#
###############################################################################################
# A328p Low Fuse  
# -----------------
# |0|1|1|0|0|0|1|0|   <<< default value 0x62    
#  ^--------------------- CKDIV8  |0| | | | | | | | : divide base clock by 8
#                                 |1| | | | | | | | : do not divide base clock by 8
#    ^------------------- CKOUT   | |0| | | | | | | : output clock on PB0
#                                 | |1| | | | | | | : do not output clock on PB0
#      ^----------------- SUT1    | | |0|0| | | | | : 14CK 
#        ^--------------- SUT0    | | |0|1| | | | | : 14CK+4.1ms
#                                 | | |1|0| | | | | : 14CK+65ms
#                                 | | |1|1| | | | | : Reserved
#          ^------------- CKSEL3  | | | | |1|1|1|1| : Low power crystal osc    (p27)  
#            ^----------- CKSEL2             to
#              ^--------- CKSEL1  | | | | |1|0|0|0| : Low power crystal osc    
#                ^------- CKSEL0  | | | | | |1|1|1| : Full swing crystal osc
#                                 | | | | | |1|1|0| : Full swing crystal osc
#                                 | | | | | |1|0|1| : low Freq crystal osc
#                                 | | | | | |1|0|0| : low Freq crystal osc
#                                 | | | | | |0|1|1| : Internal 128Khz
#                                 | | | | | |0|1|0| : Internal 8Mhz
#                                 | | | | | |0|0|1| : Reserved
#                                 | | | | | |0|0|0| : external clock
###############################################################################################
# A328p High Fuse  
# -----------------
# |1|1|0|1|1|0|0|1|   <<< default value 0xD9    (p299)
#  ^------------------- RSTDISBL  |0| | | | | | | | : Reset pin disabled
#                                 |1| | | | | | | | : Reset pin enabled
#    ^----------------- DEWN      | |0| | | | | | | : Debug Wire enable
#                                 | |1| | | | | | | : Debug Wire disabled
#      ^--------------- SPIEN     | | |0| | | | | | : Enable serial programming and downloading
#                                 | | |1| | | | | | : Disable serial programming and downloading
#        ^------------- WDTON     | | | |0| | | | | : Watchdog always on
#                                 | | | |1| | | | | : Watchdog not always on ? or never on ?
#          ^----------- EESAVE    | | | | |0| | | | : EEPROM memory preserved through chip erase
#                                 | | | | |1| | | | : EEPROM memory not preserved 
#            ^--------- BOOTSZ1   | | | | | |1|1| | : 256  words / 4  pages / Application: 0x0000-0x3EFF / Bootloader: 0x3F00-3FFFF
#              ^------- BOOTSZ0   | | | | | |1|0| | : 512  words / 8  pages / Application: 0x0000-0x3DFF / Bootloader: 0x3E00-3FFFF
#                                 | | | | | |0|1| | : 1024 words / 16 pages / Application: 0x0000-0x3BFF / Bootloader: 0x3C00-3FFFF
#                                 | | | | | |0|0| | : 2048 words / 32 pages / Application: 0x0000-0x37FF / Bootloader: 0x3800-3FFFF
#                ^----- BOOTRST   | | | | | | | |0| : Select reset vector






