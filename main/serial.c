#include "lewin.h"
#include <netinet/in.h>
#include "lora.h"

time_t PolingTime = 0;  //�ϴ���ѯ���͵�ʱ��
int Plc_fd;
u32 PollTime;
u8 SID;
u8 PLCSbuf[256] = {0xfe,0xfe,0xfe,0xfe,0xfe};
u8 PLChead = 3;
extern int FDW_pipe;

extern struct nodelist_st NodeList[MAXNODENUM];
extern struct parameter_set ParaSet;



void get_serial_msg(void)
{
	static u8 count = 0;
	static time_t lct = 0;
	static u8 buf[256];

	s32 dlen;
	struct protocol3762_st rmsg;
	int i;

	dlen = ttyread(Plc_fd, buf, 256, 10);
	if(dlen > 2) write_comm(MSG_COMMT+MSG_DEBUG, "PLC Recv:", buf, dlen);

	//��������10�����ղ����ڵ�����ݰ��� ��λ������ģ��
	if(lct == 0) lct = time(NULL);
	if(time(NULL) < lct) lct = time(NULL);
	if(dlen < MINP3762HEADLEN)
	{
		//write_log(MSG_INFO, "lct=%ld  cur=%ld\n",lct, time(NULL));
		if( (abs(time(NULL) - lct) > 600) )
		{
			count ++;
			if(count >= 2)
			{
				write_log(MSG_INFO, "Reset PLC model\n");

				//gpio_write_low(PLCM_RST);
				//sleep(1);
				//gpio_write_high(PLCM_RST);

				count = 0;
			}

			lct = time(NULL);
		}
		return;
	}

	lct = time(NULL);
	count = 0;

	if(check_plc_frame(&rmsg, buf, dlen) == 0)
	{
		memcpy((u8 *)&rmsg, buf, sizeof(rmsg));

		//printf("PLC Recv: AFN=0x%02X  FN = %d\n\n", rmsg.usrAFN, get_Fn(rmsg.usrDT));

		if(rmsg.usrAFN == 0x02) //����ת��Ӧ��
		{
			save_plc_record((u8*)&rmsg, dlen, ParaSet.PLCid);
			
			for(i=0; i<MAXNODENUM; i++)
			{
				//write_comm(MSG_COMMT+MSG_DEBUG, "NodeAdd:", NodeAddrList[i], 6);
				//write_comm(MSG_COMMT+MSG_DEBUG, "GmsgAdd:", &rmsg.usrData[3], 6);

				if(memcmp(NodeList[i].naddr, rmsg.usrSAddr, 6) == 0)  //�ýڵ�ĵ�ַ�Ѿ�����
				{
					u8 sbuf[128];
					
					write_comm(MSG_COMMT+MSG_DEBUG, "rmsg.usrData:", rmsg.usrData, 32);
					
					if((rmsg.usrData[11] == 2) &&(rmsg.usrData[12] == 0xff) )  //���ýڵ�̵���״̬����
					{
						memcpy(sbuf, rmsg.usrSAddr, 6);
						sbuf[6] = rmsg.usrData[12];
						sbuf[7] = rmsg.usrData[13];
						SendPipeMsg(FDW_pipe, PIPE_SETNODE_OK, sbuf, 8);						
					}
					
					if((rmsg.usrData[11] == 1) && (rmsg.usrData[12] == 0xAA) )  //���ýڵ�ң�ؿ�������
					{
						memcpy(sbuf, rmsg.usrSAddr, 6);
						sbuf[6] = rmsg.usrData[12];
						sbuf[7] = 00;
						SendPipeMsg(FDW_pipe, PIPE_SETNODE_OK, sbuf, 8);						
					}
					
					else if( (rmsg.usrData[11] == 5) && (rmsg.usrData[12] == 0xfe) )  //��ȡ�ڵ�������ݷ���
					{
						memcpy(sbuf, rmsg.usrSAddr, 6);
						sbuf[6] = rmsg.usrData[12];
						sbuf[7] = rmsg.usrData[13];
						sbuf[8] = rmsg.usrData[14];
						sbuf[9] = rmsg.usrData[15];
						sbuf[10] = rmsg.usrData[16];
						SendPipeMsg(FDW_pipe, PIPE_SETNODE_OK, sbuf, 11);						
					}
					
					else
					{					
						if(NodeList[i].pollt == 0x11) //���ò����У����ڷ��ر�ʾ���ò����ɹ���
						{
							sprintf(sbuf, "node %02X%02X%02X%02X%02X%02X var update %d", NodeList[i].naddr[0],NodeList[i].naddr[1],NodeList[i].naddr[2],NodeList[i].naddr[3],NodeList[i].naddr[4],NodeList[i].naddr[5], NodeList[i].lightver);
							SendPipeMsg(FDW_pipe, PIPE_PAPAUP_OK, sbuf, strlen(sbuf));
						}
						NodeList[i].pollt = 0xaa;
						memcpy(NodeList[i].version, rmsg.usrData+14, 4);
					}
					
					return;
				}

				if(is_all_xx(NodeList[i].naddr, 0x00, 6) == 0)  //�����б���β
				{
					write_log(MSG_INFO, "�������ݵĽڵ㲻����\n");
					return;
				}				
			}
		}

		if(rmsg.usrAFN == 0x06) //�ڵ������ϱ�
		{
			SID = rmsg.usrR[5];
			//send_A00(1);
			ask_A06F02(rmsg);
			if(get_Fn(rmsg.usrDT) == 2)
			{				
				process_A06F02(rmsg.usrData+4, rmsg.usrData[3]);  //��������������
			}
		}
	}
}



