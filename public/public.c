#include "public.h"

u8 nLogConsole;
u8 nDebugLevel;
extern char* GetIniSectionItem(char* filename,char* item);

/*
u8 Default_LCfg[45] = {  \
0 , 6 ,  10, 15, 4,      \
6 , 8 ,  20, 29, 8,      \
8 , 10,  25, 36, 1,      \
10, 14,  40, 45, 1,      \
14, 16,  25, 36, 1,      \
16, 18,  20, 29, 8,      \
18, 20,  15, 19, 4,      \
20, 0 ,  10, 15, 4,
0,0,0,0,0};
*/

u8 Default_LCfg[15] = {  \
0 , 12 ,  255, 255, 255,      \
12 , 0 ,  255, 255, 255,      \
0,0,0,0,0};

void display(c8 *mlog, u8 *pBuf, u8 len)
{
	c8 pDispData[256];
	s32 i;

	if(mlog != NULL) printf("%s", mlog);

	bzero(pDispData, 256);
	for (i = 0; i < len; i++)
	{
		if (((i % 120) == 0) && (i != 0))
		{
			printf("%s\n", pDispData);
			bzero(pDispData, 256);
		}
		sprintf(pDispData, "%s%02X", pDispData, pBuf[i]);
	}
	printf("%s\n", pDispData);
}


s32 read_from_file(c8 *filepath, s32 offset, s32 whence, u8 *pbuf, s32 nlen)
{
	FILE *fp = NULL;
	s32 nret = -1;

	if ((pbuf == NULL) || (nlen <= 0))
	{
		return nret;
	}

	if ((fp = fopen((u8*)filepath, "rb")) == NULL)
	{
		write_log(MSG_COMM, "Open %s error(%s)\n", filepath, strerror(errno));
		return nret;
	}

	if (fseek(fp, offset, whence) < 0)
	{
		write_log(MSG_COMM, "Fsek %s error\n", filepath);
		goto out;
	}
	if ((nret = fread(pbuf, 1, (u32)nlen, fp)) < 0)
	{
		write_log(MSG_COMM, "Read file %s error\n", filepath);
	}
out:
	if (fp)
	{
		fclose(fp);
	}

	return nret;
}


/*******************************************************************************
Description : Write the buf pointed by pbuf to file pointed by filepath, and the file will be truncated before write
Input       : mode: select the mode for opening the file.
Output      : N/A
Return      : Bytes written will be returned and -1 for errors
*******************************************************************************/
s32 write_to_file(c8 *filepath, s32 offset, s32 whence, u8 *pbuf, s32 nlen, c8 *mode)
{
	FILE    *fp = NULL;
	s32 nret = -1;

	if ((pbuf == NULL) || (nlen <= 0) || (mode == NULL))
	{
		return nret;
	}

	if ((fp = fopen((u8*)filepath, (u8*)mode)) == NULL)
	{
		write_log(MSG_COMM, "Open %s error(%s)\n", filepath, strerror(errno));
		return nret;
	}

	if (fseek(fp, offset, whence) < 0)
	{
		write_log(MSG_COMM, "Fseek %s error\n", filepath);
		goto out;
	}
	if (fwrite(pbuf, 1, (u32)nlen, fp) != nlen)
	{
		write_log(MSG_COMM, "Write %s error\n", filepath);
		goto out;
	}
	nret = nlen;
out:
	if (fp)
	{
		fclose(fp);
	}
	return nret;
}


//功能说明  用于执行sh命令；
//参数说明  string表示需要执行的sh命令行；
//返回值说明    成功返回0，失败返回-1；
s32 system_wait(c8 *string)
{
	FILE *fp;
	s32 pid;
	u8 pathbuf[32];
	time_t MyRandom;

	time(&MyRandom);
	bzero(pathbuf, sizeof(pathbuf));
	sprintf(pathbuf, "/tmp/%ld.tsh", MyRandom);
	fp = fopen(pathbuf, "w");
	if (fp == NULL)
	{
		write_log(MSG_SYSERR, "Open Tmpfile Err\n");
		return R_FAIL;
	}
	fprintf(fp, "%s", string);
	fclose(fp);

	if ((pid = vfork()) < 0)
	{
		unlink(pathbuf);
		return R_FAIL;
	}
	else if (pid == 0)
	{
		execl("/bin/sh", "/bin/sh", pathbuf, 0);
		exit(R_SUCC);
	}
	else
	{
		if (waitpid(pid, NULL, 0) != pid)
		{
			unlink(pathbuf);
			return R_FAIL;
		}
		unlink(pathbuf);
		return R_SUCC;
	}
}


