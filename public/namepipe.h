#ifndef DC_PUBLIC
#define DC_PUBLIC



/* head and tail */
#define MSG_HEAD    0x1B1B
#define MSG_TAIL    0xB1B1

#define MSG_CODE_COMM 1
#define MSG_CODE_MAIN21

//MsgType
//�ڲ�ͨ��(pipe)������
#define PIPE_INIT   0x0000
#define PIPE_REBOOT  0xcc01  //WDT֪ͨ����ϵͳ������������Ҫ�Լ���������
//  1��main ---> comm
#define PIPE_UPDATE    0xaa01  //Ҫ��ͨ��ģ���ϱ��ڵ�Ĳɼ�����  �������ݣ��ϲ�ͨ��Э�����ݣ�  len �ϱ���Э�鳤�ȣ�����ͷβ��
#define PIPE_PAPAUP_OK 0xaa02  //�ڵ�������³ɹ�
#define PIPE_SETNODE_OK 0xaa03  //�ڵ�Զ�����óɹ�

//  2��comm ---> main
#define PIPE_LIGHTCG 0xbb01  //֪ͨmain�������Ȳ���������   �������ݣ��ڵ��ַ + ���Ȳ�����
#define PIPE_NODECG  0xbb02  //֪ͨmain���̽ڵ��б������   �������ݣ��ü��������õĽڵ�������  len = 2
#define PIPE_SETNODE 0xbb03  //֪ͨmain�������ýڵ�Ĺ���ģʽ  �������ݣ��ڵ��ַ+ģʽ 7�ֽڣ�00H�� �����忨ȡ�繤��״̬��33H�� �������ͨ��״̬ 55H�� ������ƶϵ�״̬


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

///����t��,�Ŷ���һ��
#define RUN_COUNT_START(t) {\
	static u16 cnt = (t);  \
	if( (cnt == (t)) || (nDebugLevel == 0xff) ) \
	{\
			cnt = 0;\

//�����������
#define RUN_COUNT_END()  \
	}\
	cnt++;\
}\


///����t��,�Ŷ���һ��
#define RUN_TIME_START(t) {\
	static time_t last = 0;  \
	time_t now; \
	time(&now);\
	if( (now - last >= (t)) || (nDebugLevel == 0xff) )  \
	{\
			last = now;\

//�����������
#define RUN_TIME_END()  \
	}\
}\


s32 MsgCheck(u8 *MsgBuf, u8 MsgLen);
s32 read_pipe_msg(int fd, u8 *msgbuf);

s32 Init_NamePipe(u8 type, int *fdr, int *fdw);
s32 MsgRecv_Pipe(s32 fd, struct TSockBuff* TTmpBuff, u8 *msgbuf, u8 *more);
void SendPipeMsg(s32 fd, u16 type, u8 *AttachData, u8 dlen);
#endif