void poling_node_data(void)
{
	static u16 PolingPt = 0;       //��ѯ���Ľڵ��
	u16 curpoll;

	//�����Ƿ�����Ҫ������ѯ�Ľڵ�
	//if( abs(time(NULL) - PolingTime) <= 51 ) return;   //�״���ѯʱ����

	//if(broad_time() != 0 ) return;

	if(abs(time(NULL) - PolingTime) <= 10)  //�ճ���ѯʱ���� ÿ�������ѯ100���ڵ�
	{
		return;
	}

	PolingPt ++;
	if( is_all_xx(NodeList[PolingPt].naddr, 0, 6) == 0) //�Ƿ����ڵ�
	{
		PolingPt = 0;
	}

	if( is_all_xx(NodeList[PolingPt].naddr, 0, 6) == 0) //�ڵ���Ч
	{
		return;
	}

	curpoll = PolingPt;


	send_Poll_msg(curpoll);
	#if 0
	u8 payload[6] = {0x09,8,9,14,36,50};
	lora_addr_t dest_addr;
    
    memset(&dest_addr.addr, 0xFF, 6);
	//send_lora_binding_msg(&dest_addr, &payload, 1);
	send_lora_broadcast_time(&dest_addr, payload, 6);
	#endif
	PolingTime = time(NULL);
}