s32 write_comm(u8 debug_level, c8 *mlog, u8 *pBuf, s32 nLen)
{
	if (nLen > 1024)  return R_FAIL;

	if (((debug_level&0xf0) & (nDebugLevel&0xf0))
			&& ((debug_level&0x0f) <= (nDebugLevel&0x0f)))
	{
		c8 pDispData[256];
		s32 i;
		
		bzero(pDispData, 256);
		if (nLogConsole == 1)
		{
			printf("%s ", mlog);
		}
		else
		{
			//syslog(LOG_INFO, "%s ", mlog);
			sprintf(pDispData, "%s ", mlog);
		}

		for (i = 0; i < nLen; i++)
		{
			if (((i % 96) == 0) && (i != 0))
			{
				sprintf(pDispData, "%s\n", pDispData);
				if (nLogConsole == 1)
				{
					printf("%s", pDispData);
				}
				else
				{
					syslog(LOG_INFO, "%s", pDispData);
				}
				bzero(pDispData, 256);
			}
			sprintf(pDispData, "%s%02x", pDispData, pBuf[i]);
		}

		sprintf(pDispData, "%s\n", pDispData);
		if (nLogConsole == 1)
		{
			printf("%s", pDispData);
		}
		else
		{
			syslog(LOG_INFO, "%s", pDispData);
		}

		return R_SUCC;
	}
	return R_FAIL;
}

#if 0
s32 write_log(u8 debug_level, c8 *mlog, ...)
{
	if (debug_level <= (nDebugLevel&0x0f))
	{
		va_list arg;

		if (nLogConsole == 1)
		{
			va_start(arg, mlog);
			vprintf(mlog, arg);
			va_end(arg);
		}
		else
		{
			static u8 logdata[512];
			memset(logdata, 0x00, sizeof(logdata));
			va_start(arg, mlog);
			vsprintf(logdata, mlog, arg);
			va_end(arg);
			syslog(LOG_INFO, "%s", logdata);
		}
		return R_SUCC;
	}
	return R_FAIL;
}
#endif
s32 write_log(u8 debug_level, c8 *mlog, ...)
{
    static u8 logdata[512];
    va_list arg;
    
    memset(logdata, 0x00, sizeof(logdata));
    va_start(arg, mlog);
    vsprintf(logdata, mlog, arg);
    va_end(arg);

	printf("%s",logdata);
	return R_SUCC;
}


