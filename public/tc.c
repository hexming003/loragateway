/*
 * Copyright 2008, Power Division, Sunrise Revenco.
 * All rights reserved.
 *
 * Filename : gpio.h
 */

/* Macro definitions */
#define PORT_A 0x400
#define PORT_B 0x600
#define PORT_C 0x800
#define DEBUG_TEST 0
#if DEBUG_TEST
#define TC0  0x00
#define TC1  0x40
#define TC2  0x80
#define TC_BCR  0xC0
#define TC_BMR  0xC4
#define PMCBASE 0xC00

#define PMC_SCER 0x0000             //SYSTYEM clock enalbe register
#define PMC_SCDR 0x0004             //SYSTYEM clock disalbe register
#define PMC_SCSR 0x0008             //SYSTYEM clock status register
#define PMC_PCER 0x0010             //Peripheral clock enable register
#define PMC_PCDR 0x0014             //Peripheral clock disable register
#define PMC_PCSR 0x0018             //Peripheral clock status register
#define PMC_MCKR 0x0030
#define PMC_PCK1 0x0044             //programmable clock 1 register

#define AT91_PWM_CCR        0x00
#define AT91_PWM_CMR        0x04
	#define TCCLKS(val) ((val) << 0)
	#define CLKI(val) ((val) << 3)
	#define BURST(val) ((val) << 4)
	#define CPCSTOP(val) ((val) << 6)
	#define CPCDIS(val) ((val) << 7)
	#define EEVTEDG(val) ((val) << 8)
	#define EEVT(val) ((val) << 10)
	#define ENETRG(val) ((val) << 12)
	#define WAVSEL(val) ((val) << 13)
	#define WAVE(val) ((val) << 15)
	#define ACPA(val) ((val) << 16)
	#define ACPC(val) ((val) << 18)
	#define AEEVT(val) ((val) << 20)
	#define ASWTRG(val) ((val) << 22)
	#define BCPB(val) ((val) << 24)
	#define BCPC(val) ((val) << 26)
	#define BEEVT(val) ((val) << 28)
	#define BSWTRG(val) ((val) << 30)

#define AT91_PWM_RA         0x14
#define AT91_PWM_RB         0x18
#define AT91_PWM_RC         0x1C
#define AT91_PWM_SR         0x20
#define AT91_PWM_IER        0x24
#define AT91_PWM_IDR        0x28
#define AT91_PWM_IMR        0x2C
#endif
/* Macro definitions */
#define MEM_DEV         "/dev/mem"

#if DEBUG_TEST
#define AT91_PIO_MAPADDR    0xFFFFF000
#define PIO_REG_SIZE        0x1000

#define AT91_TC_MAPADDR     0xFFFA0000
#define TC_REG_SIZE         0x1000

#define TCHZ(x)   (2831000/(x))
#endif

#include "public.h"

u8 *gpioaddr = NULL;
u8 *tcaddr = NULL;


// ����: ��mem�豸�� ��ӳ������GPIO�ռ��ַ
// ����: ��
// ���: ��
// ����: -1 :ʧ��
//        0 :�ɹ�
s32 gpio_init(void)
{
	s32 iofd = -1;

	iofd = open(MEM_DEV, O_RDWR | O_SYNC);
	if (iofd == -1)
	{
		usleep(100000);
		iofd = open(MEM_DEV, O_RDWR | O_SYNC);
		if (iofd == -1)
		{
			return -1;
		}
	}

	gpioaddr = (u8*)mmap((void *)0x0, PIO_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_PIO_MAPADDR);

	if (gpioaddr == MAP_FAILED)
	{
		usleep(100000);
		gpioaddr = (u8*)mmap((void *)0x0, PIO_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_PIO_MAPADDR);
		if (gpioaddr == MAP_FAILED)
		{
			close(iofd);
			return -1;
		}
	}
	close(iofd);

	gpio_write_low(PIN_WDOG);
	gpio_enable_write(PIN_WDOG);
	
	gpio_enable_write(INET_RST);
	gpio_write_high(INET_RST);

	gpio_write_high(PLCM_RST);  //�ز�ģ�鸴λ
	gpio_write_high(RUN_LED);   //״ָ̬ʾ��
	gpio_enable_write(PLCM_RST);
	gpio_enable_write(RUN_LED);

	gpio_enable_read(MPWR_DET);
	gpio_enable_write(MBAT_CTRL);
	gpio_write_high(MBAT_CTRL);

#ifdef __COMM
	gpio_enable_write(MPWR);
	gpio_write_low(MPWR);

	gpio_enable_write(MRST);
	gpio_write_high(MRST);
	
	gpio_write_low(MON_OFF);
	gpio_enable_write(MON_OFF);	
#endif
	return 0;
}
#if DEBUG_TEST