#if 0
void send_Poll_msg(u16 curpoll)
{
	struct protocol3762_st plcmsg;
	u8 pt;
	int i;

	write_log(MSG_INFO, "send_Poll_msg %d\n", curpoll);
	//����ת����ʽ ���ʽڵ㣬 AFN02, Fn=1
	//GB645 CMD=1  �������ݱ�ʶ���ܣ�ֱ�������ݣ��������ݰ��� ʱ��(6�ֽ�)+���ȱ仯���ñ�
	plcmsg.head = 0x68;
	//plcmsg.len = P3762HEADLEN+18; //���� �����
	plcmsg.ctrl = 0x41;
	plcmsg.usrR[0] = 0x04;
	plcmsg.usrR[1] = 0x00;
	plcmsg.usrR[2] = 0x04;
	plcmsg.usrR[3] = 0x00;
	plcmsg.usrR[4] = 0x00;
	plcmsg.usrR[5] = SID++;
	memcpy(plcmsg.usrSAddr, ParaSet.PLCid, 6);
	memcpy(plcmsg.usrDAddr, NodeList[curpoll].naddr, 6);
	plcmsg.usrAFN = 0x02;
	plcmsg.usrDT[0] = 0x01;
	plcmsg.usrDT[1] = 0x00;
	plcmsg.usrData[0] = 0x02;  //Э������
	//plcmsg.usrData[1] = 14;    //���ĳ���, �����

	plcmsg.usrData[2] = 0x68;
	memcpy(&plcmsg.usrData[3], NodeList[curpoll].naddr, 6);
	plcmsg.usrData[9] = 0x68;
	plcmsg.usrData[10] = 0x14;
	//plcmsg.usrData[11] = 0x02;  //���ݳ���, �����

	pt = 12;
	Get_CurTime(plcmsg.usrData+pt, 6);     //ʱ��
	pt += 6;
	plcmsg.usrData[pt++] = ParaSet.ICCARD;    //�ϱ���Ƭ������1���ֽڣ�0-63��
	plcmsg.usrData[pt++] = ParaSet.PWDMODE;   //��Կ����ģʽ
	memcpy(plcmsg.usrData+pt, ParaSet.PWDBYTE, 6);  //��������Կ
	pt += 6;
	plcmsg.usrData[pt++] = ParaSet.CARDFUNC;       //ȡ�翪�ع�������ģʽ
	memcpy(plcmsg.usrData+pt, ParaSet.HOTELID, 5); //�Ƶ�ID��
	pt += 5;
	
	memcpy(plcmsg.usrData+pt, NodeList[curpoll].roomid, 3); //�����
	pt += 3;
	
	for(i=0; i<24; i++)
	{
		memcpy(plcmsg.usrData+pt, NodeList[curpoll].lightcfg+i*5, 5);
		pt += 5;
		if( NodeList[curpoll].lightcfg[i*5+1] == 0 )
			break;
	}

	plcmsg.usrData[11] = pt - 12;
	plcmsg.usrData[pt] = calc_bcc(plcmsg.usrData+2, pt-2);
	pt ++;
	plcmsg.usrData[pt++] = 0x16;

	plcmsg.usrData[1] = pt - 2;    //ת�����ĳ���
	plcmsg.len = P3762HEADLEN + pt + 2;
	plcmsg.usrData[pt] = calc_bcc((u8 *)&plcmsg.ctrl, P3762HEADLEN-3+pt);  //CS
	pt ++;
	plcmsg.usrData[pt] = 0x16;  //END
	pt ++;
	plcwrite(Plc_fd, (u8 *)&plcmsg, P3762HEADLEN+pt);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", (u8 *)&plcmsg, P3762HEADLEN+pt);

	save_plc_record((u8 *)&plcmsg, P3762HEADLEN+pt, ParaSet.PLCid);
}
#endif
void send_Poll_msg(u16 curpoll)
{
    lora_addr_t dest_addr;
    parameter_set_payload_t  para_payload;
    memcpy(&dest_addr.addr, NodeList[curpoll].naddr, 6);
    //    int idx;
    //send_lora_req_data(&dest_addr);

                 
    memcpy(&para_payload, &ParaSet.block_to_read, sizeof(parameter_set_payload_t) );

    #if 0
    send_lora_set_para(&dest_addr.addr, &para_payload, sizeof(parameter_set_payload_t) );
    for(idx = 0; idx++;idx < sizeof(parameter_set_payload_t) )
    {
                   printf("0x%x",*((char *)&para_payload + idx));
    }
    printf("\n");
    #endif
}

