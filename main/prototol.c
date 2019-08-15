#include "lewin.h"
#include "lora.h"

extern int FDW_pipe, FDR_pipe;
extern int Plc_fd;
extern struct parameter_set ParaSet;
extern struct nodelist_st NodeList[MAXNODENUM];
extern time_t IstimeSaveNodeList;
extern time_t Updatetimer;
extern u8 SID;
extern u8 Default_LCfg[45];


//15-12-2 因为节点使用mktime没有把月份减1， 出现错误， 所以这里进行修正；
void correct_time(u8 *time)
{
	u8 curt[8];
	u32 temp1, temp2;
	
	Get_CurTime(curt, 5);
	
	temp1 = curt[0]*360 + curt[1]*30 + curt[2];
	temp2 = time[0]*360 + time[1]*30 + time[2];
	
	if( (temp2 > temp1) && ((temp2-temp1) >= 24) && ((temp2-temp1) <= 36) )  //时间超前了一个月左右
	{
		if(time[1] > 1)  
		{
			time[1] = time[1] - 1;
		}
		else
		{
			time[1] = 12;
			time[0] = time[0] - 1;
		}
	}
	
	if(time[1] ==  0)  //月份错误
	{
		time[1] = 12;
		time[0] = time[0] - 1;
	}
}

//只考虑单个Fn的情况
u8 get_Fn(u8 *dt)
{
	u8 ret = 0;

	ret = dt[1] * 8;
	if(dt[0] & 0x01) ret += 1;
	else if(dt[0] & 0x02) ret += 2;
	else if(dt[0] & 0x04) ret += 3;
	else if(dt[0] & 0x08) ret += 4;
	else if(dt[0] & 0x10) ret += 5;
	else if(dt[0] & 0x20) ret += 6;
	else if(dt[0] & 0x40) ret += 7;
	else if(dt[0] & 0x80) ret += 8;

	return ret;
}

