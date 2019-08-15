#include "comm.h"
#include <sys/mman.h>
#include <sys/time.h>   /* Time structures for select() */

#define TCP     0
#define UDP     1
typedef struct
{
	c8 *command;      /*the AT Command we send to Modem*/
	c8 *result;       /*the result we expected*/
	u8 timeout;       /*MAX time of second that we could waiting*/
	u8 r_time;        /*MAX times that we could re_send to Modem*/
}AT_COMMAND;

u8 GPRS_ip[4];             //HEX GPRS拨号后的IP地址 3   ASCII
int Gprs_fd = 0;           //与modem连接的串口
u8 Flag_mpwr = 0;          //modem口的打开状态

extern struct parameter_set ParaSet;
extern u8 SGPRS_flag;
extern u8 Sturnon_count;
extern u8 Sdail_count;
extern u8 Scont_count;
extern u8 Scommfail_count;
extern time_t Scommfail_time;

u8 check_AT_return(void);
void reset_modem(void);
u8 at_comm_wait(u8 *str, u8 len, u8 wait);

#define MAP_SIZE		4096UL
#define MAP_MASK		(MAP_SIZE - 1)
#define ADDR_MASK		0xfffff000

#define GPIO_START_ADDR 	0xB8003000

//LED toggle use PG5

#define GPIOF_DIR				0xB8003140
#define GPIOF_DOUT				0xB8003144
#define GPIOF_DIN    			0xB8003148
#define GPIOF_PUEN				0xB8003160

#define GSM_VDD_GPIOF14_PIN_NUM          (14)
#define PWD_ON_GPIOF13_PIN_NUM          (13)


static void *map_base_gpio = NULL;

static int mem_fd = -1;

static void mem_write(unsigned int addr, unsigned int value)
{
	volatile unsigned int *virt_addr;
	if ( (addr & ADDR_MASK) == GPIO_START_ADDR)
	{
		virt_addr = map_base_gpio + (addr & MAP_MASK);
		*virt_addr =  value;
	}
}

static unsigned int mem_read(unsigned int addr)
{
	volatile unsigned int *virt_addr;
	if ( (addr & ADDR_MASK) == GPIO_START_ADDR)
	{
		virt_addr = map_base_gpio + (addr & MAP_MASK);
		return *virt_addr;
	}
	return 0;
}

s32 gpio_init()
{
    unsigned int value;

    if((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) 
    	return 0;
    map_base_gpio = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_START_ADDR & ~MAP_MASK);
    if(map_base_gpio == MAP_FAILED)
	{
		close(mem_fd);
		mem_fd = -1;
    	return 0;
	}

    //PF13 out pullup
	value = mem_read(GPIOF_DIR);
	mem_write(GPIOF_DIR,value | (1 << PWD_ON_GPIOF13_PIN_NUM));		

    value = mem_read(GPIOF_PUEN);
	mem_write(GPIOF_PUEN, value | (1 << PWD_ON_GPIOF13_PIN_NUM));
    
	value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (1 << PWD_ON_GPIOF13_PIN_NUM));

    //PF14 out pullup
	value = mem_read(GPIOF_DIR);
	mem_write(GPIOF_DIR,value | (1 << GSM_VDD_GPIOF14_PIN_NUM));		

    value = mem_read(GPIOF_PUEN);
	mem_write(GPIOF_PUEN, value | (1 << GSM_VDD_GPIOF14_PIN_NUM));
    
	value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (1 << GSM_VDD_GPIOF14_PIN_NUM));

	return 1;
}
void set_gsm_vdd_ctrl_off(void)
{
    unsigned int value;

    value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (1 << GSM_VDD_GPIOF14_PIN_NUM));
}
void set_gsm_vdd_ctrl_on(void)
{
    unsigned int value;

    value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (0 << GSM_VDD_GPIOF14_PIN_NUM));
}


