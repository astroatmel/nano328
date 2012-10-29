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

TARGET=motor_test
OBJECT_FILES=a328p_lcd.o a328p_rt.o motor_test.o

all: motor_test.hex

# Those defines control which modules are active
MODULES_OPTIONS=-DUSE_LCD -DUSE_RT

clean:
	rm -f *.o *.hex *.obj *.hex *.s

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

motor_test.obj: motor_test.c a328p_lcd.c a328p_lcd.h a328p_rt.c a328p_rt.h
	$(CC) $(CFLAGS) $(ASFLAGS) $(MODULES_OPTIONS) motor_test.c
	$(CC) $(CFLAGS) $(ASFLAGS) $(MODULES_OPTIONS) a328p_lcd.c
	$(CC) $(CFLAGS) $(ASFLAGS) $(MODULES_OPTIONS) a328p_rt.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(MODULES_OPTIONS) -o $@  motor_test.c a328p_lcd.c a328p_rt.c


program: 
	./chmod_ttyACMx
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex

mst: 
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c jtag2isp -P usb    -U flash:w:$(TARGET).hex



# to read the Fuses: to the files high and low :
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0  -U hfuse:r:high:h -U lfuse:r:low:h

# to change the fuse setting:
# avrdude -v -p m328p -c avrisp2 -P /dev/ttyACM0 -U lfuse:w:0xe0:m

# test:
# avrdude -v -p x128a1 -c jtag2 -P usb -U flash:w:timer.hex