//rbuf为376.2的 usrData
//len   rbuf的长度
void process_A06F02(u8 *rbuf, u16 len)
{
	int ret;
	u8 pmsgbuf[256];
	u8 tbuf[6];

	write_comm(MSG_COMMT+MSG_DEBUG, "PRA06F02:", rbuf, len);
	ret = check_node_msg(rbuf, len);
	if(ret != 0)
	{
		write_log(MSG_INFO, "check_node_msg ERROR ret = %d\r\n", ret);
		return;
	}
	
	if(save_plc_record(rbuf, len, rbuf+1) != 0) return;  //当与上一次发送数据帧内容一样时，不发送到服务器	
	
	Get_CurTime(tbuf, 6); 
	rbuf[12] &= 0x0f;  //去掉月SID标志
	rbuf[13] &= 0x1f;  //去掉日SID标志

	switch(rbuf[10])    //上报原因
	{
		case 1:  //定时上报
		case 2:  //突变上报
		case 4:  //突变上报
		case 8:  //突变上报
			pmsgbuf[0] = 0x68;
			if(rbuf[10] != 1) pmsgbuf[1] = 0x01;
			else pmsgbuf[1] = 0x00;
			memcpy(pmsgbuf+2, ParaSet.Devid, 6);  //集中器地址
			memcpy(pmsgbuf+8, rbuf+1, 6);  //节点地址
			memcpy(pmsgbuf+14, rbuf+11, 5); //开始时间
			correct_time(pmsgbuf+14);   //因为节点使用mktime没有把月份减1， 出现错误， 所以这里进行修正；
			pmsgbuf[19] = 2;               //间隔
			memcpy(pmsgbuf+20, rbuf+17, rbuf[16]*2);  //数据内容
			pmsgbuf[20+rbuf[16]*2] = calc_bcc(pmsgbuf, 20+rbuf[16]*2);
			pmsgbuf[21+rbuf[16]*2] = 0x16;

			//发送数据到后台服务器
			SendPipeMsg(FDW_pipe, PIPE_UPDATE, pmsgbuf, 22+rbuf[16]*2);
		break;
		
		default:
			if(rbuf[10] & 0x30) //插拔卡主动上报
			{
				pmsgbuf[0] = 0x68;
				pmsgbuf[1] = 0x05;
				memcpy(pmsgbuf+2, ParaSet.Devid, 6);  //集中器地址
				memcpy(pmsgbuf+8, rbuf+1, 6);         //节点地址
				memcpy(pmsgbuf+14, rbuf+11, 6);       //开始时间
				memcpy(pmsgbuf+20, rbuf+17, 16);      //数据内容
				if(rbuf[10]&0x10)  pmsgbuf[36] = 0xAA; //插拔卡标志
				else pmsgbuf[36] = 0;
				memcpy(pmsgbuf+37, rbuf+33, 5);      //卡ID（5字节）			
				pmsgbuf[42] = calc_bcc(pmsgbuf, 42);
				pmsgbuf[43] = 0x16;
				//发送插拔卡信息数据到后台服务器
				SendPipeMsg(FDW_pipe, PIPE_UPDATE, pmsgbuf, 44);
	
				//当合并上送时，发送异常数据到后台服务器
				if(len >= 48)
				{
					pmsgbuf[0] = 0x68;
					pmsgbuf[1] = 0x01;
					memcpy(pmsgbuf+2, ParaSet.Devid, 6);  //集中器地址
					memcpy(pmsgbuf+8, rbuf+1, 6);   //节点地址
					rbuf[39] &= 0x0f;  //去掉月SID标志
					rbuf[40] &= 0x1f;  //去掉日SID标志
					memcpy(pmsgbuf+14, rbuf+38, 5); //开始时间
					pmsgbuf[19] = 2;                //间隔
					memcpy(pmsgbuf+20, rbuf+44, rbuf[43]*2);  //数据内容
					pmsgbuf[20+rbuf[43]*2] = calc_bcc(pmsgbuf, 20+rbuf[43]*2);
					pmsgbuf[21+rbuf[43]*2] = 0x16;
	
					usleep(500000);
					//发送异常数据到后台服务器
					SendPipeMsg(FDW_pipe, PIPE_UPDATE, pmsgbuf, 22+rbuf[43]*2);
				}
			}
			else if(rbuf[10] & 0x0F)  //两个异常同时发生
			{
				pmsgbuf[0] = 0x68;
				if(rbuf[10] & 0x0E) pmsgbuf[1] = 0x01;
				else pmsgbuf[1] = 0x0;
				memcpy(pmsgbuf+2, ParaSet.Devid, 6);  //集中器地址
				memcpy(pmsgbuf+8, rbuf+1, 6);  //节点地址
				memcpy(pmsgbuf+14, rbuf+11, 5); //开始时间
				pmsgbuf[19] = 2;               //间隔
				memcpy(pmsgbuf+20, rbuf+17, rbuf[16]*2);  //数据内容
				pmsgbuf[20+rbuf[16]*2] = calc_bcc(pmsgbuf, 20+rbuf[16]*2);
				pmsgbuf[21+rbuf[16]*2] = 0x16;

				//发送数据到后台服务器
				SendPipeMsg(FDW_pipe, PIPE_UPDATE, pmsgbuf, 22+rbuf[16]*2);
			}
			else
			{
				write_log(MSG_INFO, "recv err node cmd = %02X\n", rbuf[10]);
			}
		break;
	}
}


//检查解析出来的GB645数据帧格式
u8 check_node_msg(u8 *rbuf, u16 len)
{
	if(rbuf[0] != 0x68) return 1;
	if(rbuf[7] != 0x68) return 2;
	if(len != rbuf[9]+12) return 3;  //rbuf[9]有效数据长度
	if(check_bcc(rbuf, len-1) != 0) return 4;
	if(rbuf[len-1] != 0x16) return 5;

	return 0;
}