void get_node_energy(void)
{
	static time_t lgtime = 0;
	struct protocol3762_st plcmsg;
	u8 pt;
	
	if(abs(time(NULL) - lgtime) <= 60)  //�ճ���ѯʱ����
		return;
	
	lgtime = time(NULL);		

	write_log(MSG_INFO, "get node Energy\n");
	//����ת����ʽ ���ʽڵ㣬 AFN02, Fn=1
	//GB645 CMD=1  �������ݱ�ʶ���ܣ�ֱ�������ݣ��������ݰ��� ʱ��(6�ֽ�)+���ȱ仯���ñ�
	plcmsg.head = 0x68;
	//plcmsg.len = P3762HEADLEN+18; //���� �����
	plcmsg.ctrl = 0x41;
	plcmsg.usrR[0] = 0x04;
	plcmsg.usrR[1] = 0x00;
	plcmsg.usrR[2] = 0x04;
	plcmsg.usrR[3] = 0x00;
	plcmsg.usrR[4] = 0x00;
	plcmsg.usrR[5] = SID++;
	memcpy(plcmsg.usrSAddr, ParaSet.PLCid, 6);
	memcpy(plcmsg.usrDAddr, NodeList[0].naddr, 6);
	plcmsg.usrAFN = 0x02;
	plcmsg.usrDT[0] = 0x01;
	plcmsg.usrDT[1] = 0x00;
	plcmsg.usrData[0] = 0x02;  //Э������
	//plcmsg.usrData[1] = 14;    //���ĳ���, �����

	plcmsg.usrData[2] = 0x68;
	memcpy(&plcmsg.usrData[3], NodeList[0].naddr, 6);
	plcmsg.usrData[9] = 0x68;
	plcmsg.usrData[10] = 0x14;
	//plcmsg.usrData[11] = 0x02;  //���ݳ���, �����

	pt = 12;
	plcmsg.usrData[pt++] = 0xFE;    //����ţ� 0xFE

	plcmsg.usrData[11] = pt - 12;
	plcmsg.usrData[pt] = calc_bcc(plcmsg.usrData+2, pt-2);
	pt ++;
	plcmsg.usrData[pt++] = 0x16;

	plcmsg.usrData[1] = pt - 2;    //ת�����ĳ���
	plcmsg.len = P3762HEADLEN + pt + 2;
	plcmsg.usrData[pt] = calc_bcc((u8 *)&plcmsg.ctrl, P3762HEADLEN-3+pt);  //CS
	pt ++;
	plcmsg.usrData[pt] = 0x16;  //END
	pt ++;
	plcwrite(Plc_fd, (u8 *)&plcmsg, P3762HEADLEN+pt);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", (u8 *)&plcmsg, P3762HEADLEN+pt);

	save_plc_record((u8 *)&plcmsg, P3762HEADLEN+pt, ParaSet.PLCid);
}



void send_setmode_msg(u8 *addr, u8 mode, u8 cmd)
{
	struct protocol3762_st plcmsg;
	u8 pt;

	write_log(MSG_INFO, "send_setmode_msg addr=%02X%02X%02X%02X%02X%02X mo=%02X\n", addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],mode);
	//����ת����ʽ ���ʽڵ㣬 AFN02, Fn=1
	//GB645 CMD=1  �������ݱ�ʶ���ܣ�ֱ�������ݣ��������ݰ��� ʱ��(6�ֽ�)+���ȱ仯���ñ�
	plcmsg.head = 0x68;
	//plcmsg.len = P3762HEADLEN+18; //���� �����
	plcmsg.ctrl = 0x41;
	plcmsg.usrR[0] = 0x04;
	plcmsg.usrR[1] = 0x00;
	plcmsg.usrR[2] = 0x04;
	plcmsg.usrR[3] = 0x00;
	plcmsg.usrR[4] = 0x00;
	plcmsg.usrR[5] = SID++;
	memcpy(plcmsg.usrSAddr, ParaSet.PLCid, 6);
	memcpy(plcmsg.usrDAddr, addr, 6);
	plcmsg.usrAFN = 0x02;
	plcmsg.usrDT[0] = 0x01;
	plcmsg.usrDT[1] = 0x00;
	plcmsg.usrData[0] = 0x02;  //Э������
	//plcmsg.usrData[1] = 14;    //���ĳ���, �����

	plcmsg.usrData[2] = 0x68;
	memcpy(&plcmsg.usrData[3], addr, 6);
	plcmsg.usrData[9] = 0x68;
	plcmsg.usrData[10] = 0x14;
	//plcmsg.usrData[11] = 0x02;  //���ݳ���, �����

	pt = 12;
	if(cmd == 0x88)  //���ƿ���
	{
		plcmsg.usrData[pt++] = 0xAA;    //����ţ� 0xFF
		plcmsg.usrData[pt++] = 00;    //��������
	}
	else   //���Ƽ̵���״̬
	{
		plcmsg.usrData[pt++] = 0xFF;    //����ţ� 0xFF
		plcmsg.usrData[pt++] = mode;    //��������
	}

	plcmsg.usrData[11] = pt - 12;
	plcmsg.usrData[pt] = calc_bcc(plcmsg.usrData+2, pt-2);
	pt ++;
	plcmsg.usrData[pt++] = 0x16;

	plcmsg.usrData[1] = pt - 2;    //ת�����ĳ���
	plcmsg.len = P3762HEADLEN + pt + 2;
	plcmsg.usrData[pt] = calc_bcc((u8 *)&plcmsg.ctrl, P3762HEADLEN-3+pt);  //CS
	pt ++;
	plcmsg.usrData[pt] = 0x16;  //END
	pt ++;
	plcwrite(Plc_fd, (u8 *)&plcmsg, P3762HEADLEN+pt);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", (u8 *)&plcmsg, P3762HEADLEN+pt);

	save_plc_record((u8 *)&plcmsg, P3762HEADLEN+pt, ParaSet.PLCid);
}