void set_gsm_pwm_on(void)
{
    unsigned int value;

    value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (0 << PWD_ON_GPIOF13_PIN_NUM));
    sleep(1);
    value = mem_read(GPIOF_DOUT);
	mem_write(GPIOF_DOUT, value | (1 << PWD_ON_GPIOF13_PIN_NUM));
}

void reopen_gsm(void)
{
    set_gsm_vdd_ctrl_off();
    sleep(1);
    set_gsm_vdd_ctrl_on();
    set_gsm_pwm_on();
    
    return;
}
static void mem_close()
{
	if (map_base_gpio != NULL)
		munmap(map_base_gpio, MAP_SIZE);

	if (mem_fd != -1)
		close(mem_fd);
	map_base_gpio = NULL;
	mem_fd = -1;
}

void gpio_write_high(c8 *port_pin)
{
    return;
}
void gpio_write_low(c8 *port_pin)
    {
        return;
    }

u8 gpio_read_pin(c8 *port_pin)
    {
        return;
    }

s32 set_mcb_multcate(void)
{
    return 0;
}

void GPRS_pwr_on(void)
{
	write_log(MSG_INFO, "GPRS_pwr_on\n");
	gpio_write_high(MPWR);
	Flag_mpwr = 1;
	sleep(1);
}


void GPRS_pwr_off(void)
{
	write_log(MSG_INFO, "GPRS_pwr_off\n");
	gpio_write_low(MPWR);
	Flag_mpwr = 0;
	sleep(1);
}


u8 turn_on_modem(void)
{
	if( check_AT_return() != 0 )
	{
		gpio_write_high(MON_OFF);
		sleep(2);
		gpio_write_low(MON_OFF);
		sleep(3);

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
		sleep(2);
		gpio_write_low(MON_OFF);
		sleep(3);

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
			write_log(MSG_INFO, "Gprs recv: %s\n",buf);
			if(len != 0)
			{
				for(i=0; i<=pt-len; i++)
				{
					if( str_comp(buf+i, str, len) == 0 )
					{
						return 0;
					}
				}
			}
		}
	}
	return 1;
}
u8 get_sim_id(void)
{
	u8 buf[512];
	u16 pt = 0;
	int ret;

	write(Gprs_fd, "AT+CCID\r", 9);    //获取本机ip地址
	write_log(MSG_DEBUG, "Gprs send: AT+CCID\r" );
	if(1)
	{
		ret = myselect(Gprs_fd, 150);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 512-pt);
		if (ret <= 0) return 1;
		pt += ret;

		buf[pt]= '\0';
		write_log(MSG_INFO, "Gprs_recv sim id: %s\n",buf);
	}
	return 1;
}


u8 GPRS_AT_Init(void)
{
	int i;

	AT_COMMAND initATstr[] =
	{
		{ "ATE0\r\n",                                           "OK",   5,   2 },  //关闭回显
		{ "AT+CSQ\r\n",                                         "OK",   5,   3 },  //查询信号强度

		{ NULL, NULL, 0, 0},
	};

	AT_COMMAND *pATstr = initATstr;

	while(pATstr->command != NULL)
	{
		for(i = 0; i < pATstr->r_time; i++)
		{
			write(Gprs_fd, pATstr->command, strlen(pATstr->command) );
			write_log(MSG_DEBUG, "Gprs send: %s\n", pATstr->command );
			if(at_comm_wait( pATstr->result, strlen(pATstr->result), pATstr->timeout*100) == 0)
			{
				break;
			}
            
		}

		if (i >= pATstr->r_time)
		{
            printf("GPRS_AT_Init ret 2\n");
			return 2;
		}
		usleep(500000);
		pATstr++;
	}
    get_sim_id();
    printf("GPRS_AT_Init ret 0\n");
	return 0;
}

u8 GPRS_AT_Connect(void)
{
	int i;
	c8 CIPCSGPcmd[128];
	c8 CSTTcmd[128];

	AT_COMMAND initATstr[] =
	{
		//{ CIPCSGPcmd,                                           "OK",   5,   3},   //GPRS连接设置参数
#if TCP
		//{ "AT+CLPORT=\"TCP\",\"0\"\r\n",                        "OK",   5,   3},   //设置TCP本地端口号
#else
		//{ "AT+CLPORT=\"UDP\",\"0\"\r\n",                        "OK",   5,   3},   //设置UDP本地端口号
#endif
		{ "AT+CIPMUX=1\r\n",                                    "OK",   5,   3},   //启动多IP连接
		{ CSTTcmd,                                              "OK",   5,   3},   //启动任务并设置接入点APN, 用户名，密码？？？
		{ "AT+CIICR\r\n",                                       "OK",   5,   3},   //激活移动场景，发起GPRS或CSD无线连接
		{ "AT+CIPQSEND=0\r\n",                                  "OK",   5,   3},   //选择数据发送模式
		{ NULL, NULL, 5, 0},
	};

	AT_COMMAND *pATstr = initATstr;

	//sprintf(CIPCSGPcmd, "AT+CIPCSGP=1,\"%s\",\"%s\",\"%s\"\r\n", ParaSet.APN, ParaSet.APNName, ParaSet.APNPWD);
	sprintf(CSTTcmd, "AT+CSTT=\"%s\"\r\n", ParaSet.APN);
	
	while(pATstr->command != NULL)
	{
		for(i = 0; i < pATstr->r_time; i++)
		{
			write(Gprs_fd, pATstr->command, strlen(pATstr->command) );
			write_log(MSG_DEBUG, "Gprs send: %s\n", pATstr->command );
			if(at_comm_wait( pATstr->result, strlen(pATstr->result), pATstr->timeout*100) == 0)
			{
				break;
			}
		}

		if (i >= pATstr->r_time)
		{
			return 2;
		}

		usleep(500000);
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
		ret = myselect(Gprs_fd, 150);
		if (ret <= 0) return 1;
		usleep(10000);
		ret = read(Gprs_fd, buf + pt, 512-pt);
		if (ret <= 0) return 1;
		pt += ret;

		buf[pt]= '\0';
		write_log(MSG_INFO, "Gprs_recv: %s\n",buf);

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
	}
	return 1;
}


//拨号连接GPRS网络
//该函数按通信失败次数， 重连接次数， 重拨号次数进行不同的操作，具体见拨号流程图
u8 connect_GPRS_net(void)
{
	static u8 turn_modem_fail = 0;	
	c8 sbuf[128];
	u8 i;

	SGPRS_flag = CONNECTING;

	clearport(Gprs_fd);

	write_log(MSG_INFO, "Connect GPRS net...\n");

	write(Gprs_fd, "\r", 1);
	write_log(MSG_DEBUG, "Gprs send: \r" );
	at_comm_wait(NULL, 0, 10);

	if(Scommfail_count > 6)  //通信失败次数
	{
		//断开TCP或UDP链接
		if(gpio_read_pin(MPWR) == 1)
		{
			write(Gprs_fd, "AT+CIPCLOSE=0\r", 14);    //断开TCP或UDP链接
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
		if(gpio_read_pin(MPWR) == 1)
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
	GPRS_pwr_off();
	sleep(10);
	GPRS_pwr_on();
	sleep(1);
	reset_modem();  //实际没焊接
	sleep(1);

TURN_ON_MODEM:

	if(Sturnon_count < 0x0F) Sturnon_count ++;
	if( turn_on_modem() != 0)   //modem电源打开失败
	{		
		turn_modem_fail ++;
		GPRS_pwr_off(); 
		sleep(turn_modem_fail*10); 
		return 1; 
	}
	turn_modem_fail = 0;
	sleep(4);

	//关闭回显
	//查询信号强度	
	if (0 != GPRS_AT_Init() )
	{
		return 2;
	}
	sleep(2);


PPP_CONNECT:   //进行PPP连接
	if(Sdail_count < 0x0F) Sdail_count ++;
	
	//设置根据指定的移动台类别工作
	for(i=0; i<2; i++)  
	{
		write(Gprs_fd, "AT+CGCLASS=\"B\"\r\n", 16);
		write_log(MSG_DEBUG, "Gprs send: AT+CGCLASS=\"B\"\r\n");
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}

	//用于设置附着GPRS业务
	for(i=0; i<2; i++)
	{
		write(Gprs_fd, "AT+CGATT=1\r\n", 12);
		write_log(MSG_DEBUG, "Gprs send: AT+CGATT=1\r\n");
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	//if(i >= 2) return 7;  //不判断是否能附着

	//判断模块是否注册上网络
	for(i=0; i<8; i++)
	{
		write(Gprs_fd,"AT+CGREG?\r\n", 11);
		write_log(MSG_COMMT+MSG_DEBUG, "Gprs send: AT+CGREG?\r" );
		if(at_comm_wait(",1", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
		
		write(Gprs_fd,"AT+CGREG?\r\n", 11);
		write_log(MSG_COMMT+MSG_DEBUG, "Gprs send: AT+CGREG?\r" );
		if(at_comm_wait(",5", 2, 100) == 0) 
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	if(i >= 8) return 3;

	GPRS_AT_Connect();

	//获取IP地址
	for(i=0; i<5; i++)
	{
		if(get_GPRS_ip() != 0)  return 8;
		if( (GPRS_ip[0] != 0) || (GPRS_ip[1] != 0) || (GPRS_ip[2] != 0) || (GPRS_ip[3] != 0) )
		{
			write_log(MSG_INFO, "IP = %d.%d.%d.%d \r\n", GPRS_ip[0], GPRS_ip[1], GPRS_ip[2],GPRS_ip[3]);
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
	sprintf(sbuf, "AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d\r\n", *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.serport);
	//sprintf(sbuf, "AT+CIPSTART=0,\"TCP\",\"%d.%d.%d.%d\",%d\r\n", 59, 42, 107, 151, 51236);
#else
	sprintf(sbuf, "AT+CIPSTART=0,\"UDP\",\"%d.%d.%d.%d\",%d\r\n",  *((u8 *)&ParaSet.SRVRaddr + 3), *((u8 *)&ParaSet.SRVRaddr + 2), *((u8 *)&ParaSet.SRVRaddr + 1), *((u8 *)&ParaSet.SRVRaddr + 0), ParaSet.SRVRport);
	//sprintf(sbuf, "AT+CIPSTART=0,\"UDP\",\"%d.%d.%d.%d\",%d\r\n", 59, 42, 107, 151, 51236);
#endif
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_DEBUG, "Gprs send: %s", sbuf);
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

	SGPRS_flag = CONNECTOK;
	Scommfail_count = 0;
	Scommfail_time = time(NULL);

	return 0;
}


void GPRS_send_data(u8 *buf, int len)
{
	u8 sbuf[32];

	//printf("GPRS_send_data... flag=%d, Scommfail_count=%d\n", SGPRS_flag, Scommfail_count);

	if(SGPRS_flag != CONNECTOK)
	{
		if(SGPRS_flag == CONNECTING)  //正在连接网络
		{
			return;
		}
		else
		{
			SGPRS_flag = RECONNECT;
			return;
		}
	}

	if(Scommfail_count > 6)
	{
		SGPRS_flag = RECONNECT;
	}

	if(abs(time(NULL) - Scommfail_time) > 20)    
	{
		Scommfail_count ++;
		Scommfail_time = time(NULL);
	}		

	sprintf(sbuf, "AT+CIPSEND=0,%d\r\n", len);
	write(Gprs_fd, sbuf, strlen(sbuf));
	write_log(MSG_DEBUG, "Gprs send: %s", sbuf );

	if(at_comm_wait(">", 1, 50) != 0)
	{
		//delayTimemS(200);???
	}
	
	write(Gprs_fd, buf, len);
	sbuf[0] = '\r';
	sbuf[1] = '\n';
	write(Gprs_fd, sbuf, 2);

	write_comm(MSG_COMMR+MSG_DEBUG, "Gprs send:", buf, len + 2);
	record_comm_msg('S', buf, len);
}

/*
void hearbeat_server(void)
{
	u8 outbuf[16];

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
*/
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


	sleep(1);
	
	for(i=0; i<2; i++)  
	{
		write(Gprs_fd, "AT+CSTT?\r\n", 10);
		write_log(MSG_DEBUG, "Gprs send: AT+CSTT?\r\n");
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CIPSHUT\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", "unim2m.njm2mapn");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CIPCSGP=1,\"%s\",\"%s\",\"%s\"\r\n", "unim2m.njm2mapn", "", "");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	

//设置根据指定的移动台APN
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CIPSHUT\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}


	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CSTT=\"unim2m.njm2mapn\",\"\",\"\"\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		write(Gprs_fd, "AT+CSTT?\r\n", 10);
		write_log(MSG_DEBUG, "Gprs send: AT+CSTT?\r\n");
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT&V\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT&W\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	
	//关闭回显
	//查询信号强度	
	if (0 != GPRS_AT_Init() )
	{
		return 2;
	}

	
	//设置根据指定的移动台APN
	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CIPSHUT\r\n");
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}


	for(i=0; i<2; i++)  
	{
		sprintf(sbuf, "AT+CSTT=\"%s\"\r\n", ParaSet.APN);
		write(Gprs_fd, sbuf, strlen(sbuf));
		write_log(MSG_DEBUG, sbuf);
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	for(i=0; i<2; i++)  
	{
		write(Gprs_fd, "AT+CSTT?\r\n", 10);
		write_log(MSG_DEBUG, "Gprs send: AT+CSTT?\r\n");
		if(at_comm_wait("OK", 2, 100) == 0)
		{
			break;
		}
		at_comm_wait(NULL, 0, 100+i*15);
	}
	
	
	
	


*/
/*完整拨号流程
Connect GPRS net...
GPRS_pwr_off
GPRS_pwr_on
Gprs send: AT
Gprs send: AT
Gprs recv: IIII??
Gprs recv: IIII??AT

OK

Gprs send: ATE0

Gprs recv: ATE0

OK

Gprs send: AT+CSQ

Gprs recv: 
+CSQ: 29,0

OK

Gprs send: AT+CGCLASS="B"
Gprs recv: 
OK

Gprs send: AT+CGATT=1
Gprs recv: 
OK

Gprs recv: 
+CGREG: 0,1

OK

Gprs send: AT+CIPMUX=1

Gprs recv: 
OK

Gprs send: AT+CSTT="CMNET"

Gprs recv: 
OK

Gprs send: AT+CIICR

Gprs recv: 
OK

Gprs send: AT+CIPQSEND=0

Gprs recv: 
OK

Gprs_recv: AT+CIFSR
10.32.80.229

IP = 10.32.80.229 
Gprs send: AT+CIPSTART=0,"UDP","121.43.109.89",7007
Gprs recv: 
OK

0, CONNECT OK

Gprs send: AT+CIPSTATUS=0
Gprs recv: 
+CIPSTATUS: 0,0,"UDP","121.43.109.89","7007","CONNECTED"

OK

Gprs send: 68000101ca01003b0201ea0101440f0c0e0e08020e1a0e170e180e170e150e110e0e0e0f0e110e110e100e120e1d0e180e0f0e100e110e110e0f0e100e140e160e130e120e140e110e0f0e0c0e080e08a3168fb1
Gprs send: AT+CIPSEND=0,50
Gprs recv: 
> 
Gprs send: 68020101ca01003b0101ca01003b0f0c0e0f090046414e464341303320686561727420746f20736572766572203030305b16110e
GPRS recv:  0d0a302c2053454e44204f4b0d0a0d0a2b524543454956452c302c32303a0d0a68800201ea0102650f0c0e0f092c00120000bc16
2G rmsg: 68800201ea0102650f0c0e0f092c00120000bc16
*/
