//**********************************************
// Data Collecting Module
// author:wjx
//**********************************************

#include "comm.h"

//外部变量声明
extern u8 nDebugLevel; //定义在public.c
extern u8 nLogConsole;
extern int Gprs_fd;

u8 COMM_flag = SDATA_HAD_BEGIN;      //当前报文通信标记
time_t COMMtt = 0;                   //最后一次发报文到主站时间
struct SMSGBuffStr *SMSGList = NULL; //发送还没收到应答的列表 --用于重发
u8 SMSG_num = 0;                     //发送还没收到应答的报文数
u8 SID;                              //帧序号
u8 Comm_type = 0;            //通讯模式 0：GPRS模式； 1：RS485模式
u8 SGPRS_flag = RECONNECT;   //GPRS拨号状态
u8 Sturnon_count = 0x0f;     //软复位modem次数
u8 Sdail_count = 0x0f;       //连续拨号失败次数
u8 Scont_count = 0x0f;       //重联接服务器次数
u8 Scommfail_count = 0x0f;   //通信失败次数
time_t Scommfail_time = 0;   //通信失败的时刻
time_t RecvServerMsgT = 0;   //最后接收到后台服务器的时刻
struct parameter_set ParaSet;  //全局参数设置
int FDR_pipe, FDW_pipe;

u16 Maxframe;         //升级文件存放缓冲器大小
u16 TotalFrame;       //升级文件传送的帧数
u32 Uflag[64] = {0};  //已下载的升级帧标记
time_t Udtime = 0xffffffff;  //正在升级标志，当超30秒没收到最后一个升级帧时，返回信息让后台重发
u8 dnlpwd[64] = {0};  //要下载的文件路径

u8 RecvBuf[2048];     //GPRS串口接收缓冲区
u16 RecvLen=0;

u16 SNpt = 0;                        //0x84命令已下发的节点数量
u16 SNflag = 0x00;
u8 SNaddr[MAXNODENUM+1][6] = {{0}};  //后台配置的集中器地址数据

//亮度配置参数
static u8 listflag = 0;   //为了防止频繁读取文件，增加这个标志，当为0时表示有跟新过， 需要重新读取文件
static u8 light_list[MAXNODENUM+1][8];

/*
 * 函数名  : main
 * 函数描述: 主函数
 * 输入参数: argc，argv
 * 输出    :
 * 返回值  :
 */
s32 main(s32 argc, char *argv[])
{
	int count;
    u8 ret = 0;
	//DLdc调试等级
	nDebugLevel = 2;
	if (argc >= 2)  nDebugLevel = (u8)atoi(argv[1]);
	nLogConsole = (argc >= 3) ? (u8)atoi(argv[2]) : 0;

	nDebugLevel = 255;  //???

	gave_soft_dog("comm");
	//初始化socket和pipe
	main_init();

	write_version(1, COMM_VERVION);

	while (1)
	{
		if(ParaSet.COMMode == 2) //ETH
		{
			get_udp_msg();
		}
		else   //2G
		{
			RecvLen = ttyread(Gprs_fd, RecvBuf, 2032, 20);
			if(RecvLen != 0) //GPRS命令处理
			{
				Gprs_recv_process();
				//printf("end Gprs_recv_process()...\n");
			}
		}

		usleep(100000);
		count++;
		if(count > 100)
		{
			count = 0;
			//hearbeat_server();
		}

		//拨号通信
		if( (SGPRS_flag == RECONNECT) && (ParaSet.COMMode != 2))
		{
			SGPRS_flag = connect_GPRS_net();
		}

		if(Udtime != 0xffffffff)
		{
			if(abs(time(NULL) - Udtime) > 30) reask_update_frame();
		}
		else
		{
			if(COMM_flag == SDATA_HAD_RET)
			{
				//补发数据到服务器
				reissued_smsg();
			}

			//处理内部pipe通信
			while( pipe_msg_process() != 0 ) ;

			heart_server_msg(NULL);
		}

		gave_soft_dog("comm");
		record_comm_flag();
	}
}


/*
 * 函数名  : main_init
 * 函数描述: sock、pipe等初始化
 * 输入参数:
 * 输出    :
 * 返回值  :
 */
void main_init(void)
{
	gpio_init();
	//init_param(&ParaSet);
	//disp_paraset(&ParaSet);
	//copy_nocomm_list();

	//创建fifo命名管道
	//if (Init_NamePipe(PIPE_CtoM, &FDR_pipe, &FDW_pipe) != 0)
	{
	//	write_log(MSG_INFO, "\n !!!!!  init name pipe ERROR !!!! \n");
	}

	GPRS_pwr_off();
	Gprs_fd = set_serial("/dev/ttyS2", 115200L, "8/N/1", 0);
	write_log(MSG_INFO, "comm program start OK Gprs_fd=%d\n", Gprs_fd);
}



//GPRS命令处理
void Gprs_recv_process(void)
{
	int ret;
	u16 i;

	for(i=0; i<16; i++) RecvBuf[RecvLen+i] = 0;
	//write_log(MSG_INFO, "GPRS rlen%d: %s", RecvLen, RecvBuf);
	write_comm(MSG_COMMR+MSG_DEBUG, "GPRS recv: ",  RecvBuf, RecvLen);

	if(RecvLen < 15)
	{
		return;
	}


	for(i=0; i < RecvLen-15; i++)
	{
		if(memcmp(RecvBuf+i, "RECEIVE,0,", 10) == 0)
		{
			Sturnon_count = 0;
			Sdail_count = 0;         //连续拨号失败次数
			Scont_count = 0;         //重联接服务器次数
			Scommfail_count = 0;     //通信失败次数
			Scommfail_time = time(NULL);
			RecvServerMsgT = time(NULL);

			ret = mystrtol(RecvBuf+i+9, NULL);
			//printf("begin: RecvLen=%d, i=%d, ret=%d \n", RecvLen, i, ret);
			if(ret < 0 ) ret = 0;
			while(RecvBuf[i+11] != ':') i++;
			if(RecvLen < i + 11 + ret)
			{
				write_log(MSG_INFO, "2G Recv msg ERR...\n");
				return;
			}

			while( (RecvBuf[i+11] != 0x68) && (i<RecvLen-11) ) {i++;}
			write_comm(MSG_COMMR+MSG_DEBUG, "2G rmsg:", RecvBuf+11+i, ret);
			process_server_msg(RecvBuf+11+i, ret);

			i = i+ret;

			//printf("end  : RecvLen=%d, i=%d, ret=%d \n", RecvLen, i, ret);

			//while( (RecvBuf[RecvLen-1] != 0x16) && (RecvLen>10) ) RecvLen --;
			//write_comm(MSG_COMMR+MSG_DEBUG, "2G rmsg:", RecvBuf+11+i, RecvLen-11-i);
			//process_server_msg(RecvBuf+11+i, RecvLen-11-i);
			//return;
		}
	}

	//printf("end Gprs_recv_process()\n");
}