u8 check_plc_frame(struct protocol3762_st* rmsg, u8* buf, u16 len)
{
	u8 i;
	u8 tbuf[sizeof(struct protocol3762_st)+32];

	for(i=0; i<100; i++)
	{
		if(buf[i] == 0x68)
		{
			break;
		}
	}
	if(i >= 100) return 1;
	memmove(buf, buf+i, len);

	if(buf[4]&0x04)
	{
		memcpy((u8 *)rmsg, buf, sizeof(struct protocol3762_st));
	}
	else
	{
		bzero(tbuf, sizeof(tbuf));
		memcpy(tbuf, buf, 10);
		memcpy(tbuf+22, buf+10, sizeof(struct protocol3762_st)-22);
		memcpy((u8 *)rmsg, tbuf, sizeof(struct protocol3762_st));
		//write_comm(MSG_COMMT+MSG_DEBUG, "tbuf:", tbuf, len+12);
	}

	write_log(MSG_INFO, "AFN=%02X, Fn=%02x %02x \n", rmsg->usrAFN, rmsg->usrDT[0], rmsg->usrDT[1]);

	return 0;
}


//��AFN=5 fn=1����
void set_plc_node_addr(void)
{
	u8 sbuf[21] = {0x68, 0x15, 0x00, 0x41, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,   0xA9, 0x16};

	memcpy(sbuf+13, ParaSet.PLCid, 6);
	//serverbuf(sbuf+13, 6);
	sbuf[19] = calc_bcc(sbuf+3, 16);  //CS
	sbuf[20] = 0x16;  //END
	plcwrite(Plc_fd, sbuf, 21);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", sbuf, 21);

	PolingTime = time(NULL) - 45;
}

