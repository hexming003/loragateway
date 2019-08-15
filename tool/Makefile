#CROSS = /SRC9260/toolchain/4.1.1-lib2.5/bin/arm-linux-
CROSS = arm-linux-

CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

EXEC1 = address
OBJS1 = address.o

EXEC2 = repara
OBJS2 = repara.o

CFLAGS += -DPARANOID_TTYS -DUSE_TERMIO -DKLUDGELINEMODE -D_GNU_SOURCE
CFLAGS += -I /SRC9260/linux-2.6.21/include/
CFLAGS += -DLOG_CONSOLE
CFLAGS += -Wall -Wstrict-prototypes -Wimplicit -Wcomment -Wchar-subscripts
LDLIBS += -lm -pthread


all: clean $(EXEC1) $(EXEC2)

$(EXEC1): $(OBJS1)
	$(CC) $(LDFLAGS) -o $@ $(OBJS1) $(LDLIBS)
	cp $(EXEC1) ../../target/TestTool/

$(EXEC2): $(OBJS2)
	$(CC) $(LDFLAGS) -o $@ $(OBJS2) $(LDLIBS)
	cp $(EXEC2) ../../target/TestTool/

clean:
	-rm -f $(EXEC1) $(EXEC2) *.elf *.gdb *.o

