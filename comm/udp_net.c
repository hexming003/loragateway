/*
客户端网络连接程序。
前提： 主机服务端已经开通，并等待连接
通过端口10001来连接主机；
主机IP 192.168.1.1
并向主机发送消息
*/

#include "comm.h"

int UDP_fd = -1;
extern struct parameter_set  ParaSet;
extern u8 Regist_flag;
extern time_t RecvServerMsgT;

void init_UDP_socket(void)
{
	struct ip_mreq command;
	int loop = 0;   /* 多播循环 */
	int bord = 1;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(ParaSet.UDPport);
	if((UDP_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		write_log(MSG_INFO, "socket err\n");
		return;
	}

	/* 调用bind之前，设置套接口选项启用多播IP支持*/
	bord = 1;
	if(setsockopt(UDP_fd, SOL_SOCKET, SO_REUSEADDR, &bord, sizeof(bord)) < 0)
	{
		write_log(MSG_INFO, "setsockopt:SO_REUSEADDR ERR\n");
		close(UDP_fd);
		UDP_fd = -1;
		return;
	}

	if(bind(UDP_fd,(struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		write_log(MSG_INFO, "bind ERR \n");
		close(UDP_fd);
		UDP_fd = -1;
		return;
	}

	/* 在同一个主机上进行广播设置套接口，作用是方便单个开发系统上测试多播IP广播 */
	loop = 0;
	if(setsockopt(UDP_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
	{
		write_log(MSG_INFO, "setsockopt:IP_MULTICAST_LOOP ERR\n");
		close(UDP_fd);
		UDP_fd = -1;
		return;
	}

	 /* 加入一个广播组。进一步告诉Linux内核，特定的套接口即将接受广播数据*/
	command.imr_multiaddr.s_addr = htonl(ParaSet.UDPBaddr);          //inet_addr("224.0.0.1");
	command.imr_interface.s_addr = htonl(INADDR_ANY);
	if(setsockopt(UDP_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &command, sizeof(command)) < 0)
	{
		write_log(MSG_INFO, "setsockopt:IP_ADD_MEMBERSHIP ERR\n");
		close(UDP_fd);
		UDP_fd = -1;
		return;
	}

	if(set_mcb_multcate() == -1)
	{
		write_log(MSG_INFO, "set mcba multcate regist ERR!\n");
		close(UDP_fd);
		UDP_fd = -1;
		return;
	}

	write_log(MSG_INFO, "init_UDP_socket OK\n");
}



void get_udp_msg(void)
{
	static u8 errcount = 0;
	struct timeval TimeOut;
	struct sockaddr_in their_addr;
	fd_set ReadFDs;
	int addr_len;
	int dlen,ret;
	u8 buf[2048];

	if(UDP_fd <= 0)
	{
		init_UDP_socket();
	}

	TimeOut.tv_sec = 0;
	TimeOut.tv_usec = 10000;

	FD_ZERO(&ReadFDs);
	FD_SET(UDP_fd, &ReadFDs);
	ret = select(UDP_fd+1, &ReadFDs, NULL, NULL, &TimeOut);
	if (ret == 0)  //超时，没消息
	{
		errcount = 0;
		//printf("no UDP msg\n");
	}
	else if (ret < 0)  //出错
	{
		errcount ++;
		write_log(MSG_INFO, "UDP select error:%d!\n",errno);  //什么时候才会这样，由于上次fd没关闭， 端口给占用了。这个会频繁出现
		if(errcount > 100)
		{
			system("reboot");
		}
	}
	else   //有数据接收到
	{
		errcount = 0;
		if (FD_ISSET(UDP_fd, &ReadFDs))
		{
			addr_len = sizeof(struct sockaddr);
			dlen = recvfrom(UDP_fd, buf, 2048, 0, (struct sockaddr *)&their_addr, &addr_len); // 接收数据
			if (dlen == -1)
			{
				write_log(MSG_INFO, "get_udp_msg recv error\n");
				close(UDP_fd);
				UDP_fd = -1;
				return;
			}
			write_comm(MSG_COMMR+MSG_DEBUG, "UDP_recv: ",  buf, dlen);
			RecvServerMsgT = time(NULL);
			process_server_msg(buf, dlen);
		}
	}
}


void udp_send(u32 addr, u32 port, u8 *buf, int dlen)
{
	//static int sock = -1;
	struct sockaddr_in addrto;
	int ret;
	int nlen;
	struct in_addr tmpaddr;
	c8 srbuf[64];

	usleep(1000);

	if(UDP_fd <= 0)
	{
		init_UDP_socket();
	}

	bzero(&addrto, sizeof(struct sockaddr_in));
	addrto.sin_family = AF_INET;

	//inet_aton("192.168.2.255", &addrto.sin_addr);
	addrto.sin_addr.s_addr = htonl(addr);
	addrto.sin_port = htons(port);

	tmpaddr.s_addr = htonl(addr);
	//write_log(MSG_INFO, "UDP_send: %s:%d, len=%d data: ", inet_ntoa(tmpaddr), port, dlen);
	//display(NULL, buf, dlen);
	sprintf(srbuf, "UDP_send: %s:%d, len=%d data: ", inet_ntoa(tmpaddr), port, dlen);
	write_comm(MSG_COMMR+MSG_DEBUG, srbuf, buf, dlen);

	nlen = sizeof(addrto);
	ret = sendto(UDP_fd, buf, dlen, 0, &addrto, nlen);
	if(ret<0)
	{
		write_log(MSG_INFO, "send udp socket error!!! 3 %d\n", ret);
	}
	else
	{

	}

	record_comm_msg('S', buf, dlen);
}