int pipe_msg_process(void)
{
	u8 mbuf[256];
	int rlen;
	u16 MsgType = 0;

	rlen = read_pipe_msg(FDR_pipe, mbuf);
	if(rlen <= 0) return 0;

	//printf("begin pipe_msg_process()...\n");

	MsgType = (u16)mbuf[TYPEPOS+1]*0x100 + mbuf[TYPEPOS];

	//printf("pipe type= %04x\n", MsgType);

	switch(MsgType)
	{
		case PIPE_UPDATE:  //节点数据需要上报
			send_to_server(mbuf+DATAPOS, mbuf[LENPOS]);
			break;

		case PIPE_REBOOT:  //wdog通知，集中器即将重启
			comm_reboot_done();
			break;

		case PIPE_PAPAUP_OK:  //节点参数同步成功
			heart_server_msg(mbuf+DATAPOS);
			break;

		case PIPE_SETNODE_OK:  //节点远程设置成功
		{
			u8 sbuf[32];
			u8 cs;
			int i;
			u8 pt;

			sbuf[0] = 0x68;
			memcpy(sbuf+2, ParaSet.Devid, 6);
			memcpy(sbuf+8, mbuf+DATAPOS, 6);
			Get_CurTime(sbuf+14, 6);

			if(mbuf[DATAPOS+6] == 0xFF)    //控制继电器动作
			{
				sbuf[1]  = 0x06;
				sbuf[20] = mbuf[DATAPOS+7];
				pt = 21;
			}
			else if(mbuf[DATAPOS+6] == 0xAA)   //控制门锁开门
			{
				sbuf[1]  = 0x08;
				sbuf[20] = mbuf[DATAPOS+7];
				pt = 21;
			}
			else if(mbuf[DATAPOS+6] == 0xFE)   //读取电量
			{
				sbuf[1]  = 0x07;
				sbuf[20] = mbuf[DATAPOS+7];
				sbuf[21] = mbuf[DATAPOS+8];
				sbuf[22] = mbuf[DATAPOS+9];
				sbuf[23] = mbuf[DATAPOS+10];
				pt = 24;
			}

			cs = 0;
			for(i=0; i<pt; i++)
			{
				cs += sbuf[i];
			}
			sbuf[pt] = cs;
			sbuf[pt+1] = 0x16;
			send_to_server(sbuf, pt+2);
		}
		break;

		default:
			break;
	}

	return 1;
	//printf("end pipe_msg_process()...\n");
}


//开机时，从flash盘中恢复需要补上报的文件到ram盘中
void copy_nocomm_list(void)
{
	u8 i;
	c8 fname[64];

	//保存列表内容， 待通信链路通畅后再补发
	if(access("/tmp/SmsgPath/", F_OK) != 0)
	{
		system("mkdir /tmp/SmsgPath");
	}

	for(i=0; i<20; i++) //保存这些未能上报的消息，需要20个文件
	{
		sprintf(fname, "/MeterRoot/SmsgPath/smsg%02d", i);
		if(access(fname, F_OK) == 0)
		{
			sprintf(fname, "mv /MeterRoot/SmsgPath/smsg%02d /tmp/SmsgPath/", i);
			system(fname);
			usleep(200000);
		}
	}
}

//集中器需要重启时，保存通讯未成功列表保存到文件，并复制到flash盘
//把通信记录复制到flash盘中
void comm_reboot_done(void)
{
	FILE *fp;
	s32 i;
	c8 fname[128];
	struct SMSGBuffStr *snext;
	struct SMSGBuffStr *smsgst;

	write_log(MSG_INFO, "comm_reboot_done...\n");

	//将没上送成功的消息保存到文件中
	if(access("/tmp/SmsgPath/", F_OK) != 0)
	{
		system("mkdir /tmp/SmsgPath");
	}
	for(i=0; i<20; i++) //保存这些未能上报的消息，需要20个文件
	{
		sprintf(fname, "/tmp/SmsgPath/smsg%02d", i);
		if(access(fname, F_OK) != 0)
		{
			fp = fopen(fname, "rw+");
		}
	}
	if( i >=  20 )
	{
		sprintf(fname, "/tmp/SmsgPath/smsg00");
		fp = fopen(fname, "r+");
	}
	smsgst = SMSGList;
	if(fp == NULL) return;

	//将上送队列的消息保存到文件
	while(smsgst != NULL)
	{
		fwrite((u8 *)smsgst,1, sizeof(struct SMSGBuffStr)-4, fp);
		snext = smsgst->next;
		free(smsgst);
		smsgst = snext;
	}
	SMSGList = NULL;
	fclose(fp);


	if(access("/MeterRoot/SmsgPath/", F_OK) != 0)
	{
		system("mkdir /MeterRoot/SmsgPath/");
	}

	for(i=0; i<20; i++) //保存这些未能上报的消息，需要20个文件
	{
		sprintf(fname, "/tmp/SmsgPath/smsg%02d", i);
		if(access(fname, F_OK) == 0)
		{
			sprintf(fname, "mv /tmp/SmsgPath/smsg%02d /MeterRoot/SmsgPath/", i);
			system(fname);
			usleep(200000);
		}
	}

	//把通信记录复制到flash盘中
	//扫描flash文件，查找要存储的文件。
	memset(fname,0x00,sizeof(fname));
	sprintf(fname, "cat %sSmsgtmp>>%sSmsg && rm %sSmsgtmp &", LogTpath, LogPath, LogTpath);
	system(fname);
	usleep(500000);

	return;
}


//重发消息到服务器
void reissued_smsg(void)
{
	static time_t lrft = 0;
	struct SMSGBuffStr *last;
	struct SMSGBuffStr *smsgst;
	c8 fname[32];
	u8 i;

	if(abs(time(NULL) - COMMtt) < 3)  return;

	//printf("begin reissued_smsg()...\n");

	if( (SMSG_num < 5) && (abs(time(NULL) - lrft) > 120) )  //小于5条补传记录，每2分钟检查一次需补发的文件 ???
	{
		lrft = time(NULL);

		//???读取文件中的未发送报文来补发送
		for(i=0; i<20; i++) //保存这些未能上报的消息，需要20个文件
		{
			sprintf(fname, "/tmp/SmsgPath/smsg%02d", i);
			if(access(fname, F_OK) == 0)
			{
				FILE *fp;
				u8 sbuf[264];

				fp = fopen(fname, "rb");
				if(fp == NULL) return;

				//printf("read reissued file %s\n", fname);

				while(fread(sbuf, 1, sizeof(struct SMSGBuffStr)-4, fp) > 0)
				{
					smsgst = (struct SMSGBuffStr *)malloc(sizeof(struct SMSGBuffStr));
					memcpy((u8*)smsgst, sbuf, sbuf[0]+1);
					smsgst->next = SMSGList;
					SMSGList = smsgst;
					SMSG_num ++;
					show_smsglst();
				}
				fclose(fp);
				unlink(fname);

				break;
			}
		}
	}

	last = SMSGList;
	if(SMSG_num >= 4)   //列表中有大于4条数据 前两条不重发
	{
		if(last != NULL) last = last->next;
		if(last != NULL) last = last->next;
	}

	if(last == NULL) return;

	if(ParaSet.COMMode == 2) //ETH
	{
		udp_send(ParaSet.SRVRaddr, ParaSet.SRVRport, last->sbuf, last->len);
	}
	else           //2G
	{
		GPRS_send_data(last->sbuf, last->len);
	}

	COMM_flag = SDATA_WAIT_RET;
	COMMtt = time(NULL);

	show_smsglst();
	//printf("end reissued_smsg()...\n");
}


void send_to_server(u8 *sbuf, int len)
{
	struct SMSGBuffStr *smsgst;

	COMMtt = time(NULL);
	if(ParaSet.COMMode == 2) //ETH
	{
		udp_send(ParaSet.SRVRaddr, ParaSet.SRVRport, sbuf, len);
	}
	else           //2G
	{
		GPRS_send_data(sbuf, len);
	}

	if(sbuf[1] >= 0x06)  return;  //报文升级的指令不保存，不重发，不需要应答

	COMM_flag = SDATA_WAIT_RET;

	if( (sbuf[1] != 0x00) && (sbuf[1] != 0x01)  && (sbuf[1] != 0x05) ) return;   //只有房态信息， 插卡信息需要补上送
	//保存到未接收应答的消息列表中
	smsgst = (struct SMSGBuffStr *)malloc(sizeof(struct SMSGBuffStr));
	memcpy(smsgst->sbuf, sbuf, len);
	smsgst->len = len;
	smsgst->next = SMSGList;
	SMSGList = smsgst;
	SMSG_num ++;

	if(SMSG_num > 120)
	{
		c8 fname[32];
		FILE *fp;
		struct SMSGBuffStr *snext;
		u8 i, j;

		//???保存列表内容， 待通信链路通畅后再补发
		if(access("/tmp/SmsgPath/", F_OK) != 0)
		{
			system("mkdir /tmp/SmsgPath");
		}
		for(i=0; i<20; i++) //用20个文件,保存这些未能上报的消息
		{
			sprintf(fname, "/tmp/SmsgPath/smsg%02d", i);
			if(access(fname, F_OK) != 0)
			{
				smsgst = SMSGList;
				for(j=0; j<19; j++)  //前面20条记录不保存到文件
				{
					if(smsgst == NULL)
					{
						write_log(MSG_INFO, " SMSGList ERROR !!!\n");
						return;
					}
					smsgst = smsgst->next;
				}

				fp = fopen(fname, "wb+");
				if(fp == NULL) return;

				snext = smsgst->next;
				smsgst->next = NULL;

				smsgst = snext;
				SMSG_num = 20;

				//printf("snext=%x; smsgst=%x; snext->next=%x; smsgst->next=%x\n", snext, smsgst, snext->next, smsgst->next);

				while(smsgst != NULL)
				{
					//printf("write %d  buf=%02x   smsgst->next=%x   \n", n++, smsgst->sbuf[17], smsgst->next);
					fwrite((u8 *)smsgst,1, sizeof(struct SMSGBuffStr)-4, fp);
					snext = smsgst->next;
					free(smsgst);
					smsgst = snext;
				}

				fclose(fp);
				show_smsglst();
				return;
			}
		}
		if(i >= 20)
		{
			static u8 count = 0;

			sprintf(fname, "/tmp/SmsgPath/smsg%02d", count);
			unlink(fname);

			count ++;
			if(count >= 20) count = 0;
		}
	}

	if(access("/tmp/lsmsg", F_OK) == 0)
	{
		show_smsglst();
	}
}


void show_smsglst(void)
{
	struct SMSGBuffStr *smsgst;
	u8 i = 1;
	c8 srbuf[32];

	if(access("/tmp/lsmsg", F_OK) != 0) return;
	printf("show_smsglst： SMSG_num = %d\n", SMSG_num);
	smsgst = SMSGList;
	while(smsgst != NULL)
	{
		sprintf(srbuf, "%3d %lX %lX smsglst: ", i++, smsgst, smsgst->next);
		write_comm(MSG_COMMR+MSG_DEBUG, srbuf, smsgst->sbuf, smsgst->len);
		smsgst = smsgst->next;
	}
}


void process_server_msg(u8 *buf, int dlen)
{
	struct SMSGStr *smsgst;
	struct SMSGBuffStr *msglist;
	struct SMSGBuffStr *mpre = NULL;

	record_comm_msg('R', buf, dlen);
	smsgst = (struct SMSGStr *)buf;
	if(check_server_msg(smsgst, dlen) != 0)
	{
		write_log(MSG_INFO, "check_server_msg err %d\n", check_server_msg(smsgst, dlen));
		return;
	}

	COMM_flag = SDATA_HAD_RET;

	//查找SMSGList列表，删除上送队列
	if((smsgst->cmd == 0x80) || (smsgst->cmd == 0x81) || (smsgst->cmd == 0x85))
	{
		msglist = SMSGList;
		while(msglist != NULL)
		{
			if(   (msglist->sbuf[1]+0x80 == smsgst->cmd)
				&& (memcmp(&msglist->sbuf[8], smsgst->devid, 6) == 0)  )
			{
				if(mpre != NULL)
				{
					mpre->next = msglist->next;
				}
				else
				{
					SMSGList = msglist->next;
				}
				free(msglist);
				SMSG_num --;
				break;
			}
			mpre = msglist;
			msglist = msglist->next;
		}
	}
	if(dlen <= 16) return;  //该节点后台数据库中没有配置，没有版本信息，不需要处理信息

	switch(smsgst->cmd)
	{
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x85:
		{
			//系统时间对时
			check_set_systime(smsgst->timebuf);

			//集中器参数版本号校对
			if(ParaSet.Paravar != (u16)smsgst->data[0]*256 + smsgst->data[1])
			{
				ask_server_CtorPara();
			}
			//节点亮度配置参数版本号校对
			if(smsgst->cmd != 0x82)
			{
				int vers;

				vers = get_node_para_version(smsgst->devid);

				if( (vers != (u16)smsgst->data[2]*256 + smsgst->data[3]) && (vers >= 0) )
				{
					usleep(500000);
					ask_server_NodePara(smsgst->devid);
				}
			}
		}
		break;

		case 0x83:   //集中器请求亮度突变值、房间标识配置表返回
			{
				int ret;

				ret = write_light_para(smsgst->devid, smsgst->data, dlen-(sizeof(struct SMSGStr) - sizeof(smsgst->data)));
				if(ret == 0)
				{
					//通知main进程亮度参数更新了
					write_log(MSG_INFO, "通知main进程亮度参数更新了。。。\n");
					memmove(smsgst->data+6, smsgst->data+2, dlen-2-(sizeof(struct SMSGStr) - sizeof(smsgst->data)) );
					smsgst->data[0] = smsgst->devid[0];
					smsgst->data[1] = smsgst->devid[1];
					smsgst->data[2] = smsgst->devid[2];
					smsgst->data[3] = smsgst->devid[3];
					smsgst->data[4] = smsgst->devid[4];
					smsgst->data[5] = smsgst->devid[5];
					SendPipeMsg(FDW_pipe, PIPE_LIGHTCG, smsgst->data, dlen+4-(sizeof(struct SMSGStr) - sizeof(smsgst->data)));
				}
			}
		break;

		case 0x84:   //请求节点地址表返回
			//if(memcmp(ParaSet.Devid, smsgst->devid, 6) == 0)
			{
				u8 tmp, i;
				FILE *fp;
				c8 sbuf[128];

				write_log(MSG_INFO, "get SMSG: total=%d, no=%d, len=%d\n", smsgst->ext >> 4, (smsgst->ext&0x0f),  dlen);
				//保存到临时数据区中
				if( (SNflag & (1UL<<(smsgst->ext&0x0f))) == 0) //防止数据包重发
				{
					if( (dlen-4-(sizeof(struct SMSGStr) - sizeof(smsgst->data)))/6  <= (MAXNODENUM-SNpt) )
					{
						memcpy(SNaddr[SNpt], smsgst->data+4, dlen - 4 - (sizeof(struct SMSGStr) - sizeof(smsgst->data)));
						SNpt += (dlen-4-(sizeof(struct SMSGStr) - sizeof(smsgst->data)))/6;
						SNflag |= (1UL<<(smsgst->ext&0x0f));
						write_log(MSG_INFO, "SNpt=%d SNflag = %04X\n", SNpt, SNflag);
					}
					else
					{
						write_log(MSG_INFO, "配置的节点数量太多 %04X %d, %d\n", SNflag, SNpt,  (dlen-4-(sizeof(struct SMSGStr) - sizeof(smsgst->data)))/6 );
					}
				}
				//检查是否所有帧都接收到
				tmp = (smsgst->ext >> 4) + 1;
				for(i=0; i<16; i++)
				{
					if( SNflag & (1UL<<i) )
					{
						tmp --;
						if(tmp == 0) //所有帧都接收到了
						{
							if(SNpt == (u16)smsgst->devid[4]*0x100 + smsgst->devid[5])  //配置的节点总数
							{
								//通知main进程参数更新了
								fp = fopen(NODE_tmplist, "wb+");
								{
									if(fp == NULL)
									{
										sleep(1);
										fp = fopen(NODE_tmplist, "wb+");
										{
											if(fp == NULL)
											{
												write_log(MSG_SYSERR, "create file %s error\n", NODE_tmplist);
												break;
											}
										}
									}
									fwrite(SNaddr[0], 1, SNpt*6, fp);
									fclose(fp);
									fp = NULL;
								}
								write_log(MSG_INFO, "通知main进程 节点参数更新了\n");
								SendPipeMsg(FDW_pipe, PIPE_NODECG, &smsgst->devid[4], 2);

								ParaSet.Paravar = (u16)smsgst->data[0]*0x100 + smsgst->data[1]; //配置版本号
								fp = fopen(PARASET_FILE, "rb+");
								if(fp == NULL)
								{
									write_log(MSG_INFO, "get PARA error %s\n",  PARASET_FILE);
									return;
								}
								while(fgets(sbuf, sizeof(sbuf), fp))
								{
									if(memcmp(sbuf, "ParaVar", 7) == 0)
									{
										sbuf[7] = ' ';
										sbuf[8] = '=';
										sbuf[9] = ' ';

										sprintf(sbuf+10, "%d        ", ParaSet.Paravar);
										sbuf[strlen(sbuf)] = ' ';  //去掉sprintf的字符串结束标志
										fseek(fp, 0-strlen(sbuf), SEEK_CUR);
										fputs(sbuf, fp);
									}
								}
								if(fp != NULL) fclose(fp);

								sprintf(sbuf, "Main %02X%02X%02X%02X%02X%02X var update %d", ParaSet.Devid[0], ParaSet.Devid[1], ParaSet.Devid[2], ParaSet.Devid[3], ParaSet.Devid[4], ParaSet.Devid[5], ParaSet.Paravar);
								system("rm /MeterRoot/LogFiles/Pg*");
								heart_server_msg(sbuf);
								sleep(1);  //等待main进程完成节点配置文件的修改
								listflag = 0;  //重新更新亮度配置版本
							}
							else
							{
								write_log(MSG_INFO, "SNpt=%d ext=%02X SNflag = %04X\n", SNpt, smsgst->ext, SNflag);

								write_log(MSG_INFO, "\n   !!!!SEVER SET PARA ERROR!!!!\n");
							}

							return;
						}
					}
				}
			}
		break;

		case 0x90:   //2.7	要求集中器执行指令
		{
			u8 cmd[128];
			u8 sbuf[512];
			u8 cs;
			int i;
			u16 pt;
			FILE *fp;

			//应答主站
			sbuf[0] = 0x68;
			sbuf[1]  = 0x10;
			memcpy(sbuf+2, ParaSet.Devid, 6);
			memcpy(sbuf+8, ParaSet.Devid, 6);
			Get_CurTime(sbuf+14, 5);
			sbuf[19] = 0;
			pt = 20;
			cs = 0;
			for(i=0; i<pt; i++)
			{
				cs += sbuf[i];
			}
			sbuf[pt++] = cs;
			sbuf[pt++] = 0x16;
			send_to_server(sbuf, pt);

			smsgst->data[dlen-15] = 0;
			//printf("recv smsg  %s \n", smsgst->data);

			if(memcmp(smsgst->data, "reboot", 6) == 0) //当配置了比较重要的参数或进行了集中器软件升级后，需要对集中器软件重启， 发送该命令给集中器，集中器收到该指令后，30秒进行重启。
			{
				system("killall main");
				sleep(3);
				system("reboot");
			}
			else if(memcmp(smsgst->data, "update", 6) == 0) //升级  做升级准备工作
			{
				Udtime = 0xffffffff;
				for(i=0; i<64; i++)
				{
					Uflag[i] = 0;
				}

				Maxframe = 10;
				fp = fopen("/tmp/upfile.tmp", "wb+");
				if(fp != NULL)
				{
					bzero(sbuf, 256);
					for(i=0; i<10; i++)
					{
						fwrite(sbuf, 1, 256, fp);
					}
					fclose(fp);
				}
			}
			else if(memcmp(smsgst->data, "downld", 6) == 0) //下载； 后带升级的文件路径和文件名
			{
				for(i=6; i<250; i++)
				{
					if(smsgst->data[i] == '"')
					{
						pt = 0;
						for(i+=1; i<250; i++)
						{
							if(smsgst->data[i] != '"')
							{
								dnlpwd[pt++] = smsgst->data[i];
							}
							else
							{
								dnlpwd[pt++] = 0;
								break;
							}
						}
						break;
					}
				}

				if(i < 250) //路径正确
				{
					pid_t status;
					int ret;
					int fnum;
					u16 count;
					struct stat fst;

					write_log(MSG_INFO, "get server msg dnlpwd %s\n", dnlpwd);
					sprintf(cmd, "tar -zcvf /tmp/dnl.tar.gz %s", dnlpwd);
					status = system(cmd);
					if( (status != -1) && (WIFEXITED(status)) && (WEXITSTATUS(status) == 0) )
					{
						write_log(MSG_INFO, "tar 成功...\n");
						fp = fopen("/tmp/dnl.tar.gz", "rb");
						if(fp == NULL)
						{
							write_log(MSG_INFO, " tar file error\n");
							break;
						}
						if(stat("/tmp/dnl.tar.gz", &fst) >= 0)
						{
							fnum = 	fst.st_size / 490;
							if((fst.st_size % 490) != 0)	fnum ++;
						}
						else
						{
							write_log(MSG_INFO, "stat dnl.tar.gz file error\n");
							if(fp != NULL) fclose(fp);
							break;
						}

						count = 0;
						do
						{
							write_log(MSG_INFO, "downld %d -- %d\n", fnum, count);

							ret = fread(sbuf+22, 1, 490, fp);
							if(ret > 0)
							{
								if(ret != 490)  sleep(3);

								sbuf[0] = 0x68;
								sbuf[1]  = 0x12;
								memcpy(sbuf+2, ParaSet.Devid, 6);
								memcpy(sbuf+8, ParaSet.Devid, 6);
								sbuf[12] = fnum / 256;
								sbuf[13] = fnum % 256;
								Get_CurTime(sbuf+14, 6);
								sbuf[20] = count / 256;
								sbuf[21] = count % 256;
								count ++;

								pt = 22 + ret;
								cs = 0;
								for(i=0; i<pt; i++)
								{
									cs += sbuf[i];
								}
								sbuf[pt++] = cs;
								sbuf[pt++] = 0x16;
								send_to_server(sbuf, pt);
								Scommfail_count = 0;  //该命令不需要应答
								usleep(120000);
								gave_soft_dog("comm");
							}
						}while(ret == 490);
						if(fp != NULL) fclose(fp);
					}
					else
					{
						write_log(MSG_INFO, "tar 失败...\n");
					}
				}
			}
		}
		break;

		case 0x92:   //后台缺失接收的数据包   6892 0101ca010024 0f0a0b0b0100 8b16
		{
			u8 fc;
			int ret;
			int fnum;
			u16 count;
			struct stat fst;
			u8 sbuf[512];
			u8 cs;
			int i;
			u16 pt;
			FILE *fp;

			//printf(".......Recv server 92cmd dlen = %d........\n", dlen);

			if(dlen < 18) return;

			//printf("补发数据包给后台...\n");

			fp = fopen("/tmp/dnl.tar.gz", "rb");
			if(fp == NULL)
			{
				write_log(MSG_INFO, "open tar file error\n");
				break;
			}
			if(stat("/tmp/dnl.tar.gz", &fst) >= 0)
			{
				fnum = 	fst.st_size / 490;
				if((fst.st_size % 490) != 0)	fnum ++;
			}
			else
			{
				write_log(MSG_INFO, "stat dnl.tar.gz file error\n");
				if(fp != NULL) fclose(fp);
				break;
			}

			for(fc=0; fc<(dlen-16)/2; fc++)
			{
				count = (u16)smsgst->data[fc*2]*256 + smsgst->data[fc*2+1];
				write_log(MSG_INFO, "downld %d -- %d\n", fnum, count);
				if(count == fnum - 1) break;

				fseek(fp, 490*count, SEEK_SET);
				ret = fread(sbuf+22, 1, 490, fp);
				if(ret > 0)
				{
					sbuf[0] = 0x68;
					sbuf[1]  = 0x12;
					memcpy(sbuf+2, ParaSet.Devid, 6);
					memcpy(sbuf+8, ParaSet.Devid, 6);
					sbuf[12] = fnum / 256;
					sbuf[13] = fnum % 256;
					Get_CurTime(sbuf+14, 6);
					sbuf[20] = count / 256;
					sbuf[21] = count % 256;
					count ++;
					pt = 22 + ret;
					cs = 0;
					for(i=0; i<pt; i++)
					{
						cs += sbuf[i];
					}
					sbuf[pt++] = cs;
					sbuf[pt++] = 0x16;
					send_to_server(sbuf, pt);
					Scommfail_count = 0;  //该命令不需要应答
					usleep(120000);
					gave_soft_dog("comm");
				}
			}

			//补发最后一帧数据
			{
				sleep(3);
				count = fnum-1;
				fseek(fp, 490*(fnum-1), SEEK_SET);
				ret = fread(sbuf+22, 1, 490, fp);
				if(ret > 0)
				{
					sbuf[0] = 0x68;
					sbuf[1]  = 0x12;
					memcpy(sbuf+2, ParaSet.Devid, 6);
					memcpy(sbuf+8, ParaSet.Devid, 6);
					sbuf[12] = fnum / 256;
					sbuf[13] = fnum % 256;
					Get_CurTime(sbuf+14, 6);
					sbuf[20] = count / 256;
					sbuf[21] = count % 256;
					count ++;
					pt = 22 + ret;
					cs = 0;
					for(i=0; i<pt; i++)
					{
						cs += sbuf[i];
					}
					sbuf[pt++] = cs;
					sbuf[pt++] = 0x16;
					send_to_server(sbuf, pt);
					usleep(600000);
				}
			}

			if(fp != NULL) fclose(fp);
		}
		break;


		case 0x91:   //后台发送文件内容给集中器
		{
			int i;
			FILE *fp;
			FILE *fpw;
			u8 sbuf[512];
			u16 wlen;
			u8 cs;
			u16 pt;
			u16 fcount;  //帧序号

			Udtime = time(NULL);
			TotalFrame = (u16)smsgst->devid[4]*256+smsgst->devid[5];
			fcount = (u16)smsgst->data[0]*256+smsgst->data[1] + 1;
			write_log(MSG_INFO, "get SMSG: total=%d, no=%d, len=%d; Uflag: %08X %08X %08X %08X\n", TotalFrame,fcount, dlen, Uflag[3], Uflag[2], Uflag[1], Uflag[0]);
			//保存到临时数据区中 每512字节一组，前两字节用于保存这一帧的有效数据长度
			fp = fopen("/tmp/upfile.tmp", "rb+");
			if(fp != NULL)
			{
				if(Maxframe < fcount)
				{
					fseek(fp, 0, SEEK_END);
					bzero(sbuf, 256);
					for(i=0; i<10; i++)
					{
						fwrite(sbuf, 1, 256, fp);
					}
					Maxframe += 10;
				}

				Uflag[(fcount-1)/32] |= (1L << ((fcount-1)%32));
				fseek(fp, (fcount-1)*512, SEEK_SET);
				smsgst->data[0] = (dlen-18)/256;
				smsgst->data[1] = (dlen-18)%256;
				fwrite(smsgst->data, 1, dlen-16, fp);
				fclose(fp);
			}
			if(fcount == TotalFrame) //接收到最后一帧数据
			{
				Udtime = 0xffffffff;
				//应答主站
				sbuf[0] = 0x68;
				sbuf[1]  = 0x11;
				memcpy(sbuf+2, ParaSet.Devid, 6);
				memcpy(sbuf+8, ParaSet.Devid, 6);
				Get_CurTime(sbuf+14, 5);
				sbuf[19] = 0;
				pt = 20;

				//检查是否接收到所有升级帧
				for(i=0; i<fcount; i++)
				{
					if( (Uflag[i/32] & (1L<<(i%32))) == 0 )
					{
						sbuf[pt++] = i/256;
						sbuf[pt++] = i%256;
						if(pt >= 220) break;
					}
				}
				cs = 0;
				for(i=0; i<pt; i++)
				{
					cs += sbuf[i];
				}
				sbuf[pt++] = cs;
				sbuf[pt++] = 0x16;
				send_to_server(sbuf, pt);

				if(pt == 22) //所有升级帧都接收到
				{
					fpw = fopen("/tmp/upfile.7z", "wb+");
					fp = fopen("/tmp/upfile.tmp", "rb");
					if((fpw != NULL) && (fp != NULL))
					{
						for(i=0; i<fcount; i++)
						{
							fread(sbuf, 1, 512, fp);
							wlen = (u16)sbuf[0]*256 + sbuf[1];
							fwrite(sbuf+2, 1, wlen, fpw);
						}
					}
					if(fp != NULL) fclose(fp);
					if(fpw != NULL) fclose(fpw);
					unlink("/tmp/upfile.tmp");
					system("cd /tmp && /MeterRoot/TestTool/7zDec e /tmp/upfile.7z");
					sleep(2);
					terminal_update();
					//unlink("/tmp/upfile.7z");
				}
			}
		}
		break;

		case 0x86:   //服务器控制节点工作状态指令
		case 0x88:   //服务器控制节点开门指令
		{
			write_log(MSG_INFO, "集中器控制 0x%02X  0x%02X\n", smsgst->cmd, smsgst->data[0]);

			smsgst->data[6] = smsgst->data[0];
			smsgst->data[7] = smsgst->cmd;
			smsgst->data[0] = smsgst->devid[0];
			smsgst->data[1] = smsgst->devid[1];
			smsgst->data[2] = smsgst->devid[2];
			smsgst->data[3] = smsgst->devid[3];
			smsgst->data[4] = smsgst->devid[4];
			smsgst->data[5] = smsgst->devid[5];
			SendPipeMsg(FDW_pipe, PIPE_SETNODE, smsgst->data, 8);
		}

		break;
	}
}

void check_set_systime(u8 *tbuf)
{
	struct tm curtm;
	struct timeval tv, tv1;
	struct timezone tz, tz1;

	gettimeofday(&tv1, &tz1);
	curtm.tm_year = tbuf[0] + 100;
	curtm.tm_mon =  tbuf[1] - 1;
	curtm.tm_mday = tbuf[2];
	curtm.tm_hour = tbuf[3];
	curtm.tm_min =  tbuf[4];
	curtm.tm_sec =  tbuf[5];

	tv.tv_sec = mktime(&curtm);
	tv.tv_usec = 0;
	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;

	if(abs(tv.tv_sec - tv1.tv_sec) > 180)
	{
		settimeofday(&tv, &tz);   /* Set    sysmtem time */
		write_log(MSG_DEBUG, "SetSysTime: %04d-%02d-%02d %02d:%02d:%02d\n", curtm.tm_year-100, curtm.tm_mon+1, curtm.tm_mday, curtm.tm_hour, curtm.tm_min, curtm.tm_sec);

		system("/MeterRoot/TestTool/rtctest -w &");
		sleep(1);
	}
}


int check_server_msg(struct SMSGStr *smsgst, int dlen)
{
	u8 *pt;
	int i;
	u8 cs;
	u8 tail;
	u8 msgcs;

	if(dlen < SMSGHEADLEN+2) return 1;
	pt = (u8 *)smsgst;
	tail = pt[dlen-1];
	msgcs = pt[dlen-2];
	if(smsgst->head != 0x68) return 2;
	if(tail != 0x16) return 3;

	cs = 0;
	for(i=0; i<dlen-2; i++)
	{
		cs += pt[i];
	}
	if(cs != msgcs) return 4;

	return 0;
}


