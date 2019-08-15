#ifndef __LEWIN_H
#define __LEWIN_H

#include "../public/public.h"
#include "../public/namepipe.h"

struct srvcmd_st
{
	u8 head;        //֡ͷ  0xff
	u8 cmd;         //������
	u32 time;       //ʱ���
	u8 addr[6];     //��ַ��
	u8 data[5];     //������
	u8 cs;          //У����
} __attribute__((packed));

#define MINP3762HEADLEN 13
#define P3762HEADLEN    25
struct protocol3762_st
{
	u8 head;          //֡ͷ    0x68
	u16 len;          //���ȣ�ָ֡���ݵ��ܳ��ȣ���2�ֽ���ɣ�BIN��ʽ�������û����ݳ���L1��6���ֽڵĹ̶����ȣ���ʼ�ַ������ȡ�������У��͡������ַ���
	u8 ctrl;          //����: ��ʾ���ĵĴ��䷽��������־��ͨ��ģ���ͨ�ŷ�ʽ������Ϣ 41:���������У� 81:�ڵ����У� C1���ڵ������ϱ��� 01��������Ӧ��ڵ�
	u8 usrR[6];       //�û�������Ϣ��R
	u8 usrSAddr[6];   //�û�����Դ��ַ��A
	u8 usrDAddr[6];   //�û�����Ŀ�ĵ�ַ��A
	u8 usrAFN;        //�û�����Ӧ�ù�����AFN  02H  ����ת��
	u8 usrDT[2];      //���ݵ�Ԫ��ʶ ��Fn
	u8 usrData[200];  //�û�����Ӧ������
	u8 cs;            //У����
	u8 end;           //֡β 0x16
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
