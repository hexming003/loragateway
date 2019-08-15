#ifndef __PUBLIC_H
#define __PUBLIC_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <utime.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <asm/termbits.h>
//#include <asm/arch/at91sam9260.h>
//#include <at91sam9260.h>
//#include <at91_pio.h>

//#include <asm/arch/at91_pio.h>
#include <regex.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef    signed char            s8;
typedef    char                   c8;
typedef    unsigned char          u8;
typedef    signed short           s16;
typedef    unsigned short         u16;
typedef    signed int             s32;
typedef    unsigned int           u32;
typedef    signed long long       s64;
typedef    unsigned long long     u64;
typedef    float                  f32;
typedef    double                 df64;

//0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
//0 1 2 3 4 5 6 7 8 9 A  B  C  D  E  F  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W
#define COMM_VERVION "FCA"
#define MAIN_VERSION "FC3"

/*  MAIN
15-12-31  开启自动轮询功能，每天可以轮询100个节点。 之前的因程序bug，不能轮询已经通信成功过的节点。
          节点配置更新后，有新节点加入， 可以马上进入轮询。  之前最大要等2天。


*/

#define MAXNODENUM  512
#define NodeDataPath        "/MeterRoot/DataFiles/"
#define PARASET_FILE        "/MeterRoot/CFGFiles/paraset.txt"
#define CONFIG_IPSH         "/MeterRoot/config_ip.sh"
#define NODECFG_FILE        "/MeterRoot/CFGFiles/node.cfg"   //???写时要加锁
#define LogPath             "/MeterRoot/LogFiles/"
#define LogPathtmp          "/MeterRoot/LogFiles/tmpf"

#define NodeDataTpath       "/tmp/DataFiles/"
#define LogTpath            "/tmp/LogFiles/"
#define NODEDATA_FILE       "/tmp/DataFiles/node.dat"
#define NODE_tmplist        "/tmp/nodelist"

#define PATH_M2C_PIPE      "/tmp/pipeM2C"
#define PATH_C2M_PIPE      "/tmp/pipeC2M"


#define PIN_WDOG  "PB30"        //高电平喂狗；    但高电平超过1.6秒看门狗会复位
#define MPWR      "PA11"        //通信模块上电   高电平时通信模块上电
#define MRST      "PC25"        //PC25    MRST      通信模块复位   低电平复位
#define MON_OFF   "PC29"        //PC29    MON_OFF   通信模块点火   发送开机信号，PWR_ON信号由低电平置高电平，持续超过一秒，然后转回低电平
#define MPWR_DET  "PC11"        //高电平表示有市电供电
#define MBAT_CTRL "PC12"        //高电平控制允许电池供电
#define RUN_LED   "PA6"         //运行状态灯
#define PLCM_RST  "PC20"        //载波模块复位控制
#define INET_RST  "PA28"        //以太网卡复位控制 低电平复位


#define MAXPLCFLEN       120000    //300000 PLC记录文件最大长度    120K * 250个节点   = 30M
#define MAXSVRFLEN       1024000   //2048*1024 上行通信报文记录    1M 压缩后越120K×10 = 3M + 1M meseg
#define WDINTERVAL       100

#define EXG(x) (x=((x<<8)+(x>>8)))
#define EXGDW(x) (x=((x<<24)+((x&0xff00)<<8)+((x&0xff0000)>>8)+(x>>24)))

#define UPTO4(x) ((x)+3-(((x)+3)%4))   //向上进位到4的倍数
#define UPTO2(x) ((x)+((x)%2))         //向上进位到2的倍数

#define CONNECTED           0xaa
#define DISCONNECT          0
#define TRUE                1
#define FALSE               0
#define R_SUCC              0
#define R_FAIL              -1
#define NO_MSG              0
#define R_UNRIGHT           1

//nDebugLevel打印层次定义
#define MSG_NOTHING  0          //nDebugLevel == 0  任何信息都不打印
#define MSG_SYSERR   1          //            == 1  打印系统出错信息
#define MSG_INFO     2          //            == 2  打印重要调试信息
#define MSG_DEBUG    3          //            == 3  打印次要调试信息
#define MSG_COMM     4          //            == 4  打印调试信息和通讯信息
#define MSG_RUNING   5          //            == 5  运行信息   任何信息都打印

#define MSG_COMMR    0x10
#define MSG_COMMT    0x20

#define DISPLAY_LOG_SCREEN      1
#define WRITE_LOG               0

#pragma pack(1)
struct parameter_set
{
	u32 IPaddr;      //本机IP地址，当为0时，表示自动获得IP
	u32 IPmask;      //子网掩码
	u8  TCPmode;     //tcp 主从模式; 1:server,  2:client, 0:none,
	u32 SRVRaddr;    //主机(后台服务器)地址；
	u16 SRVRport;    //主机端口号(发送指向的端口号)
	u32 UDPBaddr;    //UDP广播发送地址
	u16 UDPport;     //UDP侦听端口号

