DEVICE = atmega328p
AVRDUDE_DEVICE = m328p
# DEVICE ?= atmega168
# AVRDUDE_DEVICE ?= m168

# -fverbose-asm
#CFLAGS=-g -Wall -mcall-prologues -mmcu=$(DEVICE) -Os
CFLAGS=-g -Wall -mcall-prologues -mmcu=$(DEVICE) -O2 -I /opt/cross/avr/include
CC=avr-gcc
OBJ2HEX=avr-objcopy 
LDFLAGS=-Wl,-gc-sections -lpololu_$(DEVICE) -Wl,-relax
ASFLAGS=-Wa,-ah -Wa,-D -fverbose-asm -dA -S 

PORT ?= /dev/ttyACM0
AVRDUDE=avrdude

TARGET=telescope
OBJECT_FILES=telescope.o

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.obj *.hex

%.hex: %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

telescope.o: telescope.c

%.obj: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(ASFLAGS) telescope.c 
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LDFLAGS) -o $@

program: $(TARGET).hex
	./chmod_ttyACM0
	$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex
