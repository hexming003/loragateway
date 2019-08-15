#include "comm.h"

#define TCP		0
#define UDP		1
typedef struct
{
    c8 *command;      /*the AT Command we send to Modem*/
    c8 *result;       /*the result we expected*/
    u8 timeout;       /*MAX time of second that we could waiting*/
    u8 r_time;        /*MAX times that we could re_send to Modem*/
}AT_COMMAND;

u8 GPRS_ip[4];             //HEX GPRS拨号后的IP地址 3   ASCII
int Gprs_fd = 0;
u8 Flag_mpwr = 0;

extern struct parameter_set ParaSet;
extern u8 SGPRS_flag;
extern u8 Sturnon_count;
extern u8 Sdail_count;
extern u8 Scont_count;
extern u8 Scommfail_count;


u8 check_AT_return(void);
void reset_modem(void);
u8 at_comm_wait(u8 *str, u8 len, u8 wait);



u16 myatoi(u8 *str, u8 len)
{
	u8 flag = 0;
	u8 i;
	u16 ret = 0;

	for(i=0; i<len; i++)
	{
		if( (str[i]>'9') || (str[i]<'0') )
		{
			if(flag != 0) return ret;
			else          continue;
		}
		flag = 1;
		ret = ret * 10;
		ret += str[i]-'0';
	}
	return ret;
}

u8 str_comp(u8 *str1, u8 *str2, u8 len)
{
	u8 i;

	for(i=0; i<len; i++)
	{
		if(str1[i] != str2[i]) return 1;
	}

	return 0;
}

void GPRS_pwr_on(void)
{
	gpio_write_high(MPWR);
	Flag_mpwr = 1;
	sleep(1);
}

void GPRS_pwr_off(void)
{
	gpio_write_low(MPWR);
	Flag_mpwr = 0;
	sleep(1);
}

u8 turn_on_modem(void)
{
	if( check_AT_return() != 0 )
	{
		gpio_write_high(MON_OFF);
		sleep(1);
		gpio_write_low(MON_OFF);
		sleep(1);

		if( check_AT_return() == 0 )
			return 0;
		else
			return 1;
	}
	else
		return 0;
}


u8 turn_off_modem(void)
{
	if( check_AT_return() == 0)
	{
		gpio_write_high(MON_OFF);
		sleep(1);
		gpio_write_low(MON_OFF);
		sleep(1);

		if( check_AT_return() != 0 )
			return 0;
		else
			return 1;
	}
	else
		return 0;
}

void reset_modem(void)
{
	gpio_write_low(MRST);
	sleep(1);
	gpio_write_high(MRST);
	sleep(1);
}




//获取信号强度
void get_signal(void)
{
	write(Gprs_fd, "AT+CSQ\r", 7);       //获取信号强度
	write_log(MSG_DEBUG, "Gprs send: AT+CSQ\r");
}

u8 check_AT_return(void)
{
	write(Gprs_fd, "AT\r\n", 4);
	write_log(MSG_DEBUG, "Gprs send: AT\r\n");
	return at_comm_wait("OK", 2, 100);
}

//返回 0:接收到想要的字符串
//     1:指定时间内没有接受到想要的字符串
u8 at_comm_wait(u8 *str, u8 len, u8 waittime)
{
	u8 i;
	u8 buf[512];
	u16 pt = 0;
	int ret;
	
	while(1)
	{
		ret = myselect(Gprs_fd, waittime);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 512-pt);
		if (ret <= 0) return 1;
		else
		{
			pt += ret;
			
			buf[pt]= '\0';
			printf("Gprs recv: %s\n",buf);
			
			for(i=0; i<=pt-len; i++)
			{
				if( str_comp(buf+i, str, len) == 0 )
				{
					return 0;
				}
			}
		}
	}
	return 1;
}

u8 GPRS_AT_Init(void)
{
	int i;
	
	AT_COMMAND initATstr[] =
	{
		{ "ATE0\r\n",                                           "OK",   1,   20 },  //关闭回显
		{ "AT+CSQ\r\n",                                         "OK",   2,   16 },  //查询信号强度
		{ "AT+CGCLASS=\"B\"\r\n",                               "OK",   3,   5 },   //设置根据指定的移动台类别工作

		{ NULL, NULL, 5, 0},
	};	
	
	AT_COMMAND *pATstr = initATstr;
	
	while(pATstr->command != NULL)
	{
		for(i = 0; i < pATstr->r_time; i++)
		{
			write(Gprs_fd, pATstr->command, strlen(pATstr->command) );
			write_log(MSG_DEBUG, "Gprs send: %s\n", pATstr->command );
			if(at_comm_wait( pATstr->result, strlen(pATstr->result), pATstr->timeout*100) != 0)
			{
				if(at_comm_wait(pATstr->result, strlen(pATstr->result), pATstr->timeout*100) != 0)
				{
						//return 3;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		
		if (i >= pATstr->r_time)
		{
			return 2;
		}
		sleep(1);
		pATstr++;
	}
	return 0;
}

u8 GPRS_AT_Connect(void)
{
	int i;
	c8 CIPCSGPcmd[128];
	
	AT_COMMAND initATstr[] =
	{
		{ CIPCSGPcmd,                                           "OK",   1,   3},   //GPRS连接设置参数
#if TCP		
		{ "AT+CLPORT=\"TCP\",\"0\"\r\n",                     	"OK",   1,   3},   //设置TCP本地端口号
#else
		{ "AT+CLPORT=\"UDP\",\"0\"\r\n",                     	"OK",   1,   3},   //设置UDP本地端口号
#endif	
		{ "AT+CIPMUX=1\r\n",                                    "OK",   1,   3},   //启动多IP连接 ??? 应该用单路AT+CIPMUX=0
		{ "AT+CSTT=\"CMNET\",\"\",\"\"\r\n",                    "OK",   1,   3},   //启动任务并设置接入点APN, 用户名，密码？？？
		{ "AT+CIICR\r\n",                                       "OK",   1,   3},   //激活移动场景，发起GPRS或CSD无线连接
		{ "AT+CIPQSEND=0\r\n",                                  "OK",   1,   3},   //选择数据发送模式
		//{ "AT+CIFSR\r\n",                                       "\r\n",     1,   3},  //获取本机ip地址

		{ NULL, NULL, 5, 0},
	};	
	
	AT_COMMAND *pATstr = initATstr;

	
	sprintf(CIPCSGPcmd, "AT+CIPCSGP=1,\"%s\",\"%s\",\"%s\"\r\n", ParaSet.APN, ParaSet.APNName, ParaSet.APNPWD);
	
	while(pATstr->command != NULL)
	{
		for(i = 0; i < pATstr->r_time; i++)
		{
			write(Gprs_fd, pATstr->command, strlen(pATstr->command) ); 
			write_log(MSG_DEBUG, "Gprs send: %s\n", pATstr->command );
			if(at_comm_wait( pATstr->result, strlen(pATstr->result), pATstr->timeout*100) != 0)
			{
				if(at_comm_wait(pATstr->result, strlen(pATstr->result), pATstr->timeout*100) != 0)
				{
						//return 3;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		
		if (i >= pATstr->r_time)
		{
			return 2;
		}
		
		sleep(1);
		pATstr++;
	}
	
	return 0;
}

//+XIIC： 1, 10.10.73.214
u8 get_GPRS_ip(void)
{
	int ip[4];
	u8 i;
	u8 buf[512];
	u16 pt = 0;
	int ret;

	write(Gprs_fd, "AT+CIFSR\r\n", 10);    //获取本机ip地址
	write_log(MSG_DEBUG, "Gprs send: AT+CIFSR\r" );
	while(1)
	{
		ret = myselect(Gprs_fd, 250);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 512-pt);
		pt += ret;
		
		buf[pt]= '\0';
		printf("Gprs_recv: %s\n",buf);
		
		{			
			i = sscanf(buf, "%d.%d.%d.%d", &ip[0],&ip[1],&ip[2],&ip[3]) ;
			if (4 == i)
			{
				GPRS_ip[0] = ip[0];
				GPRS_ip[1] = ip[1];
				GPRS_ip[2] = ip[2];
				GPRS_ip[3] = ip[3];

				return 0;
			}
		}
		if (ret <= 0) return 1;
	}
	return 1;
}


/*
void get_sim_stat(void)
{
	u8 i;
	U16 modem_buf_pt;

	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");

	modem_buf_hpt = modem_buf_tpt;
	Uart0_recv_haveframe = 0;

	memcpy(Uart0_send_buf, "AT+CCID\r", 9);  //获取sim卡号
	uart0send();
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");
	delayTimemS(200);
	asm("WDR");

	while(Uart0_recv_haveframe)
	{
		modem_buf_pt = modem_buf_hpt;
		for(i=0; i<255; i++)
		{
			Gtmpbuf[i] = modem_buf[modem_buf_pt++];
			if(Gtmpbuf[i] == '\r') break;
			if(modem_buf_pt >= 512) modem_buf_pt=0;
		}
		if( search_str(Gtmpbuf, "ERROR", i, 5) == 0 )
		{
			flag_sim = 0;
			modem_buf_hpt = modem_buf_pt;
			Uart0_recv_haveframe--;
			return;
		}

		modem_buf_hpt = modem_buf_pt;
		Uart0_recv_haveframe--;
	}
}
*/



//拨号连接GPRS网络
//该函数按通信失败次数， 重连接次数， 重拨号次数进行不同的操作，具体见拨号流程图
u8 connect_GPRS_net(void)
{
	c8 sbuf[128];
	u8 i;
	u8 count;
	
	SGPRS_flag = 0xff;

	clearport(Gprs_fd);

	printf("Connect GPRS net...\n");
	
	write(Gprs_fd, "\r", 1);
	write_log(MSG_DEBUG, "Gprs send: \r" );
	at_comm_wait(NULL, 0, 100);

	if(Scommfail_count > 3)  //通信失败次数
	{
		//断开TCP或UDP链接
		if(Flag_mpwr == 1)
		{
			write(Gprs_fd, "AT+CIPCLOSE=0\r", 14);	  //断开TCP或UDP链接		
			write_log(MSG_DEBUG, "Gprs send: AT+CIPCLOSE=0\r" );
			if(at_comm_wait("OK", 2, 100) != 0)
			{
				if(at_comm_wait("OK", 2, 100) != 0)
				{
					//return 3;
				}
			}
		}
	}
	else
	{
		return 0;
	}

	if(Scont_count > 3)  //TCP连接次数
	{
		//断开PPP链接
		GPRS_ip[0] = 0;
		GPRS_ip[1] = 0;
		GPRS_ip[2] = 0;
		GPRS_ip[3] = 0;
		if(Flag_mpwr == 1)
		{
			write(Gprs_fd, "AT+CIICR\r", 11);  //激活移动场景，发起GPRS或CSD无线连接
			write_log(MSG_DEBUG, "Gprs_fd send: AT+CIICR\r" );
			if(at_comm_wait("OK", 2, 100) != 0)
			{
				if(at_comm_wait("OK", 2, 100) != 0)
				{
					//return 3;
				}
			}
		}
	}
	else
	{
		goto TCP_CONNECT;
	}

	if(Sdail_count > 3)  //PPP拨号次数
	{
		if(gpio_read_pin(MPWR) == 1)
		{
			turn_off_modem();
		}
	}
	else
	{
		goto PPP_CONNECT;
	}

	if(Sturnon_count > 3) //Turn on 次数
	{
		if(gpio_read_pin(MPWR) == 1)
		{
			GPRS_pwr_off();
		}
	}
	else
	{
		goto TURN_ON_MODEM;
	}



//PWR_ON_MODEM:
	printf("GPRS_pwr_on.\n");
	GPRS_pwr_off();
	sleep(5);
	GPRS_pwr_on();
	reset_modem();

TURN_ON_MODEM:

	if(Sturnon_count < 0x0F) Sturnon_count ++;

	if( turn_on_modem() != 0)
		return 1;   //modem电源打开失败	
	sleep(5);
	
	//关闭回显                    
	//查询信号强度                
	//设置根据指定的移动台类别工作

	if (2 == GPRS_AT_Init() )
	{
		return 2;
	}
	sleep(2);

PPP_CONNECT:

	if(Sdail_count < 0x0F) Sdail_count ++;



	/*
	//设置为2G模块内部协议栈
	write(Gprs_fd, "AT+XISP=0\r", 10);
	write_log(MSG_DEBUG, "Gprs send: AT+XISP=0\r" );
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 4;
		}
	}
	*/
	//设置APN
	//{
		//ParaSet.APN[16] = 0;
		//count = strlen(ParaSet.APN);
		/*
		memcpy(sbuf, "AT+CGDCONT=1,\"IP\",\"", 19);
		memcpy(sbuf+19, ParaSet.APN, count);
		memcpy(sbuf+19+count, "\"\r", 2);
		*/
		//sprintf(sbuf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", "CMNET");
		//write(Gprs_fd, sbuf, strlen(sbuf) );
		//write_log(MSG_DEBUG, "Gprs send: %s", sbuf );
		//memcpy(Uart0_send_buf, "AT+CGDCONT=1,\"IP\",\"CMNET\"\r", 26);
		//at_comm_send(26);
		
		//printf("%s\n", sbuf);
		//if(at_comm_wait("OK", 2, 100) != 0)
		//{
		//	if(at_comm_wait("OK", 2, 150) != 0)
		//	{
		//		return 5;
		//	}
		//}
	//}

	/*
	// 用户身份认证，专网中一般需要加这条指令
	sprintf(sbuf, "AT+XGAUTH=1,1,\"%s\",\"%s\"\r", ParaSet.APNName, ParaSet.APNPWD);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_DEBUG, "Gprs send: %s", sbuf);
	//memcpy(Uart0_send_buf, "AT+XGAUTH=1,1,\"CMNET\",\"CMMET\"\r", 30);
	//at_comm_send(30);
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 6;
		}
	}
	*/
	//进行PPP连接
	write(Gprs_fd, "AT+CGATT=1\r\n", 12);
	write_log(MSG_DEBUG, "Gprs send: AT+CGATT=1\r\n");
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 7;
		}
	}
	at_comm_wait(NULL, 0, 100);
	
	//判断模块是否注册上网络
	for(i=0; i<10; i++)
	{
		printf("AT+CGREG?\n");
		write(Gprs_fd,"AT+CGREG?\r\n", 11);
		write_log(MSG_COMMT+MSG_DEBUG, "Gprs send: AT+CGREG?\r" );
		if( (at_comm_wait(",1", 2, 100) == 0) || (at_comm_wait(",5", 2, 100) == 0) )
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	if(i >= 10) return 3;
	
	GPRS_AT_Connect();
	
	//获取IP地址
	for(i=0; i<5; i++)
	{
		if(get_GPRS_ip() != 0)	return 8;
		if( (GPRS_ip[0] != 0) || (GPRS_ip[1] != 0) || (GPRS_ip[2] != 0) || (GPRS_ip[3] != 0) )
		{
			printf("IP = %d.%d.%d.%d \r\n", GPRS_ip[0], GPRS_ip[1], GPRS_ip[2],GPRS_ip[3]);
			break;
		}
		at_comm_wait(NULL, 0, 200);
	}
	if(i >= 5) return 9;


TCP_CONNECT:

	if(Scont_count < 0x0F) Scont_count ++;

	at_comm_wait(NULL, 0, 100);

	//建立TCP链接
#if TCP	
	printf("AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d\r\n", *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.serport);
	sprintf(sbuf, "AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d\r\n", *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.serport);
	//sprintf(sbuf, "AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d\r\n", 59, 42, 107, 151, 51236);
#else
	printf("AT+CIPSTART=0,\"UDP\",\"%d.%d.%d.%d\",%d\r\n", *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.SRVRport);
	sprintf(sbuf, "AT+CIPSTART=0,\"UDP\",\"%d.%d.%d.%d\",%d\r\n",  *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.SRVRport);
	//sprintf(sbuf, "AT+CIPSTART=0,\"UDP\",\"%d.%d.%d.%d\",%d\r\n", 59, 42, 107, 151, 51236);
#endif	
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_DEBUG, "Gprs send: %s", sbuf );
	//memcpy(Uart0_send_buf, "at+tcpsetup=0,210.21.94.88,9200\r", 32);
	//at_comm_send(32);
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 10;
		}
	}
	at_comm_wait(NULL, 0, 100);
	
	//查询TCP链接状态
	write(Gprs_fd, "AT+CIPSTATUS=0\r\n", 16);
	write_log(MSG_DEBUG, "Gprs send: AT+CIPSTATUS=0\r\n" );
	if(at_comm_wait("CONNECT", 7, 50) != 0)
	{
		if(at_comm_wait("CONNECT", 7, 100) != 0)
		{
			return 11;
		}
	}
	
	SGPRS_flag = 0;
	Scommfail_count = 0;
	
	//printf("Hear beat...\n");
	//hearbeat_server();

	return 0;
}




void GPRS_send_data(u8 *buf, int len)
{
	u8 sbuf[256];
	
	printf("GPRS_send_data... flag=%d, Scommfail_count=%d\n", SGPRS_flag, Scommfail_count);
	
	if(SGPRS_flag != 0)
	{
		if(SGPRS_flag == 0xff)  //正在连接网络
		{
			return;
		}
		else
		{
			SGPRS_flag = RECONNECT;
			return;
		}
	}

	if(Scommfail_count > 3)
	{
		SGPRS_flag = RECONNECT;
	}

	Scommfail_count ++;

	//MAX_wait_timer = 100;
	//while((Uart0_send_haveframe == HAVE) && MAX_wait_timer);
	//memcpy(Uart0_send_buf, "AT+TCPSEND=0,", 13);
	////Uart0_send_buf[13] = '0' + ((len/10)%10);  //十位
	//Uart0_send_buf[14] = '0' + (len%10);  //个位
	//Uart0_send_buf[15] = '\r';
	//at_comm_send(16);
	
	
	sprintf(sbuf, "AT+CIPSEND=0,%d\r\n", len);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_DEBUG, "Gprs send: %s", sbuf );
	
	if(at_comm_wait(">", 1, 50) != 0)
	{
		//delayTimemS(200);
	}
	
	memcpy(sbuf, buf, len);
	
	sbuf[len + 0] = '\r';
	sbuf[len + 1] = '\n';	
	write(Gprs_fd, sbuf, len + 2);
	//write(Gprs_fd, "abcdefghijklmnopq", len + 2);
	write_comm(MSG_COMMR+MSG_DEBUG, "Gprs send:", sbuf, len + 2);
}


void hearbeat_server(void)
{
	u8 outbuf[256];

	outbuf[0] = 0x68;
	outbuf[5] = 0x12;
	outbuf[6] = 0x12;
	outbuf[7] = 0x12;
	outbuf[8] = 0x12;
	outbuf[5] = 0;
	outbuf[6] = 0;
	outbuf[7] = 0x68;
	outbuf[8] = 0xA4;
	outbuf[9] = 0;
	outbuf[10] = 0;
	//CS_check(outbuf, 11);
	outbuf[12] = 0x16;

	GPRS_send_data(outbuf, 13);
}



/* AT指令定义
//13711112418

Send: AT\r
Recv: OK

Send: ATZ\r
Recv: OK

Send: ATE0    //禁止回显
Recv: OK

Send: AT+CCID  //查询sim卡ID
Recv: +CCID: 89860002190810001367
Recv: OK

Send: AT+CREG?  //查询是否注册上网络，只有(0,1)或(0,5)时表示注册上网络，可以进行拨号了
Recv: +CREG:0,1
Recv: OK

Send: AT+CSQ\r   检查信号强度
Recv: +CSQ: 24,0   或 +CSQ: 99,99
Recv: OK

Send: AT+ISP=0\r   设置为内部协议栈（默认为外部的，所以必须设置）
Recv: OK

Send: AT+CGDCONT=1,"IP","CMNET"\r   设置APN
Recv: OK

Send: AT+XGAUTH=1,1,"NAME","PWD"\r  身份验证（需要时使用）
Recv: OK

Send: at+xiic=1   //进行PPP连接
Recv: OK

Send: AT+TCPSETUP=0,210.21.94.88,9000 建立TCP链接
Recv: OK


Send: at+tcpsend=0,10 // 在TCP 连接上发送数据
Recv: >
Send: 0123456789 \r
Recv:OK
Recv:+TCPSEND：0,10 //数据发送成功
Recv:at+ipstatus=0

Recv:+TCPRECV:0,10,xxxxxxx \r  \\接收数据




Send: AT+IPR=9600\r  设置modem通信波特率
Recv: OK
Send: AT&F\r         保存设置
Recv: OK

Send: AT^SMSO=?\r   能否软复位
Recv: OK
Send: AT^SMSO\r     控制软复位
Recv: ^SMSO: MS OFF
Recv: OK
Recv: ^SHUTDOWN

Send: AT^SCKS?\r   检查SIM卡
Recv: ^SCKS: 0,1\r OK\r

Send: AT+CXXCID    检查 SIM卡是否正确
Recv: +CXXCID: 89860064190713698640

Send: AT+CNMI=3,1,0,0,1\r 短信接收位置


Send: AT+CMGF=1\r   \\短信发送方式


Send: AT+CMGS=%s\r \\发送短信目标号码
Recv: >
Send: %s(0x1A)     \\发送短信内容
Recv: +CMGS: 203   \\发送成功
Recv: OK

Recv: +CMTI: "MT",1 \\接收到短消息提示
Send: AT+CMGR=1     \\读取短消息
Recv: +CMGR: "REC UNREAD","+8613640251499",,"08/02/26,21:50:56+32"
Recv: (/SEND 01:1001;02:0001;03:00C/)
Recv: OK
Send: AT+CMGD=1  \\删除短消息

Send: at+cgdcont=1,"ip","cmnet"   GPRS拨号设置
Recv: OK

Send: ATDT*99***1#    GPRS拨号
Recv: CONNECT







 init name pipe OK fdr=3  fdw=4 
portset:  /dev/ttyS4, 115200, 8/N/1, 0
comm program start OK Gprs_fd=5
Connect GPRS net.
Gprs recv: ü: AT+CIICR=0
GPRS_pwr_on.
Gprs send: AT
Gprs recv: IIII??
Gprs send: AT
Gprs recv: 
OK

Gprs send: ATE0

Gprs recv: 
OK

Gprs send: AT+CSQ

Gprs recv: 
+CSQ: 15,0

OK

Gprs send: AT+CGCLASS="B"

Gprs recv: 
OK

Gprs send: AT+CGDCONT=1,"IP","CMNET"
AT+CGDCONT=1,"IP","CMNET"

Gprs recv: 
OK

Gprs send: AT+CGATT=1
Gprs recv: 
OK

AT+CGREG?
Gprs recv: 
+CGREG: 0,1

OK

Gprs send: AT+CIPCSGP=1,"","",""

Gprs recv: 
OK

Gprs send: AT+CLPORT="UDP","0"

Gprs recv: 
OK

Gprs send: AT+CIPMUX=1

Gprs recv: 
OK

Gprs send: AT+CSTT="CMNET","",""

Gprs recv: 
OK

Gprs send: AT+CIICR

Gprs recv: 
OK

Gprs send: AT+CIPQSEND=0

Gprs recv: 
OK

Gprs_recv: AT+CIFSR
10.28.13.233

IP = 10.28.13.233 
AT+CIPSTART=0,"UDP","183.136.213.204",7007
Gprs send: AT+CIPSTART=0,"UDP","183.136.213.204",7007
Gprs recv: 
OK

0, CONNECT OK

Gprs send: AT+CIPSTATUS=0
Gprs recv: 
+CIPSTATUS: 0,0,"UDP","183.136.213.204","7007","CONNECTED"

OK

GPRS_send_data... flag=0, Scommfail_count=0
Gprs send: AT+CIPSEND=0,37
Gprs recv: 
> 
Gprs send: 68 02 aa 12 34 56 00 00 aa 12 34 56 00 00 0d 02 01 00 16 00 68 65 61 72 74 20 74 6f 20 73 65 72 76 65 72 ea 16 0d 0a
GPRS rlen14: 
0, SEND OK
GPRS recv:  0d 0a 30 2c 20 53 45 4e 44 20 4f 4b 0d 0a
GPRS rlen38: 
+RECEIVE,0,20:
h?a4VGPRS recv:  0d 0a 2b 52 45 43 45 49 56 45 2c 30 2c 32 30 3a 0d 0a 68 82 aa 12 34 56 00 00 0f 01 01 03 17 0b 00 00 00 00 66 16
2G rmsg: 68 82 aa 12 34 56 00 00 0f 01 01 03 17 0b 00 00 00 00 66 16
SetSysTime: 0015-01-01 03:23:11
GPRS_send_data... flag=0, Scommfail_count=0
Gprs send: AT+CIPSEND=0,37
Gprs recv: 
> 





*/