/******************����38K�����������***********************/
/*
 * Function Name    : tc_init
 * Description      : ��ʼ��TC2��Ҫ�õ���ʱ�ӡ��ܽš����ƵȼĴ�����ͬʱ��Ҫ
 *                     GPIOģ�����ϣ�������÷ŵ�gpio_init֮��
 * Input        : N/A
 * Output       : N/A
 * Return       : -1 :false init
 *                 1 :true  init
*/
int tc_init(u32 tc_hz)
{
	s32 iofd = -1;

	iofd = open(MEM_DEV, O_RDWR | O_SYNC);
	if (iofd == -1)
	{
		usleep(100000);
		iofd = open(MEM_DEV, O_RDWR | O_SYNC);
		if (iofd == -1)
		{
			return -1;
		}
	}

	/* ӳ���ڴ��ַ */
	tcaddr = (u8*)mmap((void *)0x0, TC_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_TC_MAPADDR);
	if (tcaddr == MAP_FAILED)
	{
		usleep(100000);
		tcaddr = (u8*)mmap((void *)0x0, TC_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_TC_MAPADDR);
		if (tcaddr==MAP_FAILED)
		{
			write_log(MSG_SYSERR, "tc mmap error!\n");
			close(iofd);
			return -1;
		}
	}

	close(iofd);

	//�ر�TC1������ʱ��
	*(u32 *)(gpioaddr + PMCBASE + PMC_PCDR) = (1<<18);

	//����PA27(TIOA1)ΪA���ܽ�
	*(u32 *)(gpioaddr + PORT_A + PIO_PDR) = (1L<<27); //�ر�IO
	*(u32 *)(gpioaddr + PORT_A + PIO_ASR) = (1L<<27); //����A����
	//����PC7(TIOB1)ΪA���ܽ�
	*(u32 *)(gpioaddr + PORT_C + PIO_PDR) = (1L<<7); //�ر�IO
	*(u32 *)(gpioaddr + PORT_C + PIO_ASR) = (1L<<7); //����A����

	//TC0/1/2ͨ���Ŀ�����
	*(u32 *)(tcaddr + TC_BCR) = 0;
	*(u32 *)(tcaddr + TC_BMR) = 0x15; //�ر��ⲿʱ��
	usleep(1000);

	//�ر�TC1���
	*(u32 *)(tcaddr + TC1 + AT91_PWM_CCR) = 2;
	usleep(1000);

	//����CMR
	*(u32 *)(tcaddr + TC1 + AT91_PWM_CMR) =  // Waveform Mode (datasheet page 535/788)
			TCCLKS(2) | // TIMER_CLOCK5 (SLCK)
			CLKI(0) | // Rising edge clock
			BURST(0) | // The clock is not gated by an external signal
			CPCSTOP(0) | // Counter clock is not stopped when counter reaches RC
			CPCDIS(0) | // Counter clock is not disabled when counter reaches RC
			EEVTEDG(0) | // External Event Edge Selection = none
			EEVT(1) | // Signal selected as external event = XC0
			ENETRG(0) | // The external event has no effect on the counter and its clock
			WAVSEL(2) | // UP mode with automatic trigger on RC Compare
			WAVE(1) | // Waveform Mode is enabled
			ACPA(1) | // RA Compare Effect on TIOA = set
			ACPC(2) | // RC Compare Effect on TIOA = clear
			AEEVT(0) | // External Event Effect on TIOA = none
			ASWTRG(2) | // Software Trigger Effect on TIOA = clear
			BCPB(1) | // RB Compare Effect on TIOB = set
			BCPC(2) | // RC Compare Effect on TIOB = clear
			BEEVT(0) | // External Event Effect on TIOB = none
			BSWTRG(2); // Software Trigger Effect on TIOB = clear

		//����Ƶ�� ռ�ձ����ﶨ��û�ã���Ҫ����TC_on�ﶨ��
		*(u32 *)(tcaddr + TC1 + AT91_PWM_RA) = 0;
		usleep(1000);
		*(u32 *)(tcaddr + TC1 + AT91_PWM_RB) = 0;
		usleep(1000);
		*(u32 *)(tcaddr + TC1 + AT91_PWM_RC) = TCHZ(tc_hz);
		usleep(1000);

	//write_log(MSG_INFO, "RA = %d\n", *(u32 *)(tcaddr + TC1 + AT91_PWM_RA));
	//write_log(MSG_INFO, "RB = %d\n", *(u32 *)(tcaddr + TC1 + AT91_PWM_RB));
	//write_log(MSG_INFO, "RC = %d\n", *(u32 *)(tcaddr + TC1 + AT91_PWM_RC));


	//IRQ
	*(u32 *)(tcaddr + TC1 + AT91_PWM_IDR) = 0xff;

	usleep(1000);

	return 1;
}

/*
 * Function Name    : tc_on
 * Description      : ��TC1��PWM���
 * Input        : occ  ռ�ձ�
 * Output       : N/A
 * Return       : N/A
*/
void tcA_on(u8 ooc)
{
	u32 temp;

	if(ooc == 0) ooc = 1;
	if(ooc >= 100) ooc = 99;

	//����TC2������ʱ��
	*(u32 *)(gpioaddr + PMCBASE + PMC_PCER) |= (1<<18);
	usleep(1000);
	//����TC2���
	*(u32 *)(tcaddr + TC1 + AT91_PWM_CCR) = 5;
	usleep(1000);
	temp = *(u32 *)(tcaddr + TC1 + AT91_PWM_RC);
	temp = temp * (100 - ooc) / 100;
	*(u32 *)(tcaddr + TC1 + AT91_PWM_RA) = temp;
	usleep(1000);

	write_log(MSG_RUNING, "TCA had ON\n");
	return;
}


void tcB_on(u8 ooc)
{
	u32 temp;

	//����TC2������ʱ��
	*(u32 *)(gpioaddr + PMCBASE + PMC_PCER) |= (1<<18);
	usleep(1000);
	//����TC2���
	*(u32 *)(tcaddr + TC1 + AT91_PWM_CCR) = 5;
	usleep(1000);
	temp = *(u32 *)(tcaddr + TC1 + AT91_PWM_RC);
	if(ooc == 0) ooc = 1;

	if(ooc >= 100)
		temp = 1;
	else
		temp = temp * (100 - ooc) / 100;
	*(u32 *)(tcaddr + TC1 + AT91_PWM_RB) = temp;
	usleep(1000);

	write_log(MSG_RUNING, "TCB had ON\n");
	return;
}

//�޸�TCB��ֵ
void tcB_val(u8 ooc)
{
	u32 temp;

	temp = *(u32 *)(tcaddr + TC1 + AT91_PWM_RC);

	//if(ooc == 0) ooc = 1;

	if(ooc >= 100)
		temp = 1;
	else
		temp = temp * (100 - ooc) / 100;

	*(u32 *)(tcaddr + TC1 + AT91_PWM_RB) = temp;

	return;
}

void tcA_off(void)
{
	*(u32 *)(tcaddr + TC1 + AT91_PWM_RA) = 0;
}

