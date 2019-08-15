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
//#include <asm/arch/at91sam9260.h>
//#include <asm/arch/at91_pio.h>
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

//address -L   //显示地址配置表的节点
//address -A xxxx  //在地址配置表中增加一个节点
//address -D xxxx  //在地址配置表中删除一个节点

#define NODECFG_FILE        "/MeterRoot/CFGFiles/node.cfg"

u8 Default_LCfg[45] = {         \
0 , 12 , 0xFF, 0xFF, 0xFF,      \
12 ,0 ,  0xFF, 0xFF, 0xFF,      \
0,0,0,0,0};

struct nodelist_st   //142字节
{
	u8 naddr[6];
	u8 pollt;     	  //轮询次数，当节点有数据返回时，轮询次数为0xaa, 表示轮询成功过
	u8 roomid[3]; 	  //房间ID
	u8 version[4];    //版本号+编译时间（年月日）
	u16 lightver;     //亮度配置版本号
	u8 lightcfg[80]; //指向亮度配置表
} __attribute__((packed));


//所有数据都是 val， 返回0
//否则返回-1
int is_all_xx(u8 *buf, u8 val, int len)
{
	int i;

	for (i = 0; i < len; i++)
	{
		if (buf[i] != val) break;
	}
	if (i == len)
	{
		return 0;
	}

	return -1;
}

//将最多两个字符的hex字符转换成一个u8数据
u8 mychartohex(u8 *cstr, c8 **pt)
{
	u8 ret = 0;
	c8 bufs[4];

	while(*cstr)
	{
		if(    ((*cstr >= '0') && (*cstr <= '9'))
			|| ((*cstr >= 'A') && (*cstr <= 'F'))
			|| ((*cstr >= 'a') && (*cstr <= 'f')) )
		{
			memcpy(bufs, cstr, 3);

			if(    ((bufs[1] >= '0') && (bufs[1] <= '9'))
				|| ((bufs[1] >= 'A') && (bufs[1] <= 'F'))
				|| ((bufs[1] >= 'a') && (bufs[1] <= 'f')) )
			{
				bufs[2] = 0;
				if(pt != NULL) *pt = cstr+2;
			}
			else
			{
				bufs[1] = 0;
				if(pt != NULL) *pt = cstr+1;
			}

			ret = strtol(bufs, NULL, 16);

			return ret;
		}

		cstr ++;
	}

	return ret;
}

u8 strtohex(u8 *outbuf, c8 *instr)
{
	u8 len = 0;
	c8 *pt = instr;

	while(*pt)
	{
		if( *pt == '/') return len;
		if(    ((*pt >= '0') && (*pt <= '9'))
			|| ((*pt >= 'A') && (*pt <= 'F'))
			|| ((*pt >= 'a') && (*pt <= 'f')) )
		{
			*outbuf++ = mychartohex(pt, &pt);
			len ++;
		}
		else
		{
			pt ++;
		}

		if( (*pt == 0) || (pt == NULL) ) break;
	}

	return len;
}

/*
void test_lib(void)
{
	void * libm_handle = NULL;
	
	libm_handle = dlopen("libGreenPLAN.so", RTLD_LAZY);
}
*/

