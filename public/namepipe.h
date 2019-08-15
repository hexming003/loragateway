#ifndef DC_PUBLIC
#define DC_PUBLIC



/* head and tail */
#define MSG_HEAD    0x1B1B
#define MSG_TAIL    0xB1B1

#define MSG_CODE_COMM 1
#define MSG_CODE_MAIN21

//MsgType
//内部通信(pipe)描述：
#define PIPE_INIT   0x0000
#define PIPE_REBOOT  0xcc01  //WDT通知进程系统即将重启，需要自己保存数据
//  1、main ---> comm
#define PIPE_UPDATE    0xaa01  //要求通信模块上报节点的采集数据  数据内容：上层通信协议内容；  len 上报的协议长度，包括头尾等
#define PIPE_PAPAUP_OK 0xaa02  //节点参数更新成功
#define PIPE_SETNODE_OK 0xaa03  //节点远程设置成功

//  2、comm ---> main
#define PIPE_LIGHTCG 0xbb01  //通知main进程亮度参数更新了   数据内容：节点地址 + 亮度参数表
#define PIPE_NODECG  0xbb02  //通知main进程节点列表更新了   数据内容：该集中器配置的节点总数量  len = 2
#define PIPE_SETNODE 0xbb03  //通知main进程设置节点的工作模式  数据内容：节点地址+模式 7字节；00H： 正常插卡取电工作状态；33H： 网络控制通电状态 55H： 网络控制断电状态


#define TYPEPOS  4
#define LENPOS   6
#define DATAPOS 7
#define CHECKPOS 7

/* definition of IPC Message structure */
struct TIPCMsg
{
	u16  MsgHead;        //Head of Message
	u8   MsgSrc;         //Source of Message
	u8   MsgDst;         //Destination of Message
	u16  MsgType;        //Type of Message
	u8   DataLen;        //Length of Attached Data
	u8   *PAttachData;   //Attached Data
	u8   CheckSum;       //Checksum
	u16  MsgTail;        //Tail of Message
} __attribute__((packed));

struct TSockBuff
{
	u16 IIndex;
	u8 Buff[2048];
} __attribute__((packed));


//#define NO_MSG       0
//#define R_FAIL       -1
#define R_PIPECLOSED -2

#define MAX_PORTNUM 4

#define MIN_MSG_SIZE   10
#define MAX_MSG_SIZE   256
#define PIPE_MtoC     0x0
#define PIPE_CtoM     0x1

///运行t次,才动作一次
#define RUN_COUNT_START(t) {\
	static u16 cnt = (t);  \
	if( (cnt == (t)) || (nDebugLevel == 0xff) ) \
	{\
			cnt = 0;\

//动作包含完结
#define RUN_COUNT_END()  \
	}\
	cnt++;\
}\


///运行t秒,才动作一次
#define RUN_TIME_START(t) {\
	static time_t last = 0;  \
	time_t now; \
	time(&now);\
	if( (now - last >= (t)) || (nDebugLevel == 0xff) )  \
	{\
			last = now;\

//动作包含完结
#define RUN_TIME_END()  \
	}\
}\


s32 MsgCheck(u8 *MsgBuf, u8 MsgLen);
s32 read_pipe_msg(int fd, u8 *msgbuf);

s32 Init_NamePipe(u8 type, int *fdr, int *fdw);
s32 MsgRecv_Pipe(s32 fd, struct TSockBuff* TTmpBuff, u8 *msgbuf, u8 *more);
void SendPipeMsg(s32 fd, u16 type, u8 *AttachData, u8 dlen);
#endif