s32 mystrtol(c8 *str, c8 **endpt)
{
	s32 temp = 0;

	while(*str != 0)
	{
		if( (*str >= '0') && (*str <= '9') )
		{
			temp = *str - '0';
			str ++;
			while( (*str >= '0') && (*str <= '9') )
			{
				temp = temp * 10 + *str - '0';
				str ++;
			}
			if(endpt != NULL) *endpt = str;
			return temp;
		}
		str ++;
	}

	return temp;
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

//将10制止的ASCII码转为短整型
//输入参数：
//	  str:  要转换的ACSII码
//	  len:  str的 长度  
//返回： 转换后的整型数
u16 myatoi(u8 *str, u8 len)
{
	u8 flag = 0;
	u8 i;
	u16 ret = 0;

	for(i=0; i<len; i++)
	{
		if( (str[i]>'9') || (str[i]<'0') )
		{
			if(flag != 0) return ret;
			else          continue;
		}
		flag = 1;
		ret = ret * 10;
		ret += str[i]-'0';
	}
	return ret;
}

u8 str_comp(u8 *str1, u8 *str2, u8 len)
{
	u8 i;

	for(i=0; i<len; i++)
	{
		if(str1[i] != str2[i]) return 1;
	}

	return 0;
}

u8 bcd2uchar(u8 bcd)
{
	return((bcd >> 4)*10 + (bcd & 0x0f));
}

u8 uchar2bcd(u8 uchr)
{
	if (uchr >= 100)
		return(0xFF);
	else
		return(((uchr / 10) << 4) | (uchr % 10));
}

u32 bcd2ulng(u8 *bcdbuf, u8 len)
{
	u32 ret;
	u8 i;

	ret = 0;
	for (i = len; i > 0; i--)
	{
		ret = ret * 100;
		ret = ret + bcd2uchar(bcdbuf[i-1]);
	}
	return ret;
}

void serverbuf(u8 *buf, u8 len)
{
	u8 temp, i;

	for(i=0; i<len/2; i++)
	{
		temp = buf[i];
		buf[i] = buf[len-1-i];
		buf[len-1-i] = temp;
	}
}

/**************************************
* 函数名  ： get_time
* 函数描述:  获得当前时分
* 输入参数： 无
* 输出：     无
* 返回值：   HHMM 10进制表示的当前时分
***************************************/
u16 get_time(void)
{
	time_t tt;
	struct tm curtime;
	u16 ret = 0xffff;

	tt = time(NULL);
	localtime_r(&tt, &curtime);

	ret = curtime.tm_hour;
	ret = ret * 100 + curtime.tm_min;

	return ret;
}


void Get_CurTime(u8* o_cpBcdTime, u8 len)
{
	time_t tt;
	struct tm *curtime;

	time(&tt);
	curtime = localtime(&tt);
	o_cpBcdTime[0] = (u8)(curtime->tm_year % 100);
	o_cpBcdTime[1] = (curtime->tm_mon + 1);
	o_cpBcdTime[2] = (curtime->tm_mday);
	o_cpBcdTime[3] = (curtime->tm_hour);
	o_cpBcdTime[4] = (curtime->tm_min);

	if(len > 5) o_cpBcdTime[5] = (curtime->tm_sec);
}


//将HEX码表示的时间转为 time_t型时间
time_t ConvTime(u8 * time)
{
	struct tm curtime;

	curtime.tm_year = 2000 + time[0] - 1900;
	curtime.tm_mon = time[1] - 1;
	curtime.tm_mday = time[2];
	curtime.tm_hour = time[3];
	curtime.tm_min = time[4];
	curtime.tm_sec = time[5];
	return mktime(&curtime);
}

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



//FUNC: create a binary file of given length and original content.
//参数： FileName  要创建的文件名
//       SLen      要创建的文件长度
//       OriginalChar 文件所填原始数据
//返回：成功创建返回0  不成功返回-1
s32 File_CreateFile(c8 *FileName, s32 sLen, u8 OriginalChar)
{
#define Len_PublicBuf1 1024

	FILE *fp;
	u8 Public_Buf1[Len_PublicBuf1];

	fp = fopen(FileName, "wb+");
	if (fp == NULL)
	{
		usleep(10000);
		fp = fopen(FileName, "wb+");
		if (fp == NULL)
		{
			write_log(MSG_SYSERR, "create file %s error\n", FileName);
			return -1;
		}
	}
	if (sLen <= Len_PublicBuf1)  //小于1K的文件一次写入
	{
		memset(Public_Buf1, OriginalChar, (size_t)sLen);
		fwrite(Public_Buf1, (size_t)sLen, 1, fp);
	}
	else             //大于1K的文件分多次写入
	{
		memset(Public_Buf1, OriginalChar, Len_PublicBuf1);
		while (sLen > 0)
		{
			if (sLen > Len_PublicBuf1)
			{
				fwrite(Public_Buf1, 1, Len_PublicBuf1, fp);
			}
			else
			{
				fwrite(Public_Buf1, (size_t)sLen, 1, fp);
			}
			sLen = sLen - Len_PublicBuf1;
		}
	}
	fclose(fp);
	return 0;
}



void clearport(s32 fd)
{
	s32 ret = -1;
	u8 buf[32] = {0x00};

	while (1)
	{
		ret = read(fd, buf, 32);
		if (ret < 32) return;
	}
}

//sltime:  等待超时时间单位10mS
s32 myselect(s32 fd, u16 sltime)
{
	fd_set fs_read;
	struct timeval tv_timeout;

	FD_ZERO(&fs_read);
	FD_SET((u32)fd, &fs_read);
	tv_timeout.tv_sec = sltime / 100;   //setting timeout
	tv_timeout.tv_usec = (sltime % 100) * 10000;
	return select(fd + 1, &fs_read, NULL, NULL, &tv_timeout);
}


//#define  M_NORMAL      000000000000    //普通串口模式， 即两线串口
//#define  M_RS485       001000000000    //RS485模式，即3线串口RTS用作控制输入输出
//#define  M_HARD_HAND   002000000000    //硬件握手模式，既4线串口
//#define  M_MODEM       003000000000    //全modem控制模式, 既9线串口
//#define  M_ISOT0       004000000000
//#define  M_ISOT1       005000000000
//#define  M_IRDA        006000000000    //高速IRDA通信模式
//打开并按portset设置串口参数
//参数：  *dev   设备路径和名称 如 "/dev/ttyS1"
//        baud   波特率 如 9600, 1200等
//        protset  串口属性 格式为 B/P/S 即 数据位/校验方式/停止位   如 8/N/1 等
//        mode   串口工作模式  其定义见<asm/termbits.h>
//返回：  成功：返回打开串口后的fd
//        失败：返回-1
s32 set_serial(c8 *dev, s32 baud, c8 *portset, u32 mode)
{
	s32 fd;
	s32 mybaud;
	c8 str[16];//这里加入不用到的16字节， 是为了避免tcgetattr(fd, &options)时， 对struct termios结构赋值超出范围
	struct termios options;

	write_log(MSG_INFO, "portset:  %s, %d, %s, %x\n", dev, baud, portset, mode >> 27);
	memset(str, 0x30, 16);
	if ((fd = open(dev, O_RDWR, 0)) < 0)
	{
		write_log(MSG_SYSERR, "Open %s error!\n", dev);
		return -1;
	}
	if (tcgetattr(fd, &options) != 0)
	{
		write_log(MSG_SYSERR, "set serial port tcgetattr ERR");
		close(fd);
		return (-1);
	}
	options.c_iflag = 0;

	//波特率
	switch(baud)
	{
		case 150:     mybaud = B150;        break;
		case 300:     mybaud = B300;        break;
		case 600:     mybaud = B600;        break;
		case 1200:    mybaud = B1200;       break;
		case 2400:    mybaud = B2400;       break;
		case 4800:    mybaud = B4800;       break;
		case 9600:    mybaud = B9600;       break;
		case 19200:   mybaud = B19200;      break;
		case 38400:   mybaud = B38400;      break;
		case 57600:   mybaud = B57600;      break;
		case 115200:  mybaud = B115200;     break;
		case 230400:  mybaud = B230400;     break;
		default: mybaud = B1200;            break;
	}
	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&options, (speed_t)mybaud);
	cfsetospeed(&options, (speed_t)mybaud);
	tcflush(fd, TCIOFLUSH);
	//设置数据位
	options.c_cflag &= ~CSIZE;
	switch (portset[0])
	{
		case '5':
			options.c_cflag |= CS5;
			break;
		case '6':
			options.c_cflag |= CS6;
			break;
		case '7':
			options.c_cflag |= CS7;
			break;
		default:
			options.c_cflag |= CS8;
			break;
	}
	//设置校验位
	switch (portset[2])
	{
		case '1':
		case 'e':
		case 'E':   //偶校验
			options.c_cflag |= PARENB;      // Enable parity
			options.c_cflag &= ~(PARODD | CMSPAR);   // ......
			options.c_iflag |= INPCK;       // Enable input parity checking
			break;
		case '2':
		case 'o':
		case 'O':   //奇校验
			options.c_cflag |= (PARODD | PARENB);   // ......
			options.c_cflag &= ~CMSPAR;
			options.c_iflag |= INPCK;       // Enable input parity checking
			break;
		case 'S':
		case 's':   /*space parity*/
			options.c_cflag |= (PARENB | CMSPAR);
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       // Enable input parity checking
			break;
		case 'm':
		case 'M':   /*MASK parity*/
			options.c_cflag |= (PARENB | CMSPAR | PARODD);
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       // Enable input parity checking
			break;
		default:    //没校验
			options.c_cflag &= ~PARENB;     // Clear parity enable
			options.c_iflag &= ~INPCK;      // Disnable input parity checking
			break;
	}
	//设置停止位
	switch (portset[4])    //这里不能控制1.5位停止位， 驱动也不能， 这是由于termios结构定义没有定义1.5位
	{
		case '2':   // 2位停止位
			options.c_cflag |= CSTOPB;
			break;
		default:    //默认是1位
			options.c_cflag &= ~CSTOPB;
			break;
	}
	//设置串口工作模式
	//options.c_cflag &= ~UART_MODE;
	//options.c_cflag |= (mode & UART_MODE);

	options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL); //这个地方是用来设置控制字符和超时参数的，一般默认即可。稍微要注意的是c_cc数组的VSTART 和 VSTOP 元素被设定成DC1 和 DC3，代表ASCII 标准的XON和XOFF字符。
	//所以如果在传输这两个字符的时候就传不过去，这时需要把软件流控制屏蔽
	options.c_cc[VTIME] = 1;
	options.c_cc[VMIN] = 0;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(fd, TCSANOW, &options) != 0)
	{
		write_log(MSG_SYSERR, "Setup Serial Port tcsetattr ERR\n");
		close(fd);
		return -1;
	}
	return fd;
}


