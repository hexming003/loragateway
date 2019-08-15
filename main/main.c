#include "lewin.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include <sys/types.h>
#include <sys/stat.h>
//#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <asm/ioctls.h>
#include <time.h>
#include <pthread.h>

#include "spidev.h"
#include "gpio.h"

#include "radio.h"
#include "sx1276-Hal.h"
#include "sx1276-Fsk.h"
#include "sx1276.h"
#include "sx1276-LoRa.h"
#include "sx1276-LoRaMisc.h"
#include "lora.h"

struct parameter_set  ParaSet;    //»´æ÷≤Œ ˝
struct nodelist_st NodeList[MAXNODENUM];
u8 LightParaLst[120];
u8 LightParalen = 0;
time_t Updatetimer = 0;
time_t IstimeSaveNodeList = 0xfffffff;
u8 InputNdAdd[6] = {0};
int FDW_pipe, FDR_pipe;

extern u8 nLogConsole;
extern u8 nDebugLevel;
extern int Plc_fd;
extern u8 SID;
extern u8 Default_LCfg[45];
extern uint8_t send_lora_req_data(lora_addr_t *pdest_addr);

void send_test_pipe(void);
/*
 * Á§∫‰æãÁ®ãÂ∫è‰∏∫ËØªMX25L1635E spiflashÁöÑidÂäüËÉΩ
*/

tRadioDriver *Radio;

void lora_init(void)
{
  //LoRaSettings.RFFrequency = sys_info.lora_frequency;
  Radio = RadioDriverInit();
  Radio->Init();
  SetTxTimeout(30);
  SetRxTimeout(30);
}
uint8_t lora_pro_buf[255];
uint16_t lora_pro_len;
uint8_t sx1278_printf_buf[2048];
void lora_rxtx_buf_print(uint8_t rxtx,uint8_t *buf,uint16_t len)//ÊúÄÂ§öÂèëÈÄÅÂèØ‰ª•512‰∏™Â≠óËäÇ
{
  	uint16_t i;
	uint16_t string_len=0;
	memset(sx1278_printf_buf,0,2048);
  	if(rxtx==0x01)
	{
	  sprintf(sx1278_printf_buf,"TX:");
	  string_len=strlen(sx1278_printf_buf);

	}
	else if(rxtx==0x0)
	{
		sprintf(sx1278_printf_buf,"RX:");

		string_len=strlen(sx1278_printf_buf);

	}
	 for (i = 0; i < len-1; i++)
	{
		sprintf(&sx1278_printf_buf[string_len+3 * i], "%02x-", buf[i]);
	}
	sprintf(&sx1278_printf_buf[string_len+3 * i], "%02x", buf[i]);
	sprintf(&sx1278_printf_buf[string_len+3 * i + 2],"\r\n");
	//TST_SendData(sx1278_printf_buf,strlen((char *)sx1278_printf_buf));
	printf("%s",sx1278_printf_buf);
}
uint8_t lora_transmit_frame(uint8_t *buf, uint32_t len)
{    
    volatile uint8_t status = RF_IDLE;         
    Radio->SetTxPacket(buf, len);    
    while ((status != RF_TX_DONE) && (status != RF_TX_TIMEOUT))    
    {        
        status = Radio->Process();    
    }    
    if (status == RF_TX_TIMEOUT)    
    {        
        Radio->Init();    
    }
    else
    {
        printf("           send ok\n");
    }
    SX1276LoRaSetOpMode(RFLR_OPMODE_STANDBY);    
	Radio->StartRx();
	Radio->Process();
    return status;
}

void system_init(void)
{
	u16 i;

	for(i=0; i<MAXNODENUM; i++)
	{
		memset((u8*)&NodeList[i], 0, sizeof(struct nodelist_st));
	}

	//gpio_init();

	init_param(&ParaSet);
	disp_paraset(&ParaSet);

	get_node_id();

	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //µΩ¡À¡–±ÌµƒŒ≤
		{
			break;
		}
		write_log(MSG_INFO, "node%03d: %02X %02X %02X %02X %02X %02X\r\n", i, NodeList[i].naddr[0], NodeList[i].naddr[1], NodeList[i].naddr[2], NodeList[i].naddr[3], NodeList[i].naddr[4], NodeList[i].naddr[5]);
	}

	Init_NamePipe(PIPE_MtoC, &FDR_pipe, &FDW_pipe);
}

/***********************************************************
*∫Ø ˝√˚£∫  main
*√Ë ˆ£∫    ÷˜∫Ø ˝
* ‰»Î≤Œ ˝£∫argc
		   *argv[]  µ⁄“ª∏ˆ≤Œ ˝Œ™œ˚œ¢¥Ú”°µ»º∂£¨ µ⁄∂˛∏ˆ≤Œ ˝Œ™œ˚œ¢¥Ú”°∑ΩœÚ 0 syslogŒƒº˛£ª1 ∆¡ƒª°£
* ‰≥ˆ≤Œ ˝£∫Œﬁ
*∑µªÿ÷µ£∫  ’˝≥£‘À–– ±£¨Œ™≤ª∂œ—≠ª∑£¨≤ª∑µªÿ°£
************************************************************/
int main(int argc, c8 *argv[])
{
	u8 count = 0;
    uint8_t retran_flag;
    uint8_t status;
    
	led_test();
	spi_init();
	lora_init();
    char buf[8] = {'a','b','c'};
	nDebugLevel = 2;
	if (argc >= 2)  nDebugLevel = (u8)atoi(argv[1]);
	nLogConsole = (argc >= 3) ? (u8)atoi(argv[2]) : 0;
	nDebugLevel = 255;

	gave_soft_dog("main");

	system_init();

	write_version(0, MAIN_VERSION);

	sleep(1);
    
    init_retran_cb();

    lora_addr_t dest_addr;
	memset(&dest_addr.addr,0x11,6);	
	while(0)
	{
		send_lora_req_data(&dest_addr);
		sleep(2);
	}
	//write_log(MSG_INFO, "set_plc_node_addr....\n");
	//set_plc_node_addr();
	Radio->StartRx();
	
	while(1)
	{
	
        //printf("loop1 \n");
		//send_sev_test();
        if(Radio->Process()==RF_RX_DONE)
		{
			printf("file %s,line %d size of lora_msg_hdr_t %d\n",__FILE__,__LINE__,sizeof(lora_msg_hdr_t));
            //printf("file %s,line %d\n",__FILE__,__LINE__);
			SX1276GetRxPacket(lora_pro_buf, &lora_pro_len);
            //printf("file %s,line %d\n",__FILE__,__LINE__);

			printf("RX-(%d)-",lora_pro_len);
			lora_rxtx_buf_print(3,lora_pro_buf,lora_pro_len);
            handle_lora_rx_msg(lora_pro_buf,lora_pro_len);
			if(lora_pro_len==0)
			{
				lora_init();
				Radio->StartRx();
			}
		}
		//get_serial_msg();
		
		//get_node_energy();  //∂¡»°Ω⁄µ„µÁ¡ø ˝æ›
		#if 1
        retran_flag = get_retran_flag();
        //”–œ˚œ¢“™÷ÿ¥´µƒ ±∫Ú‘›≤ª¥¶¿Ìpipe–≈œ¢
        if(retran_flag != TRUE)
		{
		    //printf("retran0_flag %d \n",retran_flag);
    		//¥¶¿Ìƒ⁄≤øpipeÕ®–≈
    		pipe_msg_process();
            retran_flag = get_retran_flag();
            if(retran_flag != TRUE)
		    {
                //printf("retran_flag1 %d \n",retran_flag);
		        poling_node_data();  //¬÷—Ø‘ÿ≤®Ω⁄µ„ ˝æ›
		    }
        }
        else
        {
		    printf("retran_flag2 %d \n",retran_flag);
            retran_lora_msg();
        }
		#endif
		//send_test_pipe(); //???test
		//printf("loop \n");
        //usleep(2000);
		count ++;
		if(count == 5)      {}
		if(count == 10)     {}
		if(count == 15)     {}
		if(count == 20)     gave_soft_dog("main");
		if(count > 22) count = 0;
	}
}



//ªÒµ√Ω⁄µ„ID∫≈
void get_node_id(void)
{
	FILE *fp;
	int i = 0;
/*
	u8 buf[64] = {0x11, 0x22, 0x33, 0x44, 0x00, 0x00};
	memcpy(NodeList[0].naddr, buf, 6);
	NodeList[0].naddr[5] += 1;
	memcpy(NodeList[0].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	memcpy(NodeList[1].naddr, buf, 6);
	NodeList[1].naddr[5] += 8;
	memcpy(NodeList[1].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	memcpy(NodeList[2].naddr, buf, 6);
	NodeList[2].naddr[5] += 3;
	memcpy(NodeList[2].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	memcpy(NodeList[3].naddr, buf, 6);
	NodeList[3].naddr[5] += 6;
	memcpy(NodeList[3].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	memcpy(NodeList[4].naddr, buf, 6);
	NodeList[4].naddr[5] += 5;
	memcpy(NodeList[4].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	memcpy(NodeList[5].naddr, buf, 6);
	NodeList[5].naddr[5] += 7;
	memcpy(NodeList[5].lightcfg, Default_LCfg, sizeof(Default_LCfg));

	bzero(NodeList[6].naddr, 6);
	fp = fopen(NODECFG_FILE, "wb+");
	if (fp == NULL)
	{
		usleep(10000);
		fp = fopen(NODECFG_FILE, "wb+");
		if (fp == NULL)
		{
			write_log(MSG_INFO, "creat node list error %s\n", NODECFG_FILE);
			return;
		}
	}

	fwrite((u8*)&NodeList[0], 7, sizeof(struct nodelist_st), fp);

	if(fp != NULL) fclose(fp);
*/

	fp = fopen(NODECFG_FILE, "rb+");
	if (fp == NULL)
	{
		usleep(10000);
		fp = fopen(NODECFG_FILE, "rb+");
		if (fp == NULL)
		{
			write_log(MSG_INFO, "get node list error %s\n", NODECFG_FILE);
			return;
		}
	}

	while(fread((u8*)&NodeList[i], 1,sizeof(struct nodelist_st), fp) == sizeof(struct nodelist_st))
	{
		i++;
	}
	i++;
	memset((u8*)&NodeList[i], 0x00, sizeof(struct nodelist_st));

	if(fp != NULL) fclose(fp);


}


//ø™ª˙Ω¯––µƒ“ª–©≤‚ ‘–‘π§◊˜
void test_pro(void)
{
		/*  u32 msgtime1 = 0xABAD383B;
		u32 msgtime2;

		write_log(MSG_INFO, "msgtime1=%08X", msgtime1);
		write_comm(MSG_COMMT+MSG_DEBUG, "msgtime1:", (u8 *)&msgtime1, 4);
		SetSysTime(msgtime1);

		msgtime2 = get_msg_time();
		write_log(MSG_INFO, "msgtime2=%08X", msgtime2);
		write_comm(MSG_COMMT+MSG_DEBUG, "msgtime1:", (u8 *)&msgtime2, 4);
		serverbuf((u8 *)&msgtime2, 4);
		write_log(MSG_INFO, "msgtime2s=%08X", msgtime2);
		write_comm(MSG_COMMT+MSG_DEBUG, "msgtime1:", (u8 *)&msgtime2, 4);
	*/


		/*
		int i, j, n;
		n = atoi(argv[3]);

		for(i=0; i<n; i++)
		{
			set_testplc_node();

			for(j=0; j<10; j++)
			{
				usleep(50000);
				get_serial_msg();
			}
		}
		*/
}


/*
#define SMSGHEADLEN    14
struct SMSGStr
{
	u8 head;
	u8 cmd;
	u8 devid[6];
	u8 timebuf[5];
	u8 ext;
	u8 data[240];
	u8 cs;
	u8 tail;
} __attribute__((packed));


void send_sev_test(void)
{
	static time_t tt;
	struct SMSGStr smsgst;
	static u8 noid[6] = {0x11, 0x22, 0x33, 0x44, 0x00, 0x01};
	u8 i, cs;
	u8 *pt;

	if( abs(time(NULL) - tt) < 9 ) return;

	tt = time(NULL);
	if(noid[5] == 0x01)  noid[5] = 0x02;
	else                 noid[5] = 0x01;

	smsgst.head = 0x68;
	smsgst.cmd  = 0x01;
	memcpy(smsgst.devid, noid, 6);
	Get_CurTime(smsgst.timebuf, 5);
	smsgst.ext = SID ++;

	for(i=0; i<60; i+=2)
	{
		smsgst.data[i]   = 0x02;
		smsgst.data[i+1] = i*2+1;
	}

	cs = 0;
	pt = (u8 *)&smsgst;
	for(i=0; i<SMSGHEADLEN+60; i++)
	{
		cs += *pt;
		pt ++;
	}
	smsgst.data[60] = cs;
	smsgst.data[61] = 0x16;
	//send_to_server((u8 *)&smsgst, SMSGHEADLEN+62);

	SendPipeMsg(FDW_pipe, PIPE_UPDATE, (u8 *)&smsgst, SMSGHEADLEN+62);
}
*/

void main_reboot_done(void)
{
	u8 rbuf[8];
	int i;
	c8 fname[64];
	FILE *fpr;
	FILE *fpw;
	c8 sbuf[1024];

	write_log(MSG_INFO, "main_reboot_done...\n");

	for(i=0; i<MAXNODENUM; i++)
	{
		if(is_all_xx(NodeList[i].naddr, 0, 6) == 0)  //µΩ¡À¡–±ÌµƒŒ≤
		{
			break;
		}
		memcpy(rbuf+1, NodeList[i].naddr, 6);

		if(access(LogPath, F_OK) != 0)
		{
			sprintf(fname, "mkdir %s", LogPath);
			system(fname);
		}

		if(access(fname, F_OK) != 0)
		{
			sprintf(fname, "mv %sPg%02X%02X%02X %sPg%02X%02X%02X", LogTpath, rbuf[4], rbuf[5], rbuf[6], LogPath, rbuf[4], rbuf[5], rbuf[6]);
			system(fname);
			return;
		}

		sprintf(fname, "%sPg%02X%02X%02X", LogPath, rbuf[4], rbuf[5], rbuf[6]);
		fpw = fopen(fname, "r+");
		sprintf(fname, "%sPg%02X%02X%02X", LogTpath, rbuf[4], rbuf[5], rbuf[6]);
		fpr = fopen(fname, "r");

		if( (fpr != NULL) && (fpw != NULL))
		{
			fseek(fpw, 0, SEEK_END);
			while(fgets(sbuf, 1024, fpr))
			{
				fputs(sbuf, fpw);
			}
		}
		if(fpr != NULL) {fclose(fpr); fpr = NULL;}
		if(fpw != NULL) {fclose(fpw); fpw = NULL;}

		sprintf(fname, "rm %sPg%02X%02X%02X", LogTpath, rbuf[4], rbuf[5], rbuf[6]);  //…æ≥˝ram≈ÃŒƒº˛
		system(fname);
	}
}


void send_test_pipe(void)
{
	static time_t lt = 0;
	static u16 count = 0;
	u8 pmsgbuf[128];

	if(access("/tmp/lsmsg1", F_OK) == 0)
	{
		if( abs(time(NULL)-lt) < 3 ) return;
		lt = time(NULL);

		bzero(pmsgbuf, 128);
		pmsgbuf[0] = 0x68;
		pmsgbuf[1] = 0x05;
		memcpy(pmsgbuf+2, ParaSet.Devid, 6);  //ºØ÷–∆˜µÿ÷∑
		memcpy(pmsgbuf+8, ParaSet.Devid, 6);  //ºØ÷–∆˜µÿ÷∑

		pmsgbuf[16] = count/256;
		pmsgbuf[17] = count%256;
		count ++;

		pmsgbuf[48] = calc_bcc(pmsgbuf, 48);
		pmsgbuf[49] = 0x16;
		//∑¢ÀÕ≤Â∞Œø®–≈œ¢ ˝æ›µΩ∫ÛÃ®∑˛ŒÒ∆˜
		SendPipeMsg(FDW_pipe, PIPE_UPDATE, pmsgbuf, 50);
	}
}


