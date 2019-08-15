/*
���������������� �ȴ��ͻ�������
ֻͨ���˿�10001��������
������������� int Connect_fd[16]; ���鶨�����
*/

/*
�ͻ����������ӳ���
ǰ�᣺ ����������Ѿ���ͨ�����ȴ�����
ͨ���˿�10001������������
����IP 192.168.1.1
��������������Ϣ
*/

#include "lewin.h"

#define MAX_CONNECT         16  //����������������

int Client_fd = 0;
int Server_fd = 0;
int Connect_fd[MAX_CONNECT];

extern struct parameter_set  ParaSet;

//�������Ӷ˿ڣ������տͻ��˷�������Ϣ
void server_recv(void)
{
	int ret;
	struct sockaddr_in LocalAddr;
	int nSize;
	struct timeval TimeOut;
	int fdmax;
	fd_set ReadFDs;
	u8 i;
	
	TimeOut.tv_sec = 0;   
	TimeOut.tv_usec = 10000;
	
	FD_ZERO(&ReadFDs);
	FD_SET(Server_fd, &ReadFDs);
	for(i=0; i<MAX_CONNECT; i++)
	{
		if(Connect_fd[i] > 0) FD_SET(Connect_fd[i], &ReadFDs);			
	}
	
	fdmax = Server_fd;
	for(i=0; i<MAX_CONNECT; i++)
	{
		if(Connect_fd[i] > fdmax) fdmax = Connect_fd[i];
	}
	
	ret = select(fdmax+1, &ReadFDs, NULL, NULL, &TimeOut); 
	if (ret == 0)  //��ʱ��û��Ϣ
	{
		//printf("server no data select\n");
	}
	else if (ret < 0)  //����
	{
		printf("select error:%d!\n",errno);
	}
	else   //�����ݽ��յ�
	{	
		if (FD_ISSET(Server_fd, &ReadFDs))   //�������� �ͻ���ͨ�������ָ���˿����ӹ���
		{
			for(i=0; i<MAX_CONNECT; i++)	{
				if(Connect_fd[i] == 0) break;
			}
			nSize = sizeof(struct sockaddr);
			Connect_fd[i] = accept(Server_fd, (struct sockaddr*)&LocalAddr, &nSize);
			if (Connect_fd[i] < 0)
			{
				printf("Local sock listening accept error.%d\n",errno);
				Connect_fd[i] = 0;	
			}
			else
			{
				printf("have socket connect the terminal Connect_fd[%d]=%d!\n", i, Connect_fd[i]);
				printf("LocalAddr.sin_addr.s_addr = %s\n", inet_ntoa(LocalAddr.sin_addr));
				printf("LocalAddr.sin_port = %d\n", LocalAddr.sin_port);
			}				
		}
		for(i=0; i<MAX_CONNECT; i++)   //�ͻ�����������Ϣ����
		{
			if(Connect_fd[i] == 0) continue;
			if (FD_ISSET(Connect_fd[i], &ReadFDs))
			{
				printf("get local socket message Connect_fd[%d]=%d\n", i, Connect_fd[i]);
				process_client_socket(&Connect_fd[i]);
			}
		}
	}
}

	
//����ͻ����ӻ���������Ϣ
void process_client_socket(int *fd)
{
	int ret;
	u8 buf[1024];
	u8 i;
	
	ret = recv(*fd, buf, 1024, 0);
	if (ret==-1)
	{
		printf("client connect to terminal error fd=%d\n", *fd);
		*fd = 0;		
	}
	else if (ret==0)
	{
		printf("closed client socket connect to terminal fd=%d\n", *fd);
		*fd = 0;
	}
	else
	{
		printf("fd=%d recv=%dbytes:  ", *fd, ret);
		display(NULL, buf, ret);		
		for(i=1; i<16; i++)  buf[i] = buf[0] + 16 - i;
		buf[0] = *fd;
		send(*fd, buf, 16, 0);
		printf("fd=%d send=16bytes:  ", *fd);
		display(NULL, buf, 16);
	}
}


//��ʼ�������������
int init_server_sock(void)
{
	int fd;
	int i;
	struct sockaddr_in addr;
	
	for(i=0; i<MAX_CONNECT; i++){
		Connect_fd[i] = 0;
	}
	
	fd = socket(PF_INET, SOCK_STREAM, 0); //������ô�Զ�ѡ�����磬 �����ж�������أ� ��bind��ָ��
	if (fd < 0)
	{
		printf("Local listening socket error.%d\n", errno);			
		return R_FAIL;
	}
	bzero(&addr, sizeof(struct sockaddr));
	addr.sin_family = PF_INET;                  //ָ��IPV4 ����Э��
	addr.sin_port = htons(ParaSet.SRVRport);	        //�󶨶˿�
	addr.sin_addr.s_addr = htonl(INADDR_ANY);   //��IP
	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
	{
		printf("Local listening socket bind error.%d\n",errno);		
		close(fd);
		return R_FAIL;			
	}
	if (listen(fd, 10) < 0)   //�ȴ�fd�����ӣ� ָ�����ͬʱ������Ϊ10
	{
		printf("Local listening socket listen error.%d\n",errno);
		close(fd);
		return R_FAIL;	
	}
	printf("Local listening socket listen success.fd:%d\n", fd);
	return fd;
}






/*****************************************�ͻ���**************************************************/

//TCP�ͻ��˳�ʼ������������ӣ����ӵ�server��������
int connect_to_server(void)
{
	static time_t tt = 0;
	int fd;
	struct sockaddr_in addr;
	u32 flushstate;
	int ret;
	u8 flag = 0;
	
	if(abs(time(0) - tt) < 15) return -1;
	tt = time(0);
	
	fd = socket(PF_INET, SOCK_STREAM, 0); //������ô�Զ�ѡ�����磬 �����ж�������أ� ��������ʱ��bind��ָ�������ͻ���ʱ����ָ��
	if (fd <= 0)
	{
		printf("Create socket error. %d\n", errno);			
		return -1;
	}
	bzero(&addr, sizeof(struct sockaddr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(ParaSet.SRVRport);	            //��վ�˿�
	addr.sin_addr.s_addr = htonl(ParaSet.SRVRaddr);     //��վIP
	printf("Server info: %08x :%04x, %s:%d \n",
				addr.sin_addr.s_addr,	addr.sin_port, 
				inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));	
	
	flushstate = 1;
	ioctl(fd, FIONBIO, &flushstate);  //����Ϊ��������ʽ

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (ret == 0)
	{
		printf("connet to server OK\n");
		flag = 0x5a;
	}
	else 
	{
	 	if (errno == EINPROGRESS)
		{
			fd_set SockFDs;
			struct timeval TimeOut;
			s32 nSelectRet;
			s32 errorsocket = -1;
			s32 len;

			len = sizeof(s32);
			TimeOut.tv_sec = 0;
			TimeOut.tv_usec = 10000;
			FD_ZERO(&SockFDs);
			FD_SET(fd, &SockFDs);

			nSelectRet = select(fd+1, NULL, &SockFDs, NULL, &TimeOut);
			if (nSelectRet>0)
			{
			    getsockopt(fd, SOL_SOCKET, SO_ERROR, &errorsocket, (socklen_t *)&len);//if server station has fireware,the errorsocket not right.
			    if(errorsocket==0)
			    {
					printf("connet to server OK 1\n");
					flag = 0x5a;
			    }
			}			
		}
	}

	flushstate = 0;
	ioctl(fd, FIONBIO, &flushstate); //set socket of flush state
	
	if(flag == 0x5a)
	{
		ret = log_no_server(fd);
		if(ret == 0) return fd;
		else return -1;
	}
	else
	{
		close(fd);
		return -1;
	}
}

int log_no_server(int fd)
{
	return 0;
}

//������Ϣ��������
int send_to_server(int fd, u8 *buf, u8 len)
{
	int ret;

	ret = send(fd, buf, len, 0);
	if(ret == len)
	{
		return 0;
	}
	else
	{
		printf("send data error\n");
		return -1;
	}
}










//����Ƿ����������ӿͻ���(���������͹���)����Ϣ
//��鵽����Ϣʱ�����ϵ��� process_server_socket ִ����Ϣ��ȡ�ʹ���
void get_server_msg(void)
{
	struct timeval TimeOut;
	fd_set ReadFDs;
	int ret;
	
	TimeOut.tv_sec = 0;   
	TimeOut.tv_usec = 10000;
	
	FD_ZERO(&ReadFDs);
	if(Client_fd > 0) FD_SET(Client_fd, &ReadFDs);		
	ret = select(Client_fd+1, &ReadFDs, NULL, NULL, &TimeOut); 
	if (ret == 0)  //��ʱ��û��Ϣ
	{
		//printf("client no data select\n");
	}
	else if (ret < 0)  //����
	{
		printf("select error:%d!\n",errno);
	}
	else   //�����ݽ��յ�
	{	
		if (FD_ISSET(Client_fd, &ReadFDs))
		{
			printf("get local socket message\n");
			process_server_socket();
		}
	}
}

//��ȡ����������������Ϣ������
void process_server_socket(void)
{
	int ret;
	u8 buf[1536];
	
	ret = recv(Client_fd, buf, 1536, 0);
	if (ret==-1)
	{
		printf("client connect to terminal error fd=%d\n", Client_fd);
		Client_fd = 0;		
	}
	else if (ret==0)
	{
		printf("closed client socket connect to terminal fd=%d\n", Client_fd);
		Client_fd = 0;
	}
	else
	{
		printf("fd=%d recv=%dbytes:  ", Client_fd, ret);
		display(NULL, buf, ret);
		
		if(buf[0] == 0x68)
		{
			process_test_msg(buf, ret);
		}
	}
}

void process_test_msg(u8 *buf, u16 ret)
{
	u16 x,y;
	
	x = bcd2uchar(buf[2])*100 + bcd2uchar(buf[3]);
	y = bcd2uchar(buf[4])*100 + bcd2uchar(buf[5]);
	
	for(x=0; x<ret; x++)
	{
		buf[x] = buf[x]+1;
	}
	send_to_server(Client_fd, buf, ret);
}

