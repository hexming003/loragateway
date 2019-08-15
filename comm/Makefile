#CROSS = /SRC9260/toolchain/4.1.1-lib2.5/bin/arm-linux-
CROSS = /usr/local/arm_linux_4.8/bin/arm-linux-

CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

EXEC = comm
#OBJS01 = comm.o GPRS_SIM900A.o udp_net.o  ../public/namepipe.o ../public/public.o ../public/tc.o 
OBJS01 = comm.o GPRS_SIM900A.o udp_net.o  ../public/namepipe.o ../public/public.o 
#OBJS += ../../public/crontab.o

 
CFLAGS += -DPARANOID_TTYS -DUSE_TERMIO -DKLUDGELINEMODE -D_GNU_SOURCE
CFLAGS += -I /root/development/NUC970/bsp/nuc970bsp/linux-3.10.x/include
CFLAGS += -DLOG_CONSOLE
CFLAGS += -Wall -Wstrict-prototypes -Wimplicit -Wcomment -Wchar-subscripts
CFLAGS += -D__COMM
LDLIBS += -lm


all: clean $(EXEC)

$(EXEC): $(OBJS01)
	$(CC) $(LDFLAGS) -o $@ $(OBJS01) $(LDLIBS)
	cp $(EXEC) ../../target/EXEFiles/



clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
	-rm -f ../public/*.o ../public/*.gdb ../public/*.elf
	-for i in $(dir_v) $(sort $(dir_y) $(dir_n) $(dir_)) ; do \
		[ ! -d $$i ] || make -C $$i clean; \
	done

