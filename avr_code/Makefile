MCU=atmega16
PART=ATmega16
BOOTLOADER_ADDRESS=0x3800
PORT=/dev/ttyUSB0

UISP = uisp -dprog=stk500 -dserial=/dev/ttyUSB0 -dpart=$(PART)
CC=avr-gcc
OBJCOPY=avr-objcopy
# optimize for size:
CFLAGS= -mmcu=$(MCU) -I. -I../ -I../midi/ -Wall -Wstrict-prototypes -Os -mcall-prologues
#LDFLAGS += --section-start=.text=$(BOOTLOADER_ADDRESS)
LDFLAGS += -Ttext=$(BOOTLOADER_ADDRESS)

SRC = main.c \
		../midi/bytequeue/interrupt_setting.c \
		../midi/bytequeue/bytequeue.c \
		../midi/sysex_tools.c

OBJ  = ${SRC:.c=.o}

#-------------------
all: main.hex
#-------------------

.c.o:
	@echo CC $<
	@$(CC) -c $(CFLAGS) -Os -o $*.o $<

main.hex : main.out 
	$(OBJCOPY) -R .eeprom -O ihex main.out main.hex 

main.out : $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o main.out -Wl,-Map,main.map $(OBJ)

# you need to erase first before loading the program.
load: main.hex
	$(UISP) --erase
	$(UISP) --upload --verify if=main.hex -v=3 --hash=32

fuse_mega16:
	$(UISP) --wr_fuse_l=0xef --wr_fuse_h=0xd8

	#$(UISP) --wr_fuse_l=0x9f --wr_fuse_h=0xd0

check:
	$(UISP) --rd_fuses
#-------------------
clean:
	rm -f *.o *.map *.out *.hex
#-------------------