int main(int argc, char *argv[])
{
	struct nodelist_st nst;
	FILE *fp;
	int i = 0;
	int ret;

	
    if (argc < 2)
    {
    	printf("参数错误!\n");
    	printf("	L 显示地址配置表的节点\n");
    	printf("	A [xxxxxxxx]xxxx  增加一个节点\n");
    	printf("	D [xxxxxxxx]xxxx  删除一个节点\n");
    	printf("	C [xxxxxxxx]xxxx  清除节点读取成功标识\n");
    	return -1;
    }
    
    if( (*argv[1] == 'L') || (*argv[1] == 'l'))
    {
    	fp = fopen(NODECFG_FILE, "rb+");
		if (fp == NULL)
		{
			printf("get node list error %s\n", NODECFG_FILE);
			return -1;
		}
		
		while(fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
		{
			printf("node%03d: %02X %02X %02X %02X %02X %02X pollt:%02X\r\n", i++, nst.naddr[0], nst.naddr[1], nst.naddr[2], nst.naddr[3], nst.naddr[4], nst.naddr[5], nst.pollt);
		}
		
		fclose(fp);
		return -1;   
	}
	else if( (*argv[1] == 'C') || (*argv[1] == 'c'))
    {
    	fp = fopen(NODECFG_FILE, "rb+");
		if (fp == NULL)
		{
			printf("get node list error %s\n", NODECFG_FILE);
			return -1;
		}
		
		while(fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
		{
			nst.pollt = 0;
			fseek(fp, -sizeof(struct nodelist_st),  SEEK_CUR);
			fwrite((u8*)&nst, 1, sizeof(struct nodelist_st), fp);
		}
		
		fclose(fp);
		return -1;   
	}
	else if( (*argv[1] == 'A') || (*argv[1] == 'a'))
	{
		u8 addr[6];
		//pringf("argc=%d, stlen=%d\n",argc, strlen(argv[2])
    	if( (argc != 3) || ((strlen(argv[2]) != 4) && (strlen(argv[2]) != 12)) )
    	{
    		printf("参数错误!\n");
	    	printf("	-L 显示地址配置表的节点\n");
	    	printf("	-A [xxxxxxxx]xxxx  增加一个节点\n");
	    	printf("	-D [xxxxxxxx]xxxx  删除一个节点\n");
	    	return -1;
	    }
    	
    	fp = fopen(NODECFG_FILE, "rb+");
		if (fp == NULL)
		{
			printf("get node list error %s\n", NODECFG_FILE);
			return -1;
		}
    	
    	if(strlen(argv[2]) == 4)
	    {
	    	strtohex(addr+4, argv[2]);
	    	fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp);
	    	memcpy(addr, nst.naddr, 4);
	    	fseek(fp, 0, SEEK_SET);
	    }
	    else if(strlen(argv[2]) == 12)
	    {
	    	strtohex(addr, argv[2]);
	    }
	    
		while(fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
		{
			if(is_all_xx(nst.naddr, 0, 6) == 0)  //到了列表的尾
			{
				memcpy(nst.naddr, addr, 6);
				memcpy(nst.lightcfg, Default_LCfg, sizeof(Default_LCfg));
				fseek(fp, -sizeof(struct nodelist_st),  SEEK_CUR);
				fwrite((u8*)&nst, 1, sizeof(struct nodelist_st), fp);
				
				memset((u8*)&nst, 0, sizeof(struct nodelist_st));
				fwrite((u8*)&nst, 1, sizeof(struct nodelist_st), fp);
				fclose(fp);
				return -1;
			}
		}
		
		memcpy(nst.naddr, addr, 6);
		memcpy(nst.lightcfg, Default_LCfg, sizeof(Default_LCfg));
		fwrite((u8*)&nst, 1, sizeof(struct nodelist_st), fp);
		
		memset((u8*)&nst, 0, sizeof(struct nodelist_st));
		fwrite((u8*)&nst, 1, sizeof(struct nodelist_st), fp);
		fclose(fp);
		return 0;
	}
	else if( (*argv[1] == 'D') || (*argv[1] == 'd'))
    {
    	u8 addr[6];
    	
    	if( (argc != 3) || ((strlen(argv[2]) != 4) && (strlen(argv[2]) != 12)) )
    	{
    		printf("参数错误!\n");
	    	printf("	-L 显示地址配置表的节点\n");
	    	printf("	-A [xxxxxxxx]xxxx  增加一个节点\n");
	    	printf("	-D [xxxxxxxx]xxxx  删除一个节点\n");
	    	return 0;
	    }
	    if(strlen(argv[2]) == 4)
	    {
	    	strtohex(addr+4, argv[2]);
	    }
	    else if(strlen(argv[2]) == 12)
	    {
	    	strtohex(addr, argv[2]);
	    }
	    
    	fp = fopen(NODECFG_FILE, "rb+");
		if (fp == NULL)
		{
			printf("get node list error %s\n", NODECFG_FILE);
			return 0;
		}
		
		for(i=0; i<1024; i++)
		{
			while(fread((u8*)&nst, 1, sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
			{
				if(strlen(argv[2]) == 4)
			    {
			    	if( (nst.naddr[4] == addr[4]) && (nst.naddr[5] == addr[5]) )
					{
						while(fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
						{
							fseek(fp, -2*sizeof(struct nodelist_st),  SEEK_CUR);
							fwrite((u8*)&nst, 1,sizeof(struct nodelist_st), fp);
							fseek(fp, sizeof(struct nodelist_st),  SEEK_CUR);
						}
						fclose(fp);
						return 0;
					}
			    }
			    else if(strlen(argv[2]) == 12)
			    {
			    	if( memcmp(nst.naddr, addr, 6) == 0 )
					{
						while(fread((u8*)&nst, 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
						{
							fseek(fp, -2*sizeof(struct nodelist_st),  SEEK_CUR);
							fwrite((u8*)&nst, 1,sizeof(struct nodelist_st), fp);
							fseek(fp, sizeof(struct nodelist_st),  SEEK_CUR);
						}
						fclose(fp);
						return 1;
					}
			    }
			}
		}    
	}
	else
	{
		printf("参数错误!\n");
    	printf("	-L 显示地址配置表的节点\n");
    	printf("	-A [xxxxxxxx]xxxx  增加一个节点\n");
    	printf("	-D [xxxxxxxx]xxxx  删除一个节点\n");
    	return 0;
    }
    
    return 0;
}

