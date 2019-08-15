#CROSS = /SRC9260/toolchain/4.1.1-lib2.5/bin/arm-linux-
CROSS = /SRC9260/toolchain/usr/local/arm/3.4.1/bin/arm-linux-

CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

EXEC = Wdog
OBJS = Wdog.o 
OBJS += usbup.o ../public/public.o ../public/tc.o


CFLAGS += -DPARANOID_TTYS -DUSE_TERMIO -DKLUDGELINEMODE -D_GNU_SOURCE
CFLAGS += -I /SRC9260/linux-2.6.21/include/
CFLAGS += -Wall -Wstrict-prototypes -Wimplicit -Wcomment -Wchar-subscripts
CFLAGS += -g

all: clean $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	cp $(EXEC) ../../target/EXEFiles

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *.elf2flt 
	-rm -f ../public/*.o ../public/*.gdb ../public/*.elf
	