void heart_server_msg(c8 *msg)
{
	static u8 count = 0;
	static time_t htime = 0;
	u8 sbuf[256];
	u8 cs;
	int i;
	u8 pt;
	FILE *fp = NULL;

	if(msg == NULL)
	{
		if( (abs(time(NULL)-htime) < 300) && (abs(time(NULL)-COMMtt) < 20) ) return;
		COMMtt = time(NULL);
		htime  = COMMtt;
	}

	sbuf[0] = 0x68;
	sbuf[1]  = 0x02;
	memcpy(sbuf+2, ParaSet.Devid, 6);
	memcpy(sbuf+8, ParaSet.Devid, 6);
	Get_CurTime(sbuf+14, 5);
	sbuf[19] = 0;
	for(i=0; i<8; i++) sbuf[20+i] = '0';
	fp = fopen("/MeterRoot/CFGFiles/version", "rb");
	if(fp != NULL)
	{
		fread(sbuf+20, 1, 8, fp);
		fclose(fp);
	}

	if(msg == NULL)
	{
		sprintf(sbuf+28, " heart to server %03d", count++);
		pt = 28 + 20;
	}
	else
	{
		if(strlen(msg) > 150)  //附带字符串长度不超过150字节
		{
			msg[150] = 0;
		}
		sprintf(sbuf+28, " %s", msg);
		pt = 28 + 1 + strlen(msg);
	}

	cs = 0;
	for(i=0; i<pt; i++)
	{
		cs += sbuf[i];
	}
	sbuf[pt++] = cs;
	sbuf[pt++] = 0x16;
	send_to_server(sbuf, pt);
}


