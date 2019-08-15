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
#include <termios.h>
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
/*
 * 示例程序为读MX25L1635E spiflash的id功能
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
void lora_rxtx_buf_print(uint8_t rxtx,uint8_t *buf,uint16_t len)//最多发送可以512个字节
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
    return status;
}


int main(void)
{
	//gpioInitialise();
	//gpio_to_mem();
	led_test();
	spi_init();
	lora_init();
    char buf[8] = {'a','b','c'};
    uint8_t status;
    //while(1);
    while(0)
    {
        if(dio0_read_state())
        {
            printf("status %d\n",status);
        }
        
        status = lora_transmit_frame(buf, 8);
    	sleep(1);
    }
    SX1276Read( REG_LR_VERSION, &SX1276LR->RegVersion );
	Radio->StartRx();
		while(1)
		{
			//while(dio0_read_state() == 0);
            //if(get_led())

			if(Radio->Process()==RF_RX_DONE)
			{
                printf("file %s,line %d\n",__FILE__,__LINE__);
				SX1276GetRxPacket(lora_pro_buf, &lora_pro_len);
                printf("file %s,line %d\n",__FILE__,__LINE__);

				printf("RX-(%d)-",lora_pro_len);
				lora_rxtx_buf_print(3,lora_pro_buf,lora_pro_len);
				if(lora_pro_len==0)
				{
					lora_init();
					Radio->StartRx();
				}
			}
			usleep(10000);
		}
}