#if 0
//��AFN=5 fn=3����
u8 broad_time(void)
{
	u8 tbuf[6];
	u8 sbuf[64] = {0x68, 0x23, 0x00, 0x41,  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,  0x05, 0x04, 0x00};
	static time_t lastbtime = 0;
	static u8 flag = 0;

/*
	//������ÿ��3��4����ж�ʱ
	static u8 day = 0xff, hour = 0xff;
	u8 flag = 0;
	Get_CurTime(tbuf, 6);
	if( (day == 0xff) && (hour == 0xff) )
	{
		flag = 0xaa;
	}
	else
	{
		if( (tbuf[3] == 3) || (tbuf[3] == 4) )
		{
			if((day != tbuf[2]) || (hour != tbuf[3]))
			{
				flag = 0xaa;
			}
		}
	}

	if(flag == 0) return 0;

	day = tbuf[2];
	hour = tbuf[3];
*/

	if( abs(time(NULL) - lastbtime) < flag*293 ) return 0;
	if(flag < 6) flag ++;
	lastbtime = time(NULL);

	write_log(MSG_INFO, "broad_time...\n");

	Get_CurTime(tbuf, 6);
	sbuf[13] = 2;
	sbuf[14] = 0x12;
	sbuf[15] = 0x68;
	sbuf[16] = 0x99;
	sbuf[17] = 0x99;
	sbuf[18] = 0x99;
	sbuf[19] = 0x99;
	sbuf[20] = 0x99;
	sbuf[21] = 0x99;
	sbuf[22] = 0x68;
	sbuf[23] = 0x08;
	sbuf[24] = 0x06;
	sbuf[25] = uchar2bcd(tbuf[5]) + 0x33;
	sbuf[26] = uchar2bcd(tbuf[4]) + 0x33;
	sbuf[27] = uchar2bcd(tbuf[3]) + 0x33;
	sbuf[28] = uchar2bcd(tbuf[2]) + 0x33;
	sbuf[29] = uchar2bcd(tbuf[1]) + 0x33;
	sbuf[30] = uchar2bcd(tbuf[0]) + 0x33;
	sbuf[31] = calc_bcc(sbuf+15, 16);
	sbuf[32] = 0x16;
	sbuf[33] = calc_bcc(sbuf+3, 30);
	sbuf[34] = 0x16;
	plcwrite(Plc_fd, sbuf, 35);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", sbuf, 35);
	
	save_plc_record(sbuf, 35, ParaSet.PLCid);
/*	
	sleep(7);
	//�㲥���ò���  ���ݻظ���Чʱ��	
	sbuf[9] = 1;       //�������к�
	sbuf[13] = 1;      //���ĸ�ʽ
	sbuf[14] = 0x13;   //���ĳ���  68 99 99 99 99 99 99 68 04 07 8E 8D 85 76 7F 83 48 D1 16
	sbuf[15] = 0x68;
	sbuf[16] = 0x99;
	sbuf[17] = 0x99;
	sbuf[18] = 0x99;
	sbuf[19] = 0x99;
	sbuf[20] = 0x99;
	sbuf[21] = 0x99;
	sbuf[22] = 0x68;
	sbuf[23] = 0x04;
	sbuf[24] = 0x07;
	sbuf[25] = 0x8E;
	sbuf[26] = 0x8D;
	sbuf[27] = 0x85;
	sbuf[28] = 0x76;
	sbuf[29] = 0x7F;
	sbuf[30] = 0x83;
	sbuf[31] = 0x48;
	sbuf[32] = 0xD1;
	sbuf[33] = 0x16;
	sbuf[34] = calc_bcc(sbuf+3, 31);
	sbuf[35] = 0x16;
	plcwrite(Plc_fd, sbuf, 36);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", sbuf, 36);
	
	save_plc_record(sbuf, 36, ParaSet.PLCid);	
*/	
	return 1;
}
#endif

u8 broad_time(void)
{
	u8 tbuf[6];
	u8 sbuf[64] = {0x68, 0x23, 0x00, 0x41,  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,  0x05, 0x04, 0x00};
	static time_t lastbtime = 0;
	static u8 flag = 0;

/*
	//������ÿ��3��4����ж�ʱ
	static u8 day = 0xff, hour = 0xff;
	u8 flag = 0;
	Get_CurTime(tbuf, 6);
	if( (day == 0xff) && (hour == 0xff) )
	{
		flag = 0xaa;
	}
	else
	{
		if( (tbuf[3] == 3) || (tbuf[3] == 4) )
		{
			if((day != tbuf[2]) || (hour != tbuf[3]))
			{
				flag = 0xaa;
			}
		}
	}

	if(flag == 0) return 0;

	day = tbuf[2];
	hour = tbuf[3];
*/

	if( abs(time(NULL) - lastbtime) < flag*293 ) return 0;
	if(flag < 6) flag ++;
	lastbtime = time(NULL);

	write_log(MSG_INFO, "broad_time...\n");

	Get_CurTime(tbuf, 6);
	sbuf[13] = 2;
	sbuf[14] = 0x12;
	sbuf[15] = 0x68;
	sbuf[16] = 0x99;
	sbuf[17] = 0x99;
	sbuf[18] = 0x99;
	sbuf[19] = 0x99;
	sbuf[20] = 0x99;
	sbuf[21] = 0x99;
	sbuf[22] = 0x68;
	sbuf[23] = 0x08;
	sbuf[24] = 0x06;
	sbuf[25] = uchar2bcd(tbuf[5]) + 0x33;
	sbuf[26] = uchar2bcd(tbuf[4]) + 0x33;
	sbuf[27] = uchar2bcd(tbuf[3]) + 0x33;
	sbuf[28] = uchar2bcd(tbuf[2]) + 0x33;
	sbuf[29] = uchar2bcd(tbuf[1]) + 0x33;
	sbuf[30] = uchar2bcd(tbuf[0]) + 0x33;
	sbuf[31] = calc_bcc(sbuf+15, 16);
	sbuf[32] = 0x16;
	sbuf[33] = calc_bcc(sbuf+3, 30);
	sbuf[34] = 0x16;
	plcwrite(Plc_fd, sbuf, 35);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", sbuf, 35);
	
	save_plc_record(sbuf, 35, ParaSet.PLCid);
	return 1;
}