//返回接收到的字节数
s32 ttyread(s32 fd, u8 *buf, u32 len, u16 waittime)
{
	s32 ret;
	s32 readlen = 0;

	while (1)
	{
		usleep(1000);    //为防止频繁连续读取串口驱动数据。要连续读取，可以将len加大
		if(readlen == 0)
		{
			ret = myselect(fd, 0);
		}
		else
		{
			ret = myselect(fd, waittime);
		}
		if (ret <= 0) return readlen;
		ret = read(fd, buf + readlen, len - readlen);
		if (ret >= (s32)(len - readlen)) return (s32)len;
		if (ret <= 0) return readlen;
		readlen += ret;
	}
}


/*
 * 函数名  : init_param
 * 函数描述: 初始化参数，放到公共变量上
 * 输入参数:
 * 输出    :
 * 返回值  :  0
 */
s32 init_param(struct parameter_set *st)
{
	FILE *fp;
	c8 buf[128];
	c8 *pt;
	struct in_addr tmpaddr;
    char *info_str = NULL;
    char * pFileName = PARASET_FILE;
    int idx;
    char value;
    char info_str_buf[16];

	fp = fopen(PARASET_FILE, "rb");
	if(fp == NULL)
	{
		write_log(MSG_INFO, "get PARA error %s\n",  PARASET_FILE);
		return -1;
	}
	while(fgets(buf, sizeof(buf), fp))
	{
		write_log(MSG_INFO, "get str:  %s\n", buf);

		if(memcmp(buf, "IPaddr", 6) == 0)
		{
			u32 temp;

			pt = buf;
			temp = mystrtol(buf, &pt);

			if(temp == 0) //自动获取IP
			{
				st->IPaddr = 0;
			}
			else
			{
				temp = temp*0x100 + mystrtol(pt, &pt);
				temp = temp*0x100 + mystrtol(pt, &pt);
				temp = temp*0x100 + mystrtol(pt, &pt);
				st->IPaddr = temp;

				tmpaddr.s_addr = htonl(st->IPaddr);
				write_log(MSG_INFO, "IPaddr: %s  %08X\n", inet_ntoa(tmpaddr), st->IPaddr);
			}
		}

		else if(memcmp(buf, "IPmask", 6) == 0)
		{
			u32 temp;

			pt = buf;
			temp = mystrtol(buf, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			st->IPmask = temp;

			tmpaddr.s_addr = htonl(st->IPmask);
			write_log(MSG_INFO, "IPmask: %s  %08X\n", inet_ntoa(tmpaddr), st->IPmask);
		}

		else if(memcmp(buf, "TCPmode", 7) == 0)
		{
			buf[30] = 0;  //这句话最长不会大于30字节,因为后面注释中可能会出现server等字眼

			if(strstr(buf, "server") != 0)
				st->TCPmode = 1;
			else if(strstr(buf, "client") != 0)
				st->TCPmode = 2;
			else
				st->TCPmode = 0;
		}

		else if(memcmp(buf, "ServerIP", 8) == 0)
		{
			u32 temp;

			pt = buf;
			temp = mystrtol(buf, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			st->SRVRaddr = temp;

			tmpaddr.s_addr = htonl(st->SRVRaddr);
			write_log(MSG_INFO, "SRVRaddr: %s  %08X\n", inet_ntoa(tmpaddr), st->SRVRaddr);
		}

		else if(memcmp(buf, "SVR_PORT", 8) == 0)
		{
			st->SRVRport = mystrtol(buf, NULL);
		}

		else if(memcmp(buf, "UDP_IP", 6) == 0)
		{
			u32 temp;

			pt = buf;
			temp = mystrtol(buf, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			temp = temp*0x100 + mystrtol(pt, &pt);
			st->UDPBaddr = temp;

			tmpaddr.s_addr = htonl(st->UDPBaddr);
			write_log(MSG_INFO, "UDP_IP: %s  %08X\n", inet_ntoa(tmpaddr), st->UDPBaddr);
		}

		else if(memcmp(buf, "UDP_PORT", 8) == 0)
		{
			st->UDPport = mystrtol(buf, NULL);
		}

		else if(memcmp(buf, "COMMode", 7) == 0)
		{
			buf[20] = 0;  //这句话最长不会大于20字节,因为后面注释中可能会出现2G等字眼

			if(strstr(buf, "2G") != 0)
				st->COMMode = 0;
			else if(strstr(buf, "3G") != 0)
				st->COMMode = 1;
			else
				st->COMMode = 2;
		}

		else if(memcmp(buf, "APN = ", 6) == 0)
		{
			u8 temp = 0;
			u8 i, j;

			j=0;
			for(i=4; i<24; i++)
			{
				if(   ((buf[i] >= '0') && (buf[i] <= '9'))
					||((buf[i] >= 'A') && (buf[i] <= 'Z'))
					||((buf[i] >= 'a') && (buf[i] <= 'z'))
					||((buf[i] >= 33) && (buf[i] <= 46))    // "!"#$%&,()*+'-.
					||((buf[i] >= 58) && (buf[i] <= 64) && (buf[i] != '=') )    // :;<>?@					
					||(buf[i] == '_')  )
				{
					temp = 1;
					st->APN[j++] = buf[i];
				}
				else
				{
					if((temp == 1) || ((buf[i] == '/')&&(buf[i+1] == '/')) )
					{
						st->APN[j] = 0;
						break;
					}
				}
			}
			write_log(MSG_INFO, "apn = %s\n", st->APN);
		}


		else if(memcmp(buf, "APNName", 7) == 0)
		{
			u8 temp = 0;
			u8 i, j;

			j=0;
			for(i=8; i<28; i++)
			{
				if(   ((buf[i] >= '0') && (buf[i] <= '9'))
					||((buf[i] >= 'A') && (buf[i] <= 'Z'))
					||((buf[i] >= 'a') && (buf[i] <= 'z'))
					||(buf[i] == '_')  )
				{
					temp = 1;
					st->APNName[j++] = buf[i];
				}
				else
				{
					if((temp == 1) || (buf[i] == '/'))
					{
						st->APNName[j] = 0;
						break;
					}
				}
			}
			write_log(MSG_INFO, "apname = %s\n", st->APNName);
		}


		else if(memcmp(buf, "APNPWD", 6) == 0)
		{
			u8 temp = 0;
			u8 i, j;

			j=0;
			for(i=7; i<27; i++)
			{
				if(   ((buf[i] >= '0') && (buf[i] <= '9'))
					||((buf[i] >= 'A') && (buf[i] <= 'Z'))
					||((buf[i] >= 'a') && (buf[i] <= 'z'))
					||(buf[i] == '_')  )
				{
					temp = 1;
					st->APNPWD[j++] = buf[i];
				}
				else
				{
					if((temp == 1) || (buf[i] == '/'))
					{
						st->APNPWD[j] = 0;
						break;
					}
				}
			}
			 write_log(MSG_INFO, "APNPWD = %s\n", st->APNPWD);
		}

		else if(memcmp(buf, "Intervaltime", 12) == 0)
		{
			st->Intervaltime = mystrtol(buf, NULL);
		}

		else if(memcmp(buf, "DevID", 5) == 0)
		{
			buf[20] = 0;
			strtohex(st->Devid, buf+8);
		}
		
		else if(memcmp(buf, "PlcID", 5) == 0)
		{
			buf[20] = 0;
			strtohex(st->PLCid, buf+8);
		}
		
		else if(memcmp(buf, "ICCARD", 6) == 0)
		{
			buf[20] = 0;
			st->ICCARD = mystrtol(buf+8, NULL);
		}
		
		else if(memcmp(buf, "PWDMODE", 7) == 0)
		{
			buf[20] = 0;
			strtohex(&st->PWDMODE, buf+10);
		}
		
		else if(memcmp(buf, "PWDBYTE", 7) == 0)
		{
			buf[24] = 0;
			strtohex(st->PWDBYTE, buf+10);
		}
		
		else if(memcmp(buf, "CARDFUNC", 8) == 0)
		{
			buf[20] = 0;
			strtohex(&st->CARDFUNC, buf+11);
		}
		
		else if(memcmp(buf, "HOTELID", 7) == 0)
		{
			buf[24] = 0;
			strtohex(st->HOTELID, buf+10);
		}
		
		else if(memcmp(buf, "ParaVar", 7) == 0)
		{
			st->Paravar = mystrtol(buf, NULL);
		}

		else if(memcmp(buf, "HardVar", 7) == 0)
		{
			memcpy(st->hardvar, buf+10, 8);
		}

		else if(memcmp(buf, "HardName", 8) == 0)
		{
			memcpy(st->hardname, buf+11, 36);
		}

		else if(memcmp(buf, "SoftVar", 7) == 0)
		{
			memcpy(st->softvar, buf+10, 8);
		}

		else if(memcmp(buf, "SoftName", 8) == 0)
		{
			memcpy(st->softname, buf+11, 36);
		}

		else if(memcmp(buf, "SetTime", 7) == 0)
		{
			memcpy(st->settime, buf+10, 14);
		}
	}

	if(fp != NULL) fclose(fp);

    printf("reading cmd string from config file %s.\n",pFileName);

    info_str = GetIniSectionItem(PARASET_FILE,"Block_to_read");
        printf("info_str %s\n",info_str);
        st->block_to_read = atoi(info_str);
        printf("Block_to_read %d\n",atoi(info_str));
    info_str = GetIniSectionItem(PARASET_FILE,"Ac_interval");
        printf("info_str %s\n",info_str);
        
        st->ac_interval = atoi(info_str);
        printf("ac_interval %d\n",atoi(info_str));
    info_str = GetIniSectionItem(pFileName,"Cip_mode");
    printf("info_str %s\n",info_str);
    
    st->cip_mode = atoi(info_str);
    printf("cip_mode %d\n",atoi(info_str));
    
    info_str = GetIniSectionItem(pFileName,"Cipherbuf");
    printf("info_str %s\n",info_str);

    char *tmp = "0x";
    for(idx = 0; idx < 6;idx++)
    {
    
        memcpy(info_str_buf, info_str + idx*2, 2);
        info_str_buf[3] = 0;
        
        strtohex( &value ,&info_str_buf[0]);
                 
        st->cipherbuf[idx] = value;
        printf("cipherbuf  %d\n",value);
    }
    

    info_str = GetIniSectionItem(pFileName,"Running_mode");
    printf("info_str %s\n",info_str);
    
    st->running_mode = pt;
    printf("running_mode  %d\n",atoi(info_str));

    info_str = GetIniSectionItem(pFileName,"Hotel_id");
    printf("info_str %s\n",info_str);

    
    memcpy(st->hotel_id, info_str, 5);
    
    //printf("hotel_id  %d\n",atoi(info_str));
    info_str = GetIniSectionItem(pFileName,"Roomcode");
    printf("info_str %s\n",info_str);
    
    memcpy(st->roomcode, info_str, 3);
    
    info_str = GetIniSectionItem(pFileName,"Room_storage_num");
    printf("info_str %s\n",info_str);
    
    st->room_storage_num = atoi(info_str);
    printf("room_storage_num  %d\n",atoi(info_str));
    info_str = GetIniSectionItem(pFileName,"Reset_relay_flag");
    printf("info_str %s\n",info_str);
    
    st->reset_relay_flag = atoi(info_str);
	return 0;
}


void disp_paraset(struct parameter_set *st)
{
	write_log(MSG_INFO, "IPaddr: %08X\n", st->IPaddr);
	write_log(MSG_INFO, "IPmask: %08X\n", st->IPmask);
	write_log(MSG_INFO, "TCPmode: %d\n", st->TCPmode);
	write_log(MSG_INFO, "SRVRaddr: %08X\n", st->SRVRaddr);
	write_log(MSG_INFO, "SRVRport: %d\n", st->SRVRport);
	write_log(MSG_INFO, "UDPBaddr: %08X\n", st->UDPBaddr);
	write_log(MSG_INFO, "UDPport: %d\n", st->UDPport);

	write_log(MSG_INFO, "COMMode: %d\n", st->COMMode);              //与主站的通信模式：0:2G  1:3G  2:ETH
	write_log(MSG_INFO, "APN:     %s\n", st->APN);                  //2/3G拨号时，APN网络名
	write_log(MSG_INFO, "APNName: %s\n", st->APNName);              //拨号时用户名
	write_log(MSG_INFO, "APNPWD:  %s\n", st->APNPWD);               //拨号是密码

	write_log(MSG_INFO, "Intervaltime: %d\n", st->Intervaltime);
	write_log(MSG_INFO, "Devid: %02X %02X %02X %02X %02X %02X\n", st->Devid[0], st->Devid[1], st->Devid[2], st->Devid[3], st->Devid[4], st->Devid[5]);
	write_log(MSG_INFO, "PLCid: %02X %02X %02X %02X %02X %02X\n", st->PLCid[0], st->PLCid[1], st->PLCid[2], st->PLCid[3], st->PLCid[4], st->PLCid[5]);
	
	write_log(MSG_INFO, "ICCARD: %d\n", st->ICCARD);
	write_log(MSG_INFO, "PWDMODE: 0x%02X\n", st->PWDMODE);
	write_log(MSG_INFO, "PWDBYTE: %02X %02X %02X %02X %02X %02X\n", st->PWDBYTE[0], st->PWDBYTE[1], st->PWDBYTE[2], st->PWDBYTE[3], st->PWDBYTE[4], st->PWDBYTE[5]);
	write_log(MSG_INFO, "CARDFUNC: 0x%02X\n", st->CARDFUNC);
	write_log(MSG_INFO, "HOTELID: %02X %02X %02X %02X %02X\n", st->HOTELID[0], st->HOTELID[1], st->HOTELID[2], st->HOTELID[3], st->HOTELID[4]);
        
	write_log(MSG_INFO, "BlockToRead: %d\n", st->block_to_read);
	write_log(MSG_INFO, "AcInterval: %d\n", st->ac_interval);    
	write_log(MSG_INFO, "CipherBuf: %02X%02X%02X%02X%02X%02X\n", st->cipherbuf[0], st->cipherbuf[1], st->cipherbuf[2], st->cipherbuf[3], st->cipherbuf[4], st->cipherbuf[5]);
	write_log(MSG_INFO, "CipMode: %d\n", st->cip_mode);
	write_log(MSG_INFO, "RunningMode: %d\n", st->running_mode);
	write_log(MSG_INFO, "HotelId: %c%c%c%c%c\n", st->hotel_id[0], st->hotel_id[1], st->hotel_id[2], st->hotel_id[3], st->hotel_id[4]);
	write_log(MSG_INFO, "RoomCode: %c%c%c\n", st->roomcode[0], st->roomcode[1], st->roomcode[2]);
	write_log(MSG_INFO, "RoomStorageNum: %d\n", st->room_storage_num);    
	write_log(MSG_INFO, "ResetRelayFlag: %d\n", st->reset_relay_flag);
}


u8 calc_bcc(u8 *buf, int len)
{
	u8 bcc = 0;

	while(len)
	{
		bcc =  bcc + *buf;
		buf++;
		len --;
	}
	return bcc;
}



int check_bcc(u8 *buf, int len)
{
	u8 bcc = 0;

	len --;
	while(len)
	{
		bcc =  bcc + *buf;
		buf++;
		len --;
	}

	if(bcc != *buf)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void gave_soft_dog(c8 *module)
{
	FILE *fp;
	static time_t tt = 0;

	if( abs(time(0) - tt) < 30)  return;

	tt = time(0);
	fp = fopen("/var/log/running", "rb+");
	if(fp == NULL)
	{
		fp = fopen("/var/log/running", "wb+");
		if(fp == NULL)
		{
			write_log(MSG_INFO, "create running file error\n");
			return;
		}

		u8 buf[48];
		bzero(buf, sizeof(buf));
		fwrite(buf, 1, 48, fp);
	}

	if(memcmp(module, "main", 4) == 0)
	{
		fseek(fp, 0, SEEK_SET);
	}
	if(memcmp(module, "comm", 4) == 0)
	{
		fseek(fp, 24, SEEK_SET);
	}

	fprintf(fp, "%s %ld\n", module, time(0));
	fclose(fp);
}

void write_version(u8 prs, c8 *srt)
{
	FILE *fp;
	
	if(prs >= 2) return;
	
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
		fseek(fp, prs*3, SEEK_SET);
		fwrite(srt, 1, 3, fp);
		fclose(fp);
	}	
}

