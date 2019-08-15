#ifndef __LEWIN_H
#define __LEWIN_H

#include "../public/public.h"
#include "../public/namepipe.h"

struct srvcmd_st
{
	u8 head;        //帧头  0xff
	u8 cmd;         //命令码
	u32 time;       //时间戳
	u8 addr[6];     //地址码
	u8 data[5];     //数据码
	u8 cs;          //校验码
} __attribute__((packed));

#define MINP3762HEADLEN 13
#define P3762HEADLEN    25
struct protocol3762_st
{
	u8 head;          //帧头    0x68
	u16 len;          //长度：指帧数据的总长度，由2字节组成，BIN格式，包括用户数据长度L1和6个字节的固定长度（起始字符、长度、控制域、校验和、结束字符）
	u8 ctrl;          //控制: 表示报文的传输方向、启动标志和通信模块的通信方式类型信息 41:集中器下行； 81:节点上行； C1：节点主动上报； 01；集中器应答节点
	u8 usrR[6];       //用户数据信息域R
	u8 usrSAddr[6];   //用户数据源地址域A
	u8 usrDAddr[6];   //用户数据目的地址域A
	u8 usrAFN;        //用户数据应用功能码AFN  02H  数据转发
	u8 usrDT[2];      //数据单元标识 即Fn
	u8 usrData[200];  //用户数据应用数据
	u8 cs;            //校验码
	u8 end;           //帧尾 0x16
} __attribute__((packed));


void get_node_id(void);
void send_sev_test(void);
void system_init(void);
void test_pro(void);

u8 check_node_msg(u8 *rbuf, u16 len);
u8 get_Fn(u8 *dt);
void pipe_msg_process(void);
void process_A06F02(u8 *rbuf, u16 len);
void process_msg_nodecg(u16 cfgnum);
void process_msg_lightcg(u8 *msg, u8 len);
int save_plc_record(u8 *rbuf, u16 len, u8 *addr);
void ask_A06F02(struct protocol3762_st rmsg);
void main_reboot_done(void);

u8 broad_time(void);
u8 check_plc_frame(struct protocol3762_st* rmsg, u8* buf, u16 len);
u8 get_rush_poling(u8 flag, u16 *ndpt);
void get_serial_msg(void);
void poling_node_data(void);
void send_A00(u8 fn);
void set_plc_node_addr(void);
void set_testplc_node(void);
void send_Poll_msg(u16 curpoll);
void plcwrite(int fd, u8 *sbuf, u8 len);
void update_node_cfgfile(void);
void send_setmode_msg(u8 *addr, u8 mode, u8 cmd);

#endif