//请求集中器配置的节点地址表
void ask_server_CtorPara(void)
{
	static time_t lastt = 0;
	u8 sbuf[32];
	u8 cs;
	int i;

	if( abs(time(NULL)-lastt) < 300 ) return;
	lastt = time(NULL);

	sbuf[0] = 0x68;
	sbuf[1]  = 0x04;
	memcpy(sbuf+2, ParaSet.Devid, 6);
	memcpy(sbuf+8, ParaSet.Devid, 6);
	Get_CurTime(sbuf+14, 5);
	sbuf[19] = SID ++;
	cs = 0;
	for(i=0; i<SMSGHEADLEN+6; i++)
	{
		cs += sbuf[i];
	}
	sbuf[20] = cs;
	sbuf[21] = 0x16;
	send_to_server(sbuf, 22);

	SNpt = 0;
	SNflag = 0;
}

//请求节点的亮度参数
void ask_server_NodePara(u8 *nid)
{
	static time_t lastt = 0;
	u8 sbuf[32];
	u8 cs;
	int i;

	if( abs(time(NULL)-lastt) < 30 ) return;
	lastt = time(NULL);

	sbuf[0] = 0x68;
	sbuf[1]  = 0x03;
	memcpy(sbuf+2, ParaSet.Devid, 6);
	memcpy(sbuf+8, nid, 6);
	Get_CurTime(sbuf+14, 5);
	sbuf[19] = SID ++;
	cs = 0;
	for(i=0; i<SMSGHEADLEN+6; i++)
	{
		cs += sbuf[i];
	}
	sbuf[20] = cs;
	sbuf[21] = 0x16;
	send_to_server(sbuf, 22);
}

//通过节点ID查找其参数版本号
int get_node_para_version(u8 *devid)
{
	FILE *fp;
	int i;
	struct nodelist_st tnlst;

	if(listflag == 0)
	{
		fp = fopen(NODECFG_FILE, "rb");
		if(fp == NULL)
		{
			sleep(1);
			fp = fopen(NODECFG_FILE, "rb");
			{
				if(fp == NULL)
				{
					return -1;
				}
			}
		}

		for(i=0; i<MAXNODENUM; i++)
		{
			if(fseek(fp, i*sizeof(struct nodelist_st), SEEK_SET) < 0) break;
			if(fread((u8*)&tnlst, 1, 16, fp) != 16) break;
			memcpy(light_list[i], tnlst.naddr, 6);
			memcpy(light_list[i]+6, (u8*)&tnlst.lightver, 2);
		}
		memset(light_list[i], 0x00, 8);

		if(fp != NULL) fclose(fp);
		listflag = 0xaa;
	}

	for(i=0; i<MAXNODENUM; i++)
	{
		if(memcmp(devid, light_list[i], 6) == 0)
		{
			return (int)light_list[i][7]*0x100 + light_list[i][6];
		}

		if(is_all_xx(light_list[i], 0, 6) == 0)  //到了列表的尾
		{
			return -1;
		}
	}

	return -1;
}


int write_light_para(u8 *nodeid, u8 *dbuf, u8 len)
{
	struct nodelist_st ndcfg;
	FILE *fp;
	int i;
	u8 room[3];

	fp = fopen(NODECFG_FILE, "rb+");
	if(fp == NULL)
	{
		sleep(1);
		fp = fopen(NODECFG_FILE, "rb+");
		{
			if(fp == NULL)
			{
				write_log(MSG_INFO, "open NODECFG_FILE ERROR!!\n");
				return -1;
			}
		}
	}

	room[0] = dbuf[len-3];
	room[1] = dbuf[len-2];
	room[2] = dbuf[len-1];
	dbuf[len-3] = 0;
	dbuf[len-2] = 0;
	dbuf[len-1] = 0;
	dbuf[len  ] = 0;
	dbuf[len+1] = 0;

	for(i=0; i<MAXNODENUM; i++)
	{
		if(fseek(fp, i*sizeof(struct nodelist_st), SEEK_SET) < 0) break;
		if(fread((u8*)&ndcfg, 1, sizeof(struct nodelist_st), fp)< 0) break;

		if(memcmp(ndcfg.naddr, nodeid, 6) == 0)
		{
			if(ndcfg.lightver != (u16)dbuf[2]*0x100 + dbuf[3])
			{
				ndcfg.lightver = (u16)dbuf[2]*0x100 + dbuf[3];
				listflag = 0;
			}

			if(memcmp(ndcfg.roomid, room, 3) == 0)  //房号配置没变
			{
				if(memcmp(ndcfg.lightcfg, dbuf+4, len-7+5) == 0)  //只版本变化，数据没变化 (多写一条全0记录)
				{
					fseek(fp, i*sizeof(struct nodelist_st), SEEK_SET);
					fwrite((u8*)&ndcfg, 1, sizeof(struct nodelist_st), fp);
					fclose(fp);
					fp = NULL;
					write_log(MSG_INFO, "只版本变化， 数据没变化!!\n");
					return -1;
				}
			}

			{ //房号或亮度参数其中之一有变化
				listflag = 0;

				memcpy(ndcfg.roomid, room, 3);
				memcpy(ndcfg.lightcfg, dbuf+4, len-7+5);
				fseek(fp, i*sizeof(struct nodelist_st), SEEK_SET);
				fwrite((u8*)&ndcfg, 1, sizeof(struct nodelist_st), fp);
				fclose(fp);
				fp = NULL;
				return 0;
			}
		}
	}
	if(fp != NULL) fclose(fp);
	fp = NULL;

	write_log(MSG_INFO, "节点找不到!!\n");

	return -1;
}