void set_testplc_node(void)
{
	u8 sbuf[15] = {0x68, 0x0F, 0x00, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x46, 0x16};

	plcwrite(Plc_fd, sbuf, 21);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", sbuf, 21);
}

//Ӧ��ȷ�����  fn=1  ȷ�ϣ� fn=2  ���ϣ�
void send_A00(u8 fn)
{
	u8 sbuf[64];
	u8 pt = 0;

	sbuf[pt++] = 0x68;
	sbuf[pt++] = 0;      //����,������ʱ�������
	sbuf[pt++] = 0;
	sbuf[pt++] = 0x01;   //������
	sbuf[pt++] = 0x01;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0x40;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = SID++;
	sbuf[pt++] = 0x00;   //AFN
	sbuf[pt++] = fn;     //FN
	sbuf[pt++] = 0x00;
	if(fn == 1)
	{
		sbuf[pt++] = 0xff;
		sbuf[pt++] = 0xff;
		sbuf[pt++] = 0x00;
		sbuf[pt++] = 0x00;
	}
	else
	{
		sbuf[pt++] = 0x00;
	}
	sbuf[pt] = calc_bcc(sbuf+3, pt-3);  //CS
	pt++;
	sbuf[pt++] = 0x16;  //END
	sbuf[1] = pt;  //����

	memcpy(PLCSbuf+PLChead, sbuf, pt);
	plcwrite(Plc_fd, PLCSbuf, pt+PLChead);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", PLCSbuf, pt+PLChead);
}

