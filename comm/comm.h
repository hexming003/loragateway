#ifndef __DLDC_H
#define __DLDC_H

#include "../public/public.h"

struct SMSGBuffStr
{
	u8 len;
	u8 sbuf[255];
	struct SMSGBuffStr *next;
};

struct SMSGStr
{
	u8 head;
	u8 cmd;
	u8 devid[6];
	u8 timebuf[5];
	u8 ext;
	u8 data[240];
	u8 cs;
	u8 tail;
} __attribute__((packed));
#define SMSGHEADLEN    14

// COMM_flag  状态定义      //当前报文通信标记
#define SDATA_HAD_RET  0
#define SDATA_WAIT_RET 1
#define SDATA_HAD_BEGIN 2

//SGPRS_flsg 状态定义
#define RECONNECT  0xaa   //通信失败超过一定次数，需要重链接2G网络
#define CONNECTING 0xff   //正在进行拨号链接
#define CONNECTOK  0x00   //正常连接到主站

#define SEL_SEC 1
#define SEL_USEC 0
#define TIMEOUT_COUNT 30

#include "../public/public.h"
#include "../public/namepipe.h"

void ask_server_CtorPara(void);
void ask_server_NodePara(u8 *nid);
int check_server_msg(struct SMSGStr *smsgst, int dlen);
void check_set_systime(u8 *tbuf);
int get_node_para_version(u8 *devid);
void Gprs_recv_process(void);
void main_init(void);
int pipe_msg_process(void);
void record_comm_msg(c8 type, u8 *buf, int len);
void send_to_server(u8 *sbuf, int len);
int write_light_para(u8 *nodeid, u8 *dbuf, u8 len);
void show_smsglst(void);
void reissued_smsg(void);
void heart_server_msg(c8 *msg);
void comm_reboot_done(void);
void copy_nocomm_list(void);
void process_server_msg(u8 *buf, int dlen);
void terminal_update(void);
void reask_update_frame(void);
void record_comm_flag(void);

u8 connect_GPRS_net(void);
void GPRS_send_data(u8 *buf, int len);
void GPRS_pwr_off(void);

void init_UDP_socket(void);
void get_udp_msg(void);
void udp_send(u32 addr, u32 port, u8 *buf, int dlen);




#endif
