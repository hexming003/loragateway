#CROSS = /SRC9260/toolchain/4.1.1-lib2.5/bin/arm-linux-
CROSS = /usr/local/arm_linux_4.8/bin/arm-linux-

CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

EXEC = main
//OBJS = main.o serial.o prototol.o ../public/public.o ../public/namepipe.o 
OBJS = main.o spidev.o gpio.o lora.c radio/radio.o radio/sx1276.o radio/sx1276-Hal.o radio/sx1276-LoRa.o radio/sx1276-LoRaMisc.o serial.o prototol.o ../public/public.o ../public/namepipe.o ../public/inifile.o
 
CFLAGS += -DPARANOID_TTYS -DUSE_TERMIO -DKLUDGELINEMODE -D_GNU_SOURCE -Iradio
#CFLAGS += -I /root/development/NUC970/bsp/nuc970bsp/linux-3.10.x/include -I/root/development/NUC970/bsp/nuc970bsp/linux-3.10.x/arch/arm/mach-at91/include/mach
CFLAGS += -DLOG_CONSOLE
CFLAGS += -w -Wstrict-prototypes -Wimplicit -Wcomment -Wchar-subscripts
#CFLAGS += -O2
LDLIBS += -lm

all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS-$(@)))
	cp $(EXEC) ../../target/EXEFiles/

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o ../public/*.o ../public/*.gdb ../public/*.elf