void record_comm_msg(c8 type, u8 *buf, int len)
{
	c8 fname[64];
	c8 sbuf[1124] = {0};
	c8 cmds[256];
	s32 i;
	FILE *fp;
	time_t tt;
	struct tm *curtime;
	struct stat fst;

	sprintf(fname, "%sSmsgtmp", LogTpath);
	if(access(fname, F_OK) != 0)
	{
		fp = fopen(fname, "w+");
	}
	else
	{
		fp = fopen(fname, "r+");
	}
	if(fp == NULL) return;

	time(&tt);
	curtime = localtime(&tt);
	sprintf(sbuf, "%c%02d%02d-%02d%02d%02d ",type, curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
	for (i = 0; i < len; i++)
	{
		sprintf(sbuf, "%s%02X", sbuf, buf[i]);
	}
	sprintf(sbuf, "%s\n", sbuf);
	fseek(fp, 0, SEEK_END);
	fputs(sbuf,fp);
	if(fp != NULL) {fclose(fp); fp = NULL;}

	if(stat(fname, &fst) >= 0)
	{
		if(fst.st_size < MAXSVRFLEN/10)  //ram盘数据长度未满
			return;
	}


	//拷贝log文件到flash盘
	sprintf(fname, "%sSmsg", LogPath);
	if(access(fname, F_OK) != 0)
	{
	   memset(cmds, 0x00, sizeof(cmds));
	   sprintf(cmds, "mv %sSmsgtmp %sSmsg", LogTpath, LogPath);
	   system(cmds);
	}
	else
	{
	   memset(cmds,0x00,sizeof(cmds));
	   sprintf(cmds, "cat %sSmsgtmp>>%sSmsg && rm %sSmsgtmp", LogTpath, LogPath, LogTpath);
	   system(cmds);
	   sleep(1);
	}

	if( (stat(fname, &fst) >= 0) && (fst.st_size > MAXSVRFLEN) )// 文件已满，保存压缩文件
	{
	   for(i=0; i<10; i++)
	   {
			sprintf(fname, "%sSmsg.%d.tar.gz", LogPath, i);
			if(access(fname, F_OK) != 0)
			{
				break;
			}
		}
		if(i >= 10)   i = 0; //所有文件都存在 删除0文件

		memset(cmds,0x00,sizeof(cmds));
		sprintf(cmds, "cd %s && tar -czvf Smsg.%d.tar.gz Smsg && rm Smsg &", LogPath, i);
		system(cmds);
		sleep(1);

		i++;
		if(i >= 10) i = 0;
		memset(cmds,0x00,sizeof(cmds));
		sprintf(cmds, "rm %sSmsg.%d.tar.gz &", LogPath, i);
		system(cmds);
		sleep(1);
	}
}





/*

	//扫描flash文件，查找要存储的文件。
	fpw = NULL;
	for(i=0; i<10; i++)
	{
		sprintf(fname, "%sSmsg%d", LogPath, i);
		if(access(fname, F_OK) != 0)
		{
			if(i != 0) i--;  //取上一个文件，判断是否大于MAXSVRFLEN
			else i = 9;
			sprintf(fname, "%sSmsg%d", LogPath, i);
			if(stat(fname, &fst) >= 0)
			{
				if(fst.st_size < MAXSVRFLEN)    // 文件未满，可以继续写入
				{
					fpw = fopen(fname, "r+");
				}
				else
				{
					i++;
					if(i >= 10) i = 0;
					sprintf(fname, "%sSmsg%d", LogPath, i);
					if(access(fname, F_OK) == 0)
					{

						fp = fopen(fname, "w+");
					}


					fpw = fopen(fname, "w+");

					i++;
					if(i >= 10) i = 0;
					sprintf(fname, "rm %sSmsg%d", LogPath, i);
					system(fname);
				}
				break;
			}
		}
	}

	if(fpw == NULL)  //所有0～9文件都存在，由0开始。
	{
		sprintf(fname, "%sSmsg0", LogPath);
		fpw = fopen(fname, "w+");
		if(fpw != NULL)
		{
			sprintf(fname, "rm %sSmsg1", LogPath);
			system(fname);
		}
	}

	if(fpw == NULL) return;

	sprintf(fname, "%sSmsgtmp", LogTpath);
	fpr = fopen(fname, "r");
	flen = 0;
	if( (fpr != NULL) && (fpw != NULL))
	{
		flen = 1;
		fseek(fpw, 0, SEEK_END);
		while(fgets(sbuf, 1024, fpr))
		{
			fputs(sbuf, fpw);
		}
	}
	if(fpr != NULL) {fclose(fpr); fpr = NULL;}
	if(fpw != NULL) {fclose(fpw); fpw = NULL;}

	if(flen != 0)
	{
		sprintf(fname, "rm %sSmsgtmp", LogTpath);    //删除ram盘文件
		system(fname);
	}
}

*/

/*
  功能说明:    执行升级
  输入参数:    无
  输出参数:    无
  返回值说明:   无
  作者:
*/
#define DLUSBWORK_SH  "dlusbw.sh"
void terminal_update(void)
{
	s32 i;
	c8 scmd[128];

	//检查升级文件是否存在
	i = 0;
	sprintf(scmd, "/tmp/%s", DLUSBWORK_SH);
	while( (access(scmd, F_OK) != 0) && (i < 10) )
	{
		sleep(1);
		i++;
	}
	if(access(scmd, F_OK) != 0)
	{
		//如果不存在
		return;
	}

	write_log(MSG_INFO, "dlusbw.sh file is OK...\n");
	sprintf(scmd, "rm -rf /tmp/updata.OK");
	system(scmd);
	sleep(1);

	write_log(MSG_INFO, "doing DLUSBWORK_SH...\n");
	//执行脚本
	sprintf(scmd, "chmod 777 /tmp/%s && sh /tmp/%s", DLUSBWORK_SH, DLUSBWORK_SH);
	system(scmd);

	//等待脚本的完成
	i = 0;
	while( (access("/tmp/updata.OK", F_OK) != 0) && (i < 30) )
	{
		sleep(3);
		i++;
	}

	write_log(MSG_INFO, "done sh OK...\n");
}


void reask_update_frame(void)    //没接收到最后一帧数据升级数据包时处理
{
	u8 sbuf[256];
	u8 pt;
	u16 i;
	u8 cs;

	Udtime = 0xffffffff;
	//应答主站
	sbuf[0] = 0x68;
	sbuf[1]  = 0x11;
	memcpy(sbuf+2, ParaSet.Devid, 6);
	memcpy(sbuf+8, ParaSet.Devid, 6);
	Get_CurTime(sbuf+14, 5);
	sbuf[19] = 0;
	pt = 20;

	//检查是否接收到所有升级帧
	for(i=0; i<TotalFrame; i++)
	{
		if( (Uflag[i/32] & (1L<<(i%32))) == 0 )
		{
			sbuf[pt++] = i/256;
			sbuf[pt++] = i%256;
			if(pt >= 220) break;
		}
	}
	cs = 0;
	for(i=0; i<pt; i++)
	{
		cs += sbuf[i];
	}
	sbuf[pt++] = cs;
	sbuf[pt++] = 0x16;
	send_to_server(sbuf, pt);
}


void record_comm_flag(void)
{
	FILE *fp;
	time_t ctt = 0;
	static time_t tt = 0;
	static time_t ltt = 0;
	u8 buf[20];

	if( abs(time(0) - tt) < 120 )  return;

	ctt = time(0);
	if(RecvServerMsgT == 0)  RecvServerMsgT = time(NULL);
	if ( ((ctt - tt) > 240) || (ctt < tt) )  RecvServerMsgT = time(NULL);  //发生对时
	tt = ctt;
	if(ltt == RecvServerMsgT) return;
	ltt = RecvServerMsgT;	
	
	fp = fopen("/var/log/comming", "rb+");
	if(fp == NULL)
	{
		fp = fopen("/var/log/comming", "wb+");
		if(fp == NULL)
		{
			write_log(MSG_INFO, "create running file error\n");
			return;
		}
		
		bzero(buf, sizeof(buf));
		fwrite(buf, 1, 20, fp);	
		fseek(fp, 0, SEEK_SET);
	}

	fprintf(fp, "comm %ld\r\n", RecvServerMsgT);
	fclose(fp);
}