void pipe_msg_process(void)
{
	u8 mbuf[256];
	int rlen;
	u16 MsgType = 0;

	rlen = read_pipe_msg(FDR_pipe, mbuf);
	if(rlen <= 0) return;

	MsgType = (u16)mbuf[TYPEPOS+1]*0x100 + mbuf[TYPEPOS];
	switch(MsgType)
	{
		case PIPE_LIGHTCG:  //节点时间亮度变化参数改变
			//用poll的方式将参数发到节点
			//现在不需要了
			//process_msg_lightcg(mbuf+DATAPOS, mbuf[LENPOS]);
			break;
			
		case PIPE_SETNODE:  //控制节点工作状态指令  //服务器控制节点开门指令
			send_setmode_msg(mbuf+DATAPOS, mbuf[DATAPOS+6], mbuf[DATAPOS+7]);
			break;

		case PIPE_NODECG:  //节点配置信息变化
			process_msg_nodecg(mbuf[DATAPOS]*0x100 + mbuf[DATAPOS+1]);
			break;

		case PIPE_REBOOT:  //wdog通知，集中器即将重启
			main_reboot_done();
			break;
		default:
			break;
	}
}

void process_msg_lightcg(u8 *msg, u8 len)
{
	int i;

	if(len < 8) return;

	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //到了列表的尾
		{
			break;
		}

		if(memcmp(NodeList[i].naddr, msg, 6) == 0)
		{
			msg[len] = 0;
			msg[len+1] = 0;
			msg[len+2] = 0;
			msg[len+3] = 0;
			msg[len+4] = 0;

			NodeList[i].pollt = 0x11;
			NodeList[i].lightver = (u16)msg[6]*0x100 + msg[7];
			memcpy(NodeList[i].lightcfg, msg+8, len-8+5);

			send_Poll_msg(i);

			break;
		}
	}
}



void process_msg_nodecg(u16 cfgnum)
{
	u8 SNaddr[MAXNODENUM][6] = {{0}};
	FILE *fp;   //新节点地址配置文件
	u16 i, j;

	write_log(MSG_INFO, "process_msg_nodecg %d\n", cfgnum);

	if(cfgnum > MAXNODENUM)
	{
		write_log(MSG_INFO, "cfg node num=%d > MAXNODENUM !!!\n", cfgnum);
	}

	fp = fopen(NODE_tmplist, "rb");
	{
		if(fp == NULL)
		{
			sleep(1);
			fp = fopen(NODE_tmplist, "rb");
			{
				if(fp == NULL)
				{
					write_log(MSG_SYSERR, "create file %s error\n", NODE_tmplist);
					return;
				}
			}
		}
	}
	fread(SNaddr[0], 1, cfgnum*6, fp);
	fclose(fp);
	fp = NULL;

	//查找是否有删除的节点
	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //到了列表的尾
		{
			break;
		}

		for(j=0; j<cfgnum; j++)
		{
			if(memcmp(NodeList[i].naddr, SNaddr[j], 6) == 0)
			{
				break;
			}
		}

		if(j >= cfgnum) //节点在新配置的表中没有， 给删除了
		{
			c8 nfile[64];

			sprintf(nfile, "%s%02x%02x%02x%02x%02x%02x", NodeDataPath, NodeList[i].naddr[0], NodeList[i].naddr[1],NodeList[i].naddr[2], NodeList[i].naddr[3],NodeList[i].naddr[4], NodeList[i].naddr[5]);
			unlink(nfile);
			sprintf(nfile, "%s%02x%02x%02x%02x%02x%02x", NodeDataTpath, NodeList[i].naddr[0], NodeList[i].naddr[1],NodeList[i].naddr[2], NodeList[i].naddr[3],NodeList[i].naddr[4], NodeList[i].naddr[5]);
			unlink(nfile);
			write_log(MSG_INFO, "del node %s\n", nfile);

			for(j=i; j<MAXNODENUM-1; j++)
			{
				memcpy( (u8*)&NodeList[j], (u8*)&NodeList[j+1], sizeof(struct nodelist_st) );
				if(is_all_xx(NodeList[j].naddr, 0, 6) == 0)  //到了列表的尾
				{
					break;
				}
			}
			i--;
		}
	}

	//查找是否有新增的节点
	for(i=0; i<cfgnum; i++)
	{
		for(j=0; j<MAXNODENUM; j++)
		{
			if(memcmp(NodeList[j].naddr, SNaddr[i], 6) == 0)
			{
				break;
			}

			if(is_all_xx(NodeList[j].naddr, 0, 6) == 0)  //到了列表的尾
			{
				//有新增加的节点
				memset( (u8*)&NodeList[j], 0, sizeof(struct nodelist_st)*2 );
				memcpy(NodeList[j].naddr, SNaddr[i], 6);
				memcpy(NodeList[j].lightcfg, Default_LCfg, sizeof(Default_LCfg));

				write_comm(MSG_COMMR+MSG_INFO, "add node: ", SNaddr[i], 6);
				//get_rush_poling(1, NULL); //现在轮询很快了，不用这一步
				break;
			}
		}
	}

	fp = fopen(NODECFG_FILE, "wb+");
	{
		if(fp == NULL)
		{
			sleep(1);
			fp = fopen(NODECFG_FILE, "wb+");
			{
				if(fp == NULL)
				{
					write_log(MSG_SYSERR, "create file %s error\n", NODECFG_FILE);
					return;
				}
			}
		}
	}

	write_log(MSG_INFO, "process_msg_nodecg disp nlist:\n");
	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //到了列表的尾
		{
			break;
		}
		fwrite((u8*)&NodeList[i], 1, sizeof(struct nodelist_st), fp);

		write_log(MSG_INFO, "node%03d: %02X %02X %02X %02X %02X %02X\r\n", i, NodeList[i].naddr[0], NodeList[i].naddr[1], NodeList[i].naddr[2], NodeList[i].naddr[3], NodeList[i].naddr[4], NodeList[i].naddr[5]);
	}

	fclose(fp);
	fp = NULL;
}


void update_node_cfgfile(void)
{
	u8 buf[512];
	FILE *fp; 
	int i;
	
	fp = fopen(NODECFG_FILE, "rb+");
	{
		if(fp == NULL)
		{
			sleep(1);
			fp = fopen(NODECFG_FILE, "rb+");
			{
				if(fp == NULL)
				{
					write_log(MSG_SYSERR, "create file %s error\n", NODECFG_FILE);
					return;
				}
			}
		}
	}

	write_log(MSG_INFO, "update nodecg...\n");
	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //到了列表的尾
		{
			break;
		}
		fread(buf, 1, sizeof(struct nodelist_st), fp);
		if( memcmp(NodeList[i].naddr, buf, sizeof(struct nodelist_st)) != 0)
		{
			fseek(fp, -sizeof(struct nodelist_st), SEEK_CUR);
			fwrite((u8*)&NodeList[i], 1, sizeof(struct nodelist_st), fp);
		}
	}

	fclose(fp);
	fp = NULL;
}


int save_plc_record(u8 *rbuf, u16 len, u8 *addr)
{
	c8 fname[64];
	c8 sbuf[1024] = {0};
	int flen;
	s32 i;
	FILE *fp, *fpw, *fpr;
	time_t tt;
	struct tm *curtime;
	struct stat fst;
	int ret = 0;	

	if(access(LogTpath, F_OK) != 0)
	{
		sprintf(fname, "mkdir %s", LogTpath);
		system(fname);
	}

	sprintf(fname, "%sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5]);
	if(access(fname, F_OK) != 0)
	{
		fp = fopen(fname, "w+");
	}
	else
	{
		fp = fopen(fname, "r+");
	}
	if(fp == NULL) return ret;
	
	
	{ //判断是否跟之前的数据帧重复	
		u8 cbuf[32];
		u8 dbuf[8];
		
		fseek(fp, -13, SEEK_END);
		fread(cbuf, 1, 13, fp);
		cbuf[14] = 0;		
		strtohex(dbuf, cbuf);
		
		//printf("%s   ", cbuf);
		//display("save_plc: ", dbuf, 8);
		//display("save_plc: ", rbuf+len-8, 8);
		
		if( (dbuf[0] == rbuf[len-6]) && (dbuf[1] == rbuf[len-5])  && (dbuf[2] == rbuf[len-4]) && (dbuf[3] == rbuf[len-3]) && (dbuf[4] == rbuf[len-2]))
		{
			ret = 1;
			write_log(MSG_INFO, "msg is repetition...\n");   //节点载波数据重复上报， 不发送到主站
		}		
	}

	time(&tt);
	curtime = localtime(&tt);
	sprintf(sbuf, "%02d%02d-%02d%02d%02d ", curtime->tm_mon + 1, curtime->tm_mday, curtime->tm_hour, curtime->tm_min, curtime->tm_sec);
	for (i = 0; i < len; i++)   ////for (i = -29; i < len+2; i++)   //调试， 记录整个376.2报文
	{
		sprintf(sbuf, "%s%02X", sbuf, rbuf[i]);
	}
	sprintf(sbuf, "%s\n", sbuf);
	fseek(fp, 0, SEEK_END);
	fputs(sbuf,fp);
	if(fp != NULL) {fclose(fp); fp = NULL;}

	//判断ram文件是否大于10%MAXPLCFLEN(30K)， 大于时存到flash盘中。
	if(stat(fname, &fst) >= 0)
	{
		if(fst.st_size < MAXPLCFLEN/10) // 文件未满
			return ret;
	}

	if(access(LogPath, F_OK) != 0)
	{
		sprintf(fname, "mkdir %s", LogPath);
		system(fname);
	}

	//再判断flash盘中的文件是否大于MAXPLCFLEN(300K)， 大于时，删除前面的50%MAXPLCFLEN(30K)
	sprintf(fname, "%sPg%02X%02X%02X", LogPath, addr[3], addr[4], addr[5]);
	if(access(fname, F_OK) != 0)
	{
		sprintf(fname, "mv %sPg%02X%02X%02X %sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5], LogPath, addr[3], addr[4], addr[5]);
		system(fname);
		return ret;
	}

	if( (stat(fname, &fst) >= 0) && (fst.st_size > MAXPLCFLEN) )//flash盘中的文件大于300K
	{
		fp = fopen(fname, "r");
		fpw = fopen(LogPathtmp, "w+");
		sprintf(fname, "%sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5]);
		fpr = fopen(fname, "r");

		flen = 0;
		if( (fp != NULL) && (fpr != NULL) && (fpw != NULL) )
		{
			while( (flen < MAXPLCFLEN/2) && fgets(sbuf, 1024, fp))
			{
				flen += strlen(sbuf);
			}

			while(fgets(sbuf, 1024, fp))
			{
				fputs(sbuf, fpw);
			}

			if(fpr != NULL)
			{
				while(fgets(sbuf, 1024, fpr))
				{
					fputs(sbuf, fpw);
				}
			}
		}

		if(fp != NULL) {fclose(fp); fp = NULL;}
		if(fpr != NULL) {fclose(fpr); fpr = NULL;}
		if(fpw != NULL) {fclose(fpw); fpw = NULL;}

		if(flen != 0)
		{
			sprintf(fname, "mv %s %sPg%02X%02X%02X", LogPathtmp, LogPath, addr[3], addr[4], addr[5]);
			system(fname);
			sprintf(fname, "rm %sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5]);  //删除ram盘文件
			system(fname);
		}
	}
	else
	{
		fpw = fopen(fname, "r+");
		sprintf(fname, "%sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5]);
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
			sprintf(fname, "rm %sPg%02X%02X%02X", LogTpath, addr[3], addr[4], addr[5]);  //删除ram盘文件
			system(fname);
		}
	}
	
	return ret;
}

