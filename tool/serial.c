#include <pthread.h>
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
#include <pthread.h>
#include <unistd.h>

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);

typedef    signed char            c8;
typedef    signed char            s8;
typedef    unsigned char          u8;
typedef    signed short           s16;
typedef    unsigned short         u16;
typedef    signed int             s32;
typedef    unsigned int           u32;
typedef    signed long long       s64;
typedef    unsigned long long     u64;

s32 set_serial(c8 *dev, s32 baud, c8 *portset, u32 mode);
void *read_serial(void *arg);

u8  PortNo = 4;
u32 Bord = 115200;
c8  Sset[10] = "8/N/1";
u32 Sflag = M_NORMAL;

int Gprs_fd;

int main(int argc, char *argv[])
{
	char str1[256];
	int n;
	int g;
	pthread_t ntid;
	int err;

	
    //printf("请输入您要打开的端口号 ttyS? \n");
    //scanf("%d",&i);
    //meter_no = (u8)i;
    
   Gprs_fd = set_serial("/dev/ttyS4", 115200L, "8/N/1", M_NORMAL);
   
   err = pthread_create(&ntid, NULL, read_serial, NULL);
   
   while(1)
   {
   		//if( (g = getc(stdin)) != EOF)  {c = g;	write(Gprs_fd, c, 1);  printf("w:%c\n", c);}
   		
   		gets(str1); 
   		n = strlen(str1);
   		str1[n++] = '\r';
   		str1[n++] = '\n';
   		str1[n] = 0;
   		write(Gprs_fd, str1, n);
	}
}


void *read_serial(void *arg)
{
	u8 buf[8];
	
	printf("read_serial ...\n");
	while(1)
	{
		while(read(Gprs_fd, buf, 1) == 1)   printf("%c", buf[0]);
		usleep(100000);
	}
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

	printf("portset:  %s, %d, %s, %x\n", dev, baud, portset, mode >> 27);
	memset(str, 0x30, 16);
	if ((fd = open(dev, O_RDWR, 0)) < 0)
	{
		printf("Open %s error!\n", dev);
		return -1;
	}
	if (tcgetattr(fd, &options) != 0)
	{
		printf("set serial port tcgetattr ERR");
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
		printf("Setup Serial Port tcsetattr ERR\n");
		close(fd);
		return -1;
	}
	return fd;
}




