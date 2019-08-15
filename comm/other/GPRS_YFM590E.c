#include "comm.h"

u8 GPRS_ip[4];             //HEX GPRS���ź��IP��ַ 3   ASCII
int Gprs_fd = 0;
extern struct parameter_set ParaSet;;
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
	sleep(1);
}

void GPRS_pwr_off(void)
{
	gpio_write_low(MPWR);
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




//��ȡ�ź�ǿ��
void get_signal(void)
{
	write(Gprs_fd, "AT+CSQ\r", 7);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs send: AT+CSQ\r");
}

u8 check_AT_return(void)
{
	write(Gprs_fd, "AT\r", 3);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs send: AT\r");
	return at_comm_wait("OK", 2, 100);
}

u8 at_comm_wait(u8 *str, u8 len, u8 waittime)
{
	u8 i;
	u8 buf[256];
	u8 pt;
	int ret;
	
	while(1)
	{
		ret = myselect(Gprs_fd, waittime);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 1024);
		if (ret <= 0) return 1;
		else
		{
			pt += ret;
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



//+XIIC�� 1, 10.10.73.214
u8 get_GPRS_ip(void)
{
	u8 s[4];
	u8 i;
	u8 buf[512];
	u16 pt;
	u8 *databuf;
	int ret;

	write(Gprs_fd, "at+xiic?\r", 9);    
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: at+xiic?\r" );
	while(1)
	{
		ret = myselect(Gprs_fd, 250);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 1024);
		if (ret <= 0) return 1;
		else
		{
			pt += ret;
			for(i=0; i<=pt-2; i++)
			{
				if( str_comp(buf+i, "OK", 2) == 0 )
				{
					databuf = buf;
					for(i=0; i<25; i++)
					{
						if(*databuf++ == ',')
							break;
					}
				
					for(i=0; i<4; i++)
					{
						s[i] = *databuf ++;
						if(s[i] == '.')
						{
							GPRS_ip[0] = myatoi(s, i);
							break;
						}
					}
				
					for(i=0; i<4; i++)
					{
						s[i] = *databuf ++;
						if(s[i] == '.')
						{
							GPRS_ip[1] = myatoi(s, i);
							break;
						}
					}
				
					for(i=0; i<4; i++)
					{
						s[i] = *databuf ++;
						if(s[i] == '.')
						{
							GPRS_ip[2] = myatoi(s, i);
							break;
						}
					}
				
					for(i=0; i<4; i++)
					{
						s[i] = *databuf ++;
						if( (s[i] == ',') || (i == 3) )
						{
							GPRS_ip[3] = myatoi(s, i);
							break;
						}
					}
				
					return 0;
				}
			}
		}
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

	memcpy(Uart0_send_buf, "AT+CCID\r", 9);  //��ȡsim����
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



//��������GPRS����
//�ú�����ͨ��ʧ�ܴ����� �����Ӵ����� �ز��Ŵ������в�ͬ�Ĳ������������������ͼ
u8 connect_GPRS_net(void)
{
	c8 sbuf[128];
	u8 i;
	u8 count;
	
	SGPRS_flag = 0xff;

	clearport(Gprs_fd);

	write(Gprs_fd, "\r", 1);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: \r" );
	at_comm_wait(NULL, 0, 100);

	if(Scommfail_count > 3)  //ͨ��ʧ�ܴ���
	{
		//�Ͽ�TCP����
		//if(MPWR_PIN == 1)
		{
			write(Gprs_fd, "at+tcpclose=0\r", 14);
			write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: at+tcpclose=0\r" );
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

	if(Scont_count > 3)  //TCP���Ӵ���
	{
		//�Ͽ�PPP����
		GPRS_ip[0] = 0;
		GPRS_ip[1] = 0;
		GPRS_ip[2] = 0;
		GPRS_ip[3] = 0;
		//if(MPWR_PIN == 1)
		{
			write(Gprs_fd, "AT+CGATT=0\r", 11);  //AT+CGATT=1  ����PDP�����IP
			write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: AT+CGATT=0\r" );
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

	if(Sdail_count > 3)  //PPP���Ŵ���
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

	if(Sturnon_count > 3) //Turn on ����
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

	GPRS_pwr_on();
	reset_modem();

TURN_ON_MODEM:

	if(Sturnon_count < 0x0F) Sturnon_count ++;

	if( turn_on_modem() != 0)
		return 1;   //modem��Դ��ʧ��	
	sleep(5);
	
	//�رջ���
	write(Gprs_fd,"ATE0\r", 5);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: ATE0\r" );
	
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 100) != 0)
		{
			return 2;  //�رջ���ʧ��
		}
	}
	sleep(2);

PPP_CONNECT:

	if(Sdail_count < 0x0F) Sdail_count ++;

	//�ж�ģ���Ƿ�ע��������
	for(i=0; i<10; i++)
	{
		write(Gprs_fd,"AT+CREG?\r", 9);
		write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: AT+CREG?\r" );
		if( (at_comm_wait(",1", 2, 100) == 0) || (at_comm_wait(",5", 2, 100) == 0) )
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	if(i >= 10) return 3;


	//����Ϊ2Gģ���ڲ�Э��ջ
	write(Gprs_fd, "AT+XISP=0\r", 10);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: AT+XISP=0\r" );
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 4;
		}
	}

	//����APN
	{
		ParaSet.APN[16] = 0;
		count = strlen(ParaSet.APN);
		memcpy(sbuf, "AT+CGDCONT=1,\"IP\",\"", 19);
		memcpy(sbuf+19, ParaSet.APN, count);
		memcpy(sbuf+19+count, "\"\r", 2);
		write(Gprs_fd, sbuf, 21+count);
		write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: %s", sbuf );
		//memcpy(Uart0_send_buf, "AT+CGDCONT=1,\"IP\",\"CMNET\"\r", 26);
		//at_comm_send(26);
		if(at_comm_wait("OK", 2, 100) != 0)
		{
			if(at_comm_wait("OK", 2, 150) != 0)
			{
				return 5;
			}
		}
	}

	// �û������֤��ר����һ����Ҫ������ָ��
	sprintf(sbuf, "AT+XGAUTH=1,1,\"%s\",\"%s\"\r", ParaSet.APNName, ParaSet.APNPWD);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: %s", sbuf);
	//memcpy(Uart0_send_buf, "AT+XGAUTH=1,1,\"CMNET\",\"CMMET\"\r", 30);
	//at_comm_send(30);
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 6;
		}
	}

	//����PPP����
	write(Gprs_fd, "at+xiic=1\r", 10);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: at+xiic=1\r");
	if(at_comm_wait("OK", 2, 100) != 0)
	{
		if(at_comm_wait("OK", 2, 150) != 0)
		{
			return 7;
		}
	}
	at_comm_wait(NULL, 0, 100);
	
	//��ȡIP��ַ
	for(i=0; i<5; i++)
	{
		if(get_GPRS_ip() != 0)	return 8;
		if( (GPRS_ip[0] != 0) || (GPRS_ip[1] != 0) || (GPRS_ip[2] != 0) || (GPRS_ip[3] != 0) )
		{
			break;
		}
		at_comm_wait(NULL, 0, 200);
	}
	if(i >= 5) return 9;


TCP_CONNECT:

	if(Scont_count < 0x0F) Scont_count ++;

	at_comm_wait(NULL, 0, 100);

	//����TCP����
	sprintf(sbuf, "at+tcpsetup=0,%d.%d.%d.%d,%d\r", ParaSet.SRVRaddr/0x1000000, ParaSet.SRVRaddr/0x10000%0x100, ParaSet.SRVRaddr/0x100%0x100, ParaSet.SRVRaddr%0x100, ParaSet.SRVRport);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: %s", sbuf );
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
	
	//��ѯTCP����״̬
	write(Gprs_fd, "at+ipstatus=0\r", 14);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: at+ipstatus=0\r" );
	if(at_comm_wait("0,CON", 5, 50) != 0)
	{
		if(at_comm_wait("0,CON", 5, 100) != 0)
		{
			return 11;
		}
	}
	
	SGPRS_flag = 0;
	Scommfail_count = 0;
	
	hearbeat_server();

	return 0;
}




void GPRS_send_data(u8 *buf, u8 len)
{
	u8 sbuf[256];
	
	if(SGPRS_flag != 0)
	{
		if(SGPRS_flag == 0xff)  //������������
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
	////Uart0_send_buf[13] = '0' + ((len/10)%10);  //ʮλ
	//Uart0_send_buf[14] = '0' + (len%10);  //��λ
	//Uart0_send_buf[15] = '\r';
	//at_comm_send(16);
	
	memcpy(sbuf, "AT+TCPSEND=0,%d\r", len);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: %s", sbuf );
	if(at_comm_wait(">", 1, 50) != 0)
	{
		//delayTimemS(200);
	}
	
	sbuf[0] = '\r';
	sbuf[1] = '\r';
	memcpy(sbuf, buf, len);
	write(Gprs_fd, sbuf, len+2);
	write_log(MSG_COMMT+MSG_DEBUG, "Gprs_fd send: %s", sbuf);
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



/* ATָ���
//13711112418

Send: AT\r
Recv: OK

Send: ATZ\r
Recv: OK

Send: ATE0    //��ֹ����
Recv: OK

Send: AT+CCID  //��ѯsim��ID
Recv: +CCID: 89860002190810001367
Recv: OK

Send: AT+CREG?  //��ѯ�Ƿ�ע�������磬ֻ��(0,1)��(0,5)ʱ��ʾע�������磬���Խ��в�����
Recv: +CREG:0,1
Recv: OK

Send: AT+CSQ\r   ����ź�ǿ��
Recv: +CSQ: 24,0   �� +CSQ: 99,99
Recv: OK

Send: AT+ISP=0\r   ����Ϊ�ڲ�Э��ջ��Ĭ��Ϊ�ⲿ�ģ����Ա������ã�
Recv: OK

Send: AT+CGDCONT=1,"IP","CMNET"\r   ����APN
Recv: OK

Send: AT+XGAUTH=1,1,"NAME","PWD"\r  �����֤����Ҫʱʹ�ã�
Recv: OK

Send: at+xiic=1   //����PPP����
Recv: OK

Send: AT+TCPSETUP=0,210.21.94.88,9000 ����TCP����
Recv: OK


Send: at+tcpsend=0,10 // ��TCP �����Ϸ�������
Recv: >
Send: 0123456789 \r
Recv:OK
Recv:+TCPSEND��0,10 //���ݷ��ͳɹ�
Recv:at+ipstatus=0

Recv:+TCPRECV:0,10,xxxxxxx \r  \\��������




Send: AT+IPR=9600\r  ����modemͨ�Ų�����
Recv: OK
Send: AT&F\r         ��������
Recv: OK

Send: AT^SMSO=?\r   �ܷ���λ
Recv: OK
Send: AT^SMSO\r     ������λ
Recv: ^SMSO: MS OFF
Recv: OK
Recv: ^SHUTDOWN

Send: AT^SCKS?\r   ���SIM��
Recv: ^SCKS: 0,1\r OK\r

Send: AT+CXXCID    ��� SIM���Ƿ���ȷ
Recv: +CXXCID: 89860064190713698640

Send: AT+CNMI=3,1,0,0,1\r ���Ž���λ��


Send: AT+CMGF=1\r   \\���ŷ��ͷ�ʽ


Send: AT+CMGS=%s\r \\���Ͷ���Ŀ�����
Recv: >
Send: %s(0x1A)     \\���Ͷ�������
Recv: +CMGS: 203   \\���ͳɹ�
Recv: OK

Recv: +CMTI: "MT",1 \\���յ�����Ϣ��ʾ
Send: AT+CMGR=1     \\��ȡ����Ϣ
Recv: +CMGR: "REC UNREAD","+8613640251499",,"08/02/26,21:50:56+32"
Recv: (/SEND 01:1001;02:0001;03:00C/)
Recv: OK
Send: AT+CMGD=1  \\ɾ������Ϣ

Send: at+cgdcont=1,"ip","cmnet"   GPRS��������
Recv: OK

Send: ATDT*99***1#    GPRS����
Recv: CONNECT
*/

