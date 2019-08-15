/*
用于修改集中器参数
运行方法：  ./repara ServerIP 192.168.1.10 
运行后会将参数文件中的服务器地址改为 ServerIP = 192.168.1.10

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

#define PARASET_FILE        "/MeterRoot/CFGFiles/paraset.txt"
typedef    signed char            c8;
typedef    signed char            s8;
typedef    unsigned char          u8;
typedef    signed short           s16;
typedef    unsigned short         u16;
typedef    signed int             s32;
typedef    unsigned int           u32;
typedef    signed long long       s64;
typedef    unsigned long long     u64;

int main(s32 argc, char *argv[])
{
	FILE *fp;
	c8 buf[128];
	c8 wbuf[128];
	c8 *pt;
	int len, i;
	
	if(argc < 3) 
	{
		printf("格式错误 eg.  ./repara ServerIP 192.168.1.10\n");
		exit(0);
	}
	
	fp = fopen(PARASET_FILE, "rb+");
	if(fp == NULL)
	{
		printf("get PARA error %s\n",  PARASET_FILE);
		exit(0);
	}
	while(fgets(buf, sizeof(buf), fp))
	{
		//printf("get str:  %s", buf);
		
		if(memcmp(buf, argv[1], strlen(argv[1])) == 0)
		{
			pt = strstr(buf, "//");
			if(pt != 0) len = pt - buf;
			else len = 29;
			
			sprintf(wbuf, "%s = %s", argv[1], argv[2]);
			for(i=strlen(wbuf); i<len; i++)
			{
				wbuf[i] = ' ';
			}
			fseek(fp, 0 - strlen(buf), SEEK_CUR);
			fwrite(wbuf, 1, len, fp);
			
			break;
		}
	}
	
	fclose(fp);
}