	u8 COMMode;      //与主站的通信模式：0:2G  1:3G  2:ETH
	u8 APN[16];           //2/3G拨号时，APN网络名
	u8 APNName[16];      //拨号时用户名
	u8 APNPWD[16];       //拨号是密码

	u16 Intervaltime;    //采集间隔时间
	u8 Devid[6];         //设备唯一ID 固定6字节 也作为设备的地址
	u8 PLCid[6];         //载波通信设备ID
	
	u8 ICCARD;                  //IC卡片扇区设置，指定上报的扇区0～63
	u8 PWDMODE;                  //密钥启用模式（1字节 00：指定密钥生效； 01：卡号就是密码；0xff：密钥无效）
	u8 PWDBYTE[6];            //扇区的密钥（6字节）
	u8 CARDFUNC;                //取电开关功能启用模式（1字节）
	u8 HOTELID[5];             //酒店ID号（5字节）
	
	u16 Paravar;         //当前参数版本号
	u8 hardvar[8];       //硬件版本8个字节
	u8 hardname[36];     //硬件设计者名字36字节
	u8 softvar[8];       //软件版本
	u8 softname[36];     //软件设计者名字36字节
	u8 settime[14];      //配置的时间
	uint8_t block_to_read;  // 读取卡数据第n块 默认61
	uint32_t ac_interval;   //采集间隔  默认120

	uint8_t cip_mode;       //加密方式 默认0
	uint8_t cipherbuf[6];   // 卡密码 默认全FF
	uint8_t running_mode;   //运行模式 默认0
	uint8_t hotel_id[5];    //酒店代码 默认：HPWIN
	uint8_t roomcode[3];    //默认 0，0，0
	uint8_t room_storage_num; //满多少个数据上报 默认：30
	uint8_t reset_relay_flag;  //继电器定时复位标志
}__attribute__((packed));
#pragma pack(1)


struct nodelist_st   //142字节
{
	u8 naddr[6];
	u8 pollt;     	  //轮询次数，当节点有数据返回时，轮询次数为0xaa, 表示轮询成功过
	u8 roomid[3]; 	  //房间ID
	u8 version[4];    //版本号+编译时间（年月日）
	u16 lightver;     //亮度配置版本号
	u8 lightcfg[80];  //指向亮度配置表
} __attribute__((packed));


s32 gpio_init(void);
int tc_init(u32 tc_hz);
void tcA_on(u8 occ);
void tcB_on(u8 occ);
void tcA_off(void);
void tcB_off(void);
void tcB_val(u8 occ);
void tc_off(void);
void gpio_disable_upregister(c8 *port_pin);
void gpio_enable_upregister(c8 *port_pin);
void gpio_enable_read(c8 *port_pin);
void gpio_enable_write(c8 *port_pin);
u8 gpio_read_pin(c8 *port_pin);
void gpio_write_low(c8 *port_pin);
void gpio_write_high(c8 *port_pin);
void timeout_info(int signo);
void init_sigaction(int dtime);
s32 set_mcb_multcate(void);

void display(c8 *mlog, u8 *Databuf, u8 len);
s32 read_from_file(c8 *filepath, s32 offset, s32 whence, u8 *pbuf, s32 nlen);
s32 mystrtol(c8 *str, c8 **endpt);
s32 system_wait(c8 *string);
s32 write_log(u8 debug_level, c8 *log, ...);
s32 write_comm(u8 debug_level, c8 *log, u8 *pBuf, s32 nLen);
u8 strtohex(u8 *outbuf, c8 *instr);
u8 mychartohex(u8 *cstr, c8 **pt);
u32 bcd2ulng(u8 *bcdbuf, u8 len);
u8 bcd2uchar(u8 bcd);
void serverbuf(u8 *buf, u8 len);
u16 get_time(void);
int is_all_xx(u8 *buf, u8 val, int len);
s32 File_CreateFile(c8 *FileName, s32 sLen, u8 OriginalChar);
void Get_CurBCDTime1(u8* o_cpBcdTime);
time_t ConvTime(u8 * BCDTime);
u8 calc_bcc(u8 *buf, int len);
int check_bcc(u8 *buf, int len);
u8 uchar2bcd(u8 uchr);
void gave_soft_dog(c8 *module);
void write_version(u8 prs, c8 *srt);
u8 str_comp(u8 *str1, u8 *str2, u8 len);
u16 myatoi(u8 *str, u8 len);

s32 myselect(s32 fd, u16 time);
s32 ttyread(s32 fd, u8 *buf, u32 len, u16 waittime);
s32 set_serial(c8 *dev, s32 baud, c8 *portset, u32 mode);
void clearport(s32 fd);
void Get_CurTime(u8* o_cpBcdTime, u8 len);
s32 init_param(struct parameter_set *st);
void disp_paraset(struct parameter_set *st);


int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
#endif