void tcB_off(void)
{
	*(u32 *)(tcaddr + TC1 + AT91_PWM_RB) = 0;
}



/*
 * Function Name    : tc_off
 * Description      : �ر�TC1��PWM���
 * Input        : N/A
 * Output       : N/A
 * Return       : N/A
*/
void tc_off(void)
{
	//�ر�TC1���
	*(u32 *)(tcaddr + TC1 + AT91_PWM_CCR) = 2;
	usleep(1000);
	//�ر�TC1������ʱ��
	*(u32 *)(gpioaddr + PMCBASE + PMC_PCDR) |= (1<<18);

	return;
}

#endif


// ����: ���ַ���arg�����Ĳ��� ת���ɻ�����ַ(IOaddr) �� ���ź�(tPin)
// ����: arg:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: IO:   �˿�A,B,C����PA�ڵ����ƫ�� ��PORT_A PORT_B PORT_C
//       tPin: ���ź�
// ����: 0 if ok, otherwise -1 is returned
s32 get_io_info(c8 *arg, u32 *IOaddr, s32 *tPin)
{
	s32 tempPin;

	if ((arg[0] == 'P') || (arg[0] == 'p'))
	{
		tempPin = atoi(arg + 2);
		if ((tempPin < 32) && (tempPin >= 0))
		{
			if ((arg[1] == 'A') || (arg[1] == 'a'))
			{
				*IOaddr = PORT_A;
				*tPin = tempPin;
				return 0;
			}
			else if ((arg[1] == 'B') || (arg[1] == 'b'))
			{
				*IOaddr = PORT_B;
				*tPin = tempPin;
				return 0;
			}
			else if ((arg[1] == 'C') || (arg[1] == 'c'))
			{
				*IOaddr = PORT_C;
				*tPin = tempPin;
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}


// ����: ��������Ϊ����ģʽ
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_enable_read(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_PER) = (1L << pin_no);
		*(u32 *)(gpioaddr + io_addr + PIO_ODR) = (1L << pin_no);
	}
}

// ����: ��������Ϊ���ģʽ
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_enable_write(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_PER) = (1L << pin_no);
		*(u32 *)(gpioaddr + io_addr + PIO_OER) = (1L << pin_no);
	}
}


// ����: �����������ŵ���������
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_enable_upregister(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_PUER) = (1L << pin_no);
	}
}


// ����: ��ֹ�������ŵ���������
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_disable_upregister(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_PUDR) = (1L << pin_no);
	}
}


// ����: ��IO���ŵ�ƽ
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: 0 �͵�ƽ  1 �ߵ�ƽ
u8 gpio_read_pin(c8 *port_pin)
{
	u32 io_addr;
	s32 pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		if( (*(u32 *)(gpioaddr + io_addr + PIO_PDSR)) & (1L<<pin_no) )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	return 0xff;
}


// ����: ������������ߵ�ƽ
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_write_high(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_SODR) = (1L << pin_no);
	}
}

// ����: ������������͵�ƽ
// ����: port_pin:  ���ַ������������ź� �� "PA7" "PB21" "PC30"
// ���: ��
// ����: ��
void gpio_write_low(c8 *port_pin)
{
	u32 io_addr;
	s32  pin_no;

	if(get_io_info(port_pin, &io_addr, &pin_no) == 0)
	{
		*(u32 *)(gpioaddr + io_addr + PIO_CODR) = (1L << pin_no);
	}
}

#if DEBUG_TEST

/*
Linux���õ�3����ʱ��
LinuxΪÿ����������3���ڲ���ʱ����
ITIMER_REAL��ʵʱ��ʱ�������ܽ����ں���ģʽ�����У������ڽ��̱�����ʱ���������ڼ�������ʱ�������̷���SIGALRM�źš�
ITIMER_VIRTUAL���������ʵʱ��ʱ�������������û�ģʽ��������ִ��ʱ���������ִ�е�ʱ�䡣��ʱ�������ý��̷���SIGVTALRM�źš�
ITIMER_PROF���������û�ģʽ��������ִ��ʱ���ͺ���ģʽ�������̵�����ʱ������������ʱ�������SIGPROF�źš�ITIMER_PROF��¼��ʱ���ITIMER_VIRTUAL���˽��̵���������ʱ�䡣
��ʱ���ڳ�ʼ���ǣ�������һ����ʼֵ����ʱ��ݼ����ݼ���0�󷢳��źţ�ͬʱ�ָ���ʼֵ���������У����ǿ���һ�ֻ���ȫ�����ֶ�ʱ������ͬһʱ��ͬһ���͵Ķ�ʱ��ֻ��ʹ��һ����

�õ��ĺ����У�
int getitimer(int which, struct itimerval *value);
int setitimer(int which, struct itimerval*newvalue, struct itimerval* oldvalue);
strcut timeval
{
long tv_sec; //��
long tv_usec; //΢��
};
struct itimerval
{
struct timeval it_interval; //ʱ����
struct timeval it_value;    //��ǰʱ�����
};
it_interval����ָ��ÿ���೤ʱ��ִ������ it_value�������浱ǰʱ����ִ�������ж೤ʱ�䡣����˵�� ��ָ��it_intervalΪ2��(΢��Ϊ0)����ʼ��ʱ�����ǰ�it_value��ʱ��Ҳ�趨Ϊ2�루΢��Ϊ0����������һ�룬 it_value�ͼ���һ��Ϊ1�� �ٹ�1�룬��it_value�ּ���1����Ϊ0�����ʱ�򷢳��źţ������û�ʱ�䵽�ˣ�����ִ�������ˣ�������ϵͳ�Զ���it_value��ʱ������Ϊit_interval��ֵ����2�룬�����¼�����
����ITIMER_VIRTUAL��ITIMER_PROF��ʹ�÷������ƣ�������setitimer�������õĶ�ʱ��ΪITIMER_VIRTUAL��ʱ�����sigaction�����SIGALRM��ΪSIGVTALARM, ͬ��ITIMER_PROF��ӦSIGPROF��
*/


void timeout_info(int signo)
{
	tcA_off();
	//gpio_write_low(PIN_BEEP);
	write_log(MSG_INFO, "\n\n timeout info ....\n\n");
}

/* init sigaction */
void init_sigaction(int dtime)
{
	struct itimerval itv, oldtv;

	signal(SIGALRM, timeout_info);

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = dtime;

	setitimer(ITIMER_REAL, &itv, &oldtv);
}

#define AT91_MCB_MAPADDR  0xfffC4000
#define MCB_REG_SIZE  0x200
#define MCAB_NCFG  0x04
s32 set_mcb_multcate(void)
{
	s32 iofd = -1;
	u8 *mcbaddr = NULL;
	u32 mcbncfg;

	iofd = open(MEM_DEV, O_RDWR | O_SYNC);
	if (iofd == -1)
	{
		usleep(100000);
		iofd = open(MEM_DEV, O_RDWR | O_SYNC);
		if (iofd == -1)
		{
			return -1;
		}
	}

	mcbaddr = (u8*)mmap((void *)0x0, MCB_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_MCB_MAPADDR);
	if (mcbaddr == MAP_FAILED)
	{
		usleep(100000);
		mcbaddr = (u8*)mmap((void *)0x0, MCB_REG_SIZE, PROT_READ + PROT_WRITE, MAP_SHARED, iofd, (off_t)AT91_MCB_MAPADDR);
		if (mcbaddr == MAP_FAILED)
		{
			close(iofd);
			return -1;
		}
	}
	close(iofd);

	mcbncfg = (*(u32 *)(mcbaddr + MCAB_NCFG));
	mcbncfg |= 0x000000d0;
	(*(u32 *)(mcbaddr + MCAB_NCFG)) = mcbncfg;

	return 0;
}
#endif
