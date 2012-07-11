DEVICE = atmega328p
#  avrdude: Device signature = 0x1e9514  A328-PU
#  avrdude: Device signature = 0x1e950f  A328P-PU
# AVRDUDE_DEVICE = m328
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

TARGET=telescope2
OBJECT_FILES=telescope2_master.o telescope2_slave.o

all: telescope2_master.hex telescope2_slave.hex

clean:
	rm -f *.o *.hex *.obj *.hex

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

telescope2_master.o: telescope2_master.c coords.h
telescope2_slave.o: telescope2_slave.c coords.h

#%.obj: $(OBJECT_FILES)
#	$(CC) $(CFLAGS) -D$@ $(ASFLAGS) ($@.obj:.c)
#	$(CC) $(CFLAGS) -D$@ $(OBJECT_FILES) $(LDFLAGS) -o $@

telescope2_master.obj: telescope2_master.c coords.h
	$(CC) $(CFLAGS) -DAT_MASTER $(ASFLAGS) telescope2_master.c
	$(CC) $(CFLAGS) -DAT_MASTER telescope2_master.c $(LDFLAGS) -o $@
telescope2_slave.obj: telescope2_slave.c coords.h
	$(CC) $(CFLAGS) -DAT_SLAVE $(ASFLAGS) telescope2_slave.c
	$(CC) $(CFLAGS) -DAT_SLAVE telescope2_slave.c $(LDFLAGS) -o $@

master: 
	./chmod_ttyACM0
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET)_master.hex

mst: 
	avrdude -v -p m328p -c jtag2isp -P usb -U flash:w:telescope2_master.hex

slave: 
	./chmod_ttyACM0
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET)_slave.hex
slv: 
	avrdude -v -p m328p -c jtag2isp -P usb -U flash:w:telescope2_slave.hex



# Normal Fuses for Polopu A328P
#
#  avrdude: Device signature = 0x1e950f
#  avrdude: safemode: lfuse reads as F6  CKDIV8=0x80 >>> No DIV8   CKOUT=0c40 >> No CLK OUT  CKSEL 0x03 =    SUT 0x30 >> Ceramic Crystal, fast rising
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

# test:
# avrdude -v -p x128a1 -c jtag2 -P usb -U flash:w:telescope2_master.hex
