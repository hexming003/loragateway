/*
This test.c is used to test the receive of the serial.
It proved that whenever it meet the 0xod,it will take it place
with 0x0a. the others are correct.

*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/termbits.h>
#include <error.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91_pio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <asm/termbits.h>


typedef    signed char            c8;
typedef    signed char            s8;
typedef    unsigned char          u8;
typedef    signed short           s16;
typedef    unsigned short         u16;
typedef    signed int             s32;
typedef    unsigned int           u32;
typedef    signed long long       s64;
typedef    unsigned long long     u64;

void main(s32 argc, char *argv[])
{
	FILE *fp;
	s8 str[8] = "00";
	
	if(argc >= 2) 
	{
		str[0] = argv[1][0];
		str[1] = argv[1][1];
	}

	if(access("/MeterRoot/CFGFiles/version", F_OK) != 0)
	{
		c8 inst[10] = "00000000\n";
		fp = fopen("/MeterRoot/CFGFiles/version", "wb+");
		if(fp != NULL) fwrite(inst, 1, 9, fp);
	}
	else
	{
		fp = fopen("/MeterRoot/CFGFiles/version", "rb+");
	}
	
	if(fp != NULL)
	{
		fseek(fp, 6, SEEK_SET);
		fwrite(str, 1, 2, fp);
		fclose(fp);
	}	
}

