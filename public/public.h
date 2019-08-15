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
15-12-31  �����Զ���ѯ���ܣ�ÿ�������ѯ100���ڵ㡣 ֮ǰ�������bug��������ѯ�Ѿ�ͨ�ųɹ����Ľڵ㡣
          �ڵ����ø��º����½ڵ���룬 �������Ͻ�����ѯ��  ֮ǰ���Ҫ��2�졣


*/

#define MAXNODENUM  512
#define NodeDataPath        "/MeterRoot/DataFiles/"
#define PARASET_FILE        "/MeterRoot/CFGFiles/paraset.txt"
#define CONFIG_IPSH         "/MeterRoot/config_ip.sh"
#define NODECFG_FILE        "/MeterRoot/CFGFiles/node.cfg"   //???дʱҪ����
#define LogPath             "/MeterRoot/LogFiles/"
#define LogPathtmp          "/MeterRoot/LogFiles/tmpf"

#define NodeDataTpath       "/tmp/DataFiles/"
#define LogTpath            "/tmp/LogFiles/"
#define NODEDATA_FILE       "/tmp/DataFiles/node.dat"
#define NODE_tmplist        "/tmp/nodelist"

#define PATH_M2C_PIPE      "/tmp/pipeM2C"
#define PATH_C2M_PIPE      "/tmp/pipeC2M"


#define PIN_WDOG  "PB30"        //�ߵ�ƽι����    ���ߵ�ƽ����1.6�뿴�Ź��Ḵλ
#define MPWR      "PA11"        //ͨ��ģ���ϵ�   �ߵ�ƽʱͨ��ģ���ϵ�
#define MRST      "PC25"        //PC25    MRST      ͨ��ģ�鸴λ   �͵�ƽ��λ
#define MON_OFF   "PC29"        //PC29    MON_OFF   ͨ��ģ����   ���Ϳ����źţ�PWR_ON�ź��ɵ͵�ƽ�øߵ�ƽ����������һ�룬Ȼ��ת�ص͵�ƽ
#define MPWR_DET  "PC11"        //�ߵ�ƽ��ʾ���е繩��
#define MBAT_CTRL "PC12"        //�ߵ�ƽ���������ع���
#define RUN_LED   "PA6"         //����״̬��
#define PLCM_RST  "PC20"        //�ز�ģ�鸴λ����
#define INET_RST  "PA28"        //��̫������λ���� �͵�ƽ��λ


#define MAXPLCFLEN       120000    //300000 PLC��¼�ļ���󳤶�    120K * 250���ڵ�   = 30M
#define MAXSVRFLEN       1024000   //2048*1024 ����ͨ�ű��ļ�¼    1M ѹ����Խ120K��10 = 3M + 1M meseg
#define WDINTERVAL       100

#define EXG(x) (x=((x<<8)+(x>>8)))
#define EXGDW(x) (x=((x<<24)+((x&0xff00)<<8)+((x&0xff0000)>>8)+(x>>24)))

#define UPTO4(x) ((x)+3-(((x)+3)%4))   //���Ͻ�λ��4�ı���
#define UPTO2(x) ((x)+((x)%2))         //���Ͻ�λ��2�ı���

#define CONNECTED           0xaa
#define DISCONNECT          0
#define TRUE                1
#define FALSE               0
#define R_SUCC              0
#define R_FAIL              -1
#define NO_MSG              0
#define R_UNRIGHT           1

//nDebugLevel��ӡ��ζ���
#define MSG_NOTHING  0          //nDebugLevel == 0  �κ���Ϣ������ӡ
#define MSG_SYSERR   1          //            == 1  ��ӡϵͳ������Ϣ
#define MSG_INFO     2          //            == 2  ��ӡ��Ҫ������Ϣ
#define MSG_DEBUG    3          //            == 3  ��ӡ��Ҫ������Ϣ
#define MSG_COMM     4          //            == 4  ��ӡ������Ϣ��ͨѶ��Ϣ
#define MSG_RUNING   5          //            == 5  ������Ϣ   �κ���Ϣ����ӡ

#define MSG_COMMR    0x10
#define MSG_COMMT    0x20

#define DISPLAY_LOG_SCREEN      1
#define WRITE_LOG               0

#pragma pack(1)
struct parameter_set
{
	u32 IPaddr;      //����IP��ַ����Ϊ0ʱ����ʾ�Զ����IP
	u32 IPmask;      //��������
	u8  TCPmode;     //tcp ����ģʽ; 1:server,  2:client, 0:none,
	u32 SRVRaddr;    //����(��̨������)��ַ��
	u16 SRVRport;    //�����˿ں�(����ָ��Ķ˿ں�)
	u32 UDPBaddr;    //UDP�㲥���͵�ַ
	u16 UDPport;     //UDP�����˿ں�

	u8 COMMode;      //����վ��ͨ��ģʽ��0:2G  1:3G  2:ETH
	u8 APN[16];           //2/3G����ʱ��APN������
	u8 APNName[16];      //����ʱ�û���
	u8 APNPWD[16];       //����������

	u16 Intervaltime;    //�ɼ����ʱ��
	u8 Devid[6];         //�豸ΨһID �̶�6�ֽ� Ҳ��Ϊ�豸�ĵ�ַ
	u8 PLCid[6];         //�ز�ͨ���豸ID
	
	u8 ICCARD;                  //IC��Ƭ�������ã�ָ���ϱ�������0��63
	u8 PWDMODE;                  //��Կ����ģʽ��1�ֽ� 00��ָ����Կ��Ч�� 01�����ž������룻0xff����Կ��Ч��
	u8 PWDBYTE[6];            //��������Կ��6�ֽڣ�
	u8 CARDFUNC;                //ȡ�翪�ع�������ģʽ��1�ֽڣ�
	u8 HOTELID[5];             //�Ƶ�ID�ţ�5�ֽڣ�
	
	u16 Paravar;         //��ǰ�����汾��
	u8 hardvar[8];       //Ӳ���汾8���ֽ�
	u8 hardname[36];     //Ӳ�����������36�ֽ�
	u8 softvar[8];       //����汾
	u8 softname[36];     //������������36�ֽ�
	u8 settime[14];      //���õ�ʱ��
	uint8_t block_to_read;  // ��ȡ�����ݵ�n�� Ĭ��61
	uint32_t ac_interval;   //�ɼ����  Ĭ��120

	uint8_t cip_mode;       //���ܷ�ʽ Ĭ��0
	uint8_t cipherbuf[6];   // ������ Ĭ��ȫFF
	uint8_t running_mode;   //����ģʽ Ĭ��0
	uint8_t hotel_id[5];    //�Ƶ���� Ĭ�ϣ�HPWIN
	uint8_t roomcode[3];    //Ĭ�� 0��0��0
	uint8_t room_storage_num; //�����ٸ������ϱ� Ĭ�ϣ�30
	uint8_t reset_relay_flag;  //�̵�����ʱ��λ��־
}__attribute__((packed));
#pragma pack(1)


struct nodelist_st   //142�ֽ�
{
	u8 naddr[6];
	u8 pollt;     	  //��ѯ���������ڵ������ݷ���ʱ����ѯ����Ϊ0xaa, ��ʾ��ѯ�ɹ���
	u8 roomid[3]; 	  //����ID
	u8 version[4];    //�汾��+����ʱ�䣨�����գ�
	u16 lightver;     //�������ð汾��
	u8 lightcfg[80];  //ָ���������ñ�
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