//68 1D 00 01 01 00 40 00 00 00 00 01 00 FF FF 00 00 X0 XX XX XX XX XX XX XX XX X9 41 16
//Ӧ��ȷ�����  fn=1  ȷ�ϣ� fn=2  ���ϣ�
void ask_A06F02(struct protocol3762_st rmsg)
{
	u8 tbuf[6];
	u8 sbuf[64];
	u8 pt = 0;

	sbuf[pt++] = 0x68;
	sbuf[pt++] = 0;      //����,������ʱ�������
	sbuf[pt++] = 0;
	sbuf[pt++] = 0x01;   //������
	sbuf[pt++] = 0x01;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0x40;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = SID++;
	sbuf[pt++] = 0x00;   //AFN
	sbuf[pt++] = 01;     //FN
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0xff;
	sbuf[pt++] = 0xff;
	sbuf[pt++] = 0x00;
	sbuf[pt++] = 0x00;

	Get_CurTime(sbuf+pt, 6);
	memcpy(tbuf, sbuf+pt, 6);
	pt += 6;
	sbuf[pt++] = rmsg.usrData[14];


	sbuf[pt++] = ((rmsg.usrData[17]&0xE0) >> 1) + ((rmsg.usrData[16]&0xF0) >> 4);
	sbuf[pt++] = ((rmsg.usrData[39]&0xE0) >> 1) + ((rmsg.usrData[38]&0xF0) >> 4);
	
	sbuf[pt++] = 0x00;

	sbuf[pt] = calc_bcc(sbuf+3, pt-3);  //CS
	pt++;
	sbuf[pt++] = 0x16;  //END
	sbuf[1] = pt;  //����
	
	memcpy(PLCSbuf+PLChead, sbuf, pt);
	plcwrite(Plc_fd, PLCSbuf, pt+PLChead);
	write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", PLCSbuf, pt+PLChead);
	
/*	
	//�ڵ����������޸ĵĲ��ȴ���  ���ƻ������ ԭ����Ҫ&����>>1�ģ�����û��>>1  
	if(tbuf[2] < 0x10)
	{
		sbuf[pt++] = ((rmsg.usrData[17]&0xF0) >> 0) + ((rmsg.usrData[16]&0xF0) >> 4);
		sbuf[pt++] = ((rmsg.usrData[39]&0xF0) >> 0) + ((rmsg.usrData[38]&0xF0) >> 4);
		
		sbuf[pt++] = 0x00;

		sbuf[pt] = calc_bcc(sbuf+3, pt-3);  //CS
		pt++;
		sbuf[pt++] = 0x16;  //END
		sbuf[1] = pt;  //����
	
		memcpy(PLCSbuf+PLChead, sbuf, pt);
		plcwrite(Plc_fd, PLCSbuf, pt+PLChead);
		write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", PLCSbuf, pt+PLChead);
	}
	else
	{
		sbuf[pt++] = ((rmsg.usrData[17]&0xF0) >> 0) + ((rmsg.usrData[16]&0xF0) >> 4);
		sbuf[pt++] = ((rmsg.usrData[39]&0xF0) >> 0) + ((rmsg.usrData[38]&0xF0) >> 4);
		
		sbuf[pt++] = 0x00;

		sbuf[pt] = calc_bcc(sbuf+3, pt-3);  //CS
		pt++;
		sbuf[pt++] = 0x16;  //END
		sbuf[1] = pt;  //����
	
		memcpy(PLCSbuf+PLChead, sbuf, pt);
		plcwrite(Plc_fd, PLCSbuf, pt+PLChead);
		write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", PLCSbuf, pt+PLChead);
		
		sleep(1);
		
		sbuf[pt++] = ((rmsg.usrData[17]&0xE0) >> 0) + ((rmsg.usrData[16]&0xF0) >> 4);
		sbuf[pt++] = ((rmsg.usrData[39]&0xE0) >> 0) + ((rmsg.usrData[38]&0xF0) >> 4);
		
		sbuf[pt++] = 0x00;

		sbuf[pt] = calc_bcc(sbuf+3, pt-3);  //CS
		pt++;
		sbuf[pt++] = 0x16;  //END
		sbuf[1] = pt;  //����
	
		memcpy(PLCSbuf+PLChead, sbuf, pt);
		plcwrite(Plc_fd, PLCSbuf, pt+PLChead);
		write_comm(MSG_COMMT+MSG_DEBUG, "PLC send:", PLCSbuf, pt+PLChead);
	}
*/	
}


//����û��ͨ�ųɹ����Ľڵ㣬���Ƚ�����ѯ
//���� 0  û��ûͨ�ųɹ��Ľڵ���Ҫ������ѯ
//���� 1  ��ûͨ�ųɹ��Ľڵ���Ҫ������ѯ
//flag = 0  ��ѯ��ѯ
//flag = 1  ���ø����ˣ� ��Ҫ�Ͽ������ѯ��
u8 get_rush_poling(u8 flag, u16 *ndpt)
{
	static u8 scantime = 0;
	static u16 scanpt = 0;
	static time_t scantt = 0;
	static time_t lasttt = 0;
	
	if(flag == 1) 
	{
		scantime = 0;
		scanpt = 0;
		return;
	}

	if( abs(time(0) - scantt) < (u32)scantime*1800 )  return 0;
	if( scantime && (abs(time(0) - lasttt) < 60) )  return 0;

	for( ; scanpt<MAXNODENUM; scanpt++)
	{
		if( is_all_xx(NodeList[scanpt].naddr, 0, 6) == 0) //�Ƿ����ڵ�
		{
			scanpt = 0;
			scantt = time(0);
			if(scantime < 43) scantime ++;
			update_node_cfgfile();
			return 0;
		}

		if(NodeList[scanpt].pollt < 0xaa)
		{
			*ndpt = scanpt;
			scanpt ++;
			lasttt = time(0);
			return 1;
		}
	}

	return 0;
}

void plcwrite(int fd, u8 *sbuf, u8 len)
{
	u8 plbuf[512];

	plbuf[0] = 0xfe;
	plbuf[1] = 0xfe;
	plbuf[2] = 0xfe;
	plbuf[3] = 0xfe;

	memcpy(plbuf+4, sbuf, len);
	write(fd, plbuf, len+4);
}