
#include "public.h"
#include "namepipe.h"


extern u8 nDebugLevel;



//-------------------------公共函数-------------------------------------

//****************有名管道公共函数******************
/*
 * 函数名  : Init_NamePipe
 * 函数描述: 创建命名管道
 * 输入参数: type：PIPE_MtoC: Main模块写Comm模块读, PIPE_CtoM: Comm模块写Main模块读
 * 输出    : void
 * 返回值  : 成功0 失败-1
 */

s32 Init_NamePipe(u8 type, int *fdr, int *fdw)
{
    int ret = 0;
	if(access(PATH_C2M_PIPE, 0) != 0)
	{
		ret = mkfifo(PATH_C2M_PIPE, 0666);
	}
	if(access(PATH_M2C_PIPE, 0) != 0)
	{
		ret = mkfifo(PATH_M2C_PIPE, 0666);
	}
    if(ret)
        printf("ret %d\n",ret);
	sleep(1);


	if(type == PIPE_MtoC)
	{
		*fdr = open(PATH_C2M_PIPE, O_RDONLY | O_NONBLOCK, 0);
		write_log(MSG_INFO, "fdr = %d\n", *fdr);

		*fdw = open(PATH_M2C_PIPE, O_WRONLY, 0);
		write_log(MSG_INFO, "fdw = %d\n", *fdw);
	}
	else
	{
		*fdr = open(PATH_M2C_PIPE, O_RDONLY | O_NONBLOCK, 0);
		write_log(MSG_INFO, "fdr = %d\n", *fdr);

		*fdw = open(PATH_C2M_PIPE, O_WRONLY, 0);
		write_log(MSG_INFO, "fdw = %d\n", *fdw);
	}
	if ( (*fdr > 0) && (*fdw > 0) )
	{
		write_log(MSG_INFO, "\n init name pipe OK fdr=%d  fdw=%d \n", *fdr, *fdw);
		return 0;
	}

	write_log(MSG_INFO, "\n !!!!!  init name pipe ERROR fdr=%d  fdw=%d !!!! \n", *fdr, *fdw);
	return -1;
}


/*
s32 Init_NamePipe(u8 type, int *fdr, int *fdw)
{
	u8 retrytimes = 3;

	if(type == PIPE_CtoM)
	{
		unlink(PATH_M2C_PIPE);
		mkfifo(PATH_M2C_PIPE, 0666);
		*fdr = open(PATH_M2C_PIPE, O_RDONLY | O_NONBLOCK);
	}
	else
	{
		*fdw = open(PATH_M2C_PIPE, O_WRONLY);
	}

	if ( (*fdr > 0) && (*fdw > 0) )
	{
		return 0;
	}

	write_log(MSG_SYSERR, "init the pipe error %d %d\n", *fdr, *fdw);
	return -1;
}
*/


//接收命名管道信息
/*
 * 函数名  : MsgRecv_Pipe
 * 函数描述: 接收命名管道消息
 * 输入参数: fd，消息结构
 * 输出    : msgbuf：消息接收区， more：是否还有消息
 * 返回值  : 消息长度， 或者R_FAIL失败、NO_MSG:没有消息
 */
s32 MsgRecv_Pipe(s32 fd, struct TSockBuff* TTmpBuff, u8 *msgbuf, u8 *more)
{
	s32 iLen = -1;
	u16 SLen, SNext;
	u8 buf[256] = {0x00};
	static u8 noMsgCount=0;

	bzero(buf, sizeof(buf));
	if( ((*(unsigned short *)(TTmpBuff->Buff)) != 0x1B1B) && (TTmpBuff->IIndex != 0) )
		bzero(TTmpBuff, sizeof(struct TSockBuff));

	if( *more == 0 )
	{
		iLen = read(fd, buf, 256);
		if(iLen < 0){
			return R_FAIL;
		}
		if(iLen == 0)
		{
			noMsgCount++;
			if(noMsgCount > 5){
				noMsgCount = 0;
				return R_PIPECLOSED;
			}
			return NO_MSG;
		}
		noMsgCount = 0;

		if((*((unsigned short *)buf) == 0x1B1B) && (TTmpBuff->IIndex != 0))  //新信息
			bzero(TTmpBuff, sizeof(struct TSockBuff));
		if( iLen > (sizeof(TTmpBuff->Buff) - TTmpBuff->IIndex) )   //buf不够
		{
			memcpy(&(TTmpBuff->Buff[TTmpBuff->IIndex]), buf, (sizeof(TTmpBuff->Buff)-TTmpBuff->IIndex));			
			TTmpBuff->IIndex = sizeof(TTmpBuff->Buff);
			//SMyIndex = sizeof(TTmpBuff->Buff);
			//memcpy(&(TTmpBuff->IIndex), &SMyIndex, sizeof(unsigned short));
			write_log(MSG_INFO, "DC:No enough buffer...\n");
		}
		else
		{
			memcpy( &(TTmpBuff->Buff[TTmpBuff->IIndex]), buf, iLen);
			TTmpBuff->IIndex = TTmpBuff->IIndex + iLen;
			
			//SMyIndex = TTmpBuff->IIndex;
			//SMyIndex += (unsigned short)(iLen & 0xFFFF);
			//memcpy(&(TTmpBuff->IIndex), &SMyIndex, sizeof(unsigned short));
		}
		if(TTmpBuff->IIndex < LENPOS)  //No Receive the Length of Message
			return NO_MSG;
		if(TTmpBuff->IIndex < (TTmpBuff->Buff[LENPOS]+MIN_MSG_SIZE) ) // No Receive the Entire Message
			return NO_MSG;
	}
	//else   //*more!=0
	//	SMyIndex = TTmpBuff->IIndex;

	bzero(buf, sizeof(buf));
	if((*(unsigned short *)(TTmpBuff->Buff)) != 0x1B1B)   //信息头错
	{
		bzero(TTmpBuff, sizeof(struct TSockBuff));
		*more = 0;
		return NO_MSG;
	}

	SLen = TTmpBuff->Buff[LENPOS] + MIN_MSG_SIZE;
	if(SLen > MAX_MSG_SIZE)         //信息超长
	{
		bzero(TTmpBuff, sizeof(struct TSockBuff));
		*more = 0;
		
		write_log(MSG_INFO, "DC:Msgbuf Overflow...");
		return R_FAIL;
	}
	if((*(unsigned short *)(&(TTmpBuff->Buff[SLen-2]))) != 0xB1B1)   //信息尾错
	{
		bzero(TTmpBuff, sizeof(struct TSockBuff));
		*more = 0;
		return NO_MSG;
	}

	memcpy(msgbuf, TTmpBuff->Buff, SLen);     //通过msgbuf返回一条完整消息
	
	TTmpBuff->IIndex -= SLen;
	memmove(TTmpBuff->Buff, TTmpBuff->Buff + SLen, TTmpBuff->IIndex);
	
	if(TTmpBuff->IIndex >= MIN_MSG_SIZE)
	{
		SNext = TTmpBuff->Buff[LENPOS] + MIN_MSG_SIZE;
		if(SNext > TTmpBuff->IIndex)
			*more = 0;
		else
			*more = 1;
	}
	else
		*more = 0;

	return (s32)(SLen);
}


/*
 * 函数名  : read_pipe_msg
 * 函数描述: 读管道消息，并处理，没有消息不处理
 * 输入参数:
 * 输出    :
 * 返回值  :  0
 */
s32 read_pipe_msg(int fd, u8 *msgbuf)
{
	static u8 pipedataflag = 0;
	static struct TSockBuff sPipeBuf;
	s32 MsgLen = 0;

	if (fd > 0)
	{
		MsgLen = MsgRecv_Pipe(fd, &sPipeBuf, msgbuf, &pipedataflag);
		if (MsgLen <= 0)
		{
		//RUN_TIME_START(300)
		//  write_log(MSG_RUNING, "read meter msg len %d err\n", MsgLen);
		//RUN_TIME_END()
			return 0;
		}

		write_comm(MSG_COMMR+MSG_DEBUG, "pipe recv msg:", msgbuf, MsgLen);

		if (MsgCheck(msgbuf, MsgLen) == -1)
		{
		//RUN_TIME_START(300)
			write_log(MSG_SYSERR, "pipe message check fail\n");
		//RUN_TIME_END()
			return -1;
		}

		//write_comm(MSG_COMMR+MSG_DEBUG, "pipe recv msg:", msgbuf+DATAPOS, msgbuf[LENPOS]);

		return MsgLen;
	}
	else
	{
		pipedataflag = 0;
		RUN_TIME_START(300)
		write_log(MSG_SYSERR, "pipe is error...\n");
		RUN_TIME_END()
		return -2;
	}
}



/*
 * 函数名  : SendPipeMsg
 * 函数描述: 发送有名管道信息
 * 输入参数: fd, type:发送类型(PMP_)， AttachData：附带的数据，dlen：数据的长度
 * 输出    : `<...>`
 * 返回值  : `<...>`
 */
void SendPipeMsg(s32 fd, u16 type, u8 *AttachData, u8 dlen)
{
	s32 i;
	struct TIPCMsg TMyMsg;
	struct flock lock, mylock;
	u8 tmpbuf[256] = {0x00}, CLen, checksum;

	bzero(&TMyMsg, sizeof(struct TIPCMsg));
	bzero(tmpbuf, sizeof(tmpbuf));

	TMyMsg.MsgHead = MSG_HEAD;
	TMyMsg.MsgSrc = 0;
	TMyMsg.MsgDst = 0;
	TMyMsg.MsgType = type;
	TMyMsg.DataLen = dlen;
	TMyMsg.PAttachData = AttachData;
	TMyMsg.MsgTail = MSG_TAIL;

	CLen = sizeof(TMyMsg) - 4 - sizeof(u8) - sizeof(u16);  //需计算校验的长度
	memcpy(tmpbuf, (u8 *)&TMyMsg, CLen);
	if(AttachData != NULL) memcpy(&tmpbuf[CLen], AttachData, dlen);
	CLen += dlen;
	checksum = 0;
	for (i=0; i<CLen; i++) checksum += tmpbuf[i];
	tmpbuf[CLen] = checksum;
	CLen ++;
	memcpy(&tmpbuf[CLen], &TMyMsg.MsgTail, sizeof(u16));
	CLen += sizeof(u16);
	mylock.l_type = F_WRLCK;
	mylock.l_start = 0;
	mylock.l_whence = SEEK_SET;
	mylock.l_len = 0;
	mylock.l_pid = getpid();

	i = 3;
	while(i)
	{
		lock = mylock;
		if( fcntl(fd, F_SETLK, &lock) == 0 )
		{
			if(write(fd, tmpbuf, CLen) != CLen)
			{
				write_comm(MSG_COMMR+MSG_INFO, "pipe send msg failed:", tmpbuf, CLen);
			}
			lock = mylock;
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);

			write_comm(MSG_COMMR+MSG_DEBUG, "pipe send msg:", AttachData, dlen);
			break;
		}
		else{
			i--;
			sleep(1);
		}
	}
}



//功能说明  检查消息结构，消息校验位是否合法；
//参数说明  MsgBuf所在地址， MsgLen消息所有数据的长度
//返回值说明    成功返回0，失败返回-1；
s32 MsgCheck(u8 *MsgBuf, u8 MsgLen)
{
	int i;
	u8 len, checksum;
	u16 msg_head, msg_tail;

	memcpy(&msg_head, MsgBuf, sizeof(u16));
	if (msg_head != MSG_HEAD) return R_FAIL;

	len = MsgLen - sizeof(u8) - sizeof(u16);
	checksum = 0;
	for (i = 0; i < len; i++) checksum += MsgBuf[i];
	if (checksum != MsgBuf[CHECKPOS+MsgBuf[LENPOS]]) return R_FAIL;

	memcpy(&msg_tail, &MsgBuf[MsgLen - sizeof(u16)], sizeof(u16));
	if (msg_tail != MSG_TAIL) return R_FAIL;

	return R_SUCC;
}

