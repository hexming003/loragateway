/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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
static int GiSerialFds[4] = {-1, -1, -1, -1};		/* all serial device fd         */
const char *device = "/dev/spidev1.1";
static uint8_t mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 1000000;
static uint16_t delay;


void SpiWriteRegister (uint8_t reg, uint8_t value)
{
	//int ret;
	int fd;
	uint8_t tx[] = {0,0};

	//uint8_t rx[ARRAY_SIZE(tx)] = {0, };

	tx[0]=reg|0x80;
	tx[1]=value;
	fd = open(device, O_RDWR);
	struct spi_ioc_transfer tr_txrx[] = {
		{
                .tx_buf = (unsigned long)tx,
                .rx_buf = 0,
                .len = 2,
                .delay_usecs = 0,
                .speed_hz = speed,
                .bits_per_word = bits,
		},
	};
     ioctl(fd, SPI_IOC_MESSAGE(1), &tr_txrx[0]);
	// printf("write_reg:");
//
	 //printf("addr(%x)--%x\r\n",reg,tx[1]);


     close(fd);
//	 printf("reg_read_back:%x\r\n",SpiReadRegister (reg));

}

uint8_t SpiReadRegister (uint8_t reg)
{
	int ret;
	int fd;
	uint8_t tx[] = {0xFF,0xFF};

	uint8_t rx[ARRAY_SIZE(tx)] = {0};

	tx[0]=reg&0x7F;
	fd = open(device, O_RDWR);
	struct spi_ioc_transfer tr_txrx[] = {
		{
                .tx_buf = (unsigned long)tx,
                .rx_buf = (unsigned long)rx,
                .len = 2,
                .delay_usecs = 0,
                .speed_hz = speed,
                .bits_per_word = bits,
		},
	};
	 //printf("read_reg:");


	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr_txrx[0]);

	 //printf("addr(%x)--%x\r\n",tx[0],rx[1]);
	close(fd);
	return rx[1];
}

void SPIWriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    int fd;
    //NSS = 0;
//    NSS_SET_LOW();
//    LoRa_SPI_ExchangeByte( addr | 0x80 );
//    for( i = 0; i < size; i++ )
//    {
//        LoRa_SPI_ExchangeByte( buffer[i] );
//    }
//
//    //NSS = 1;
//    NSS_SET_HIGH();
    uint8_t tx[1024];
    uint8_t tx_readback[1024];
    memset(tx,0,1024);
	tx[0]= addr|0x80;
	memcpy(&tx[1],buffer,size);
	fd = open(device, O_RDWR);
	struct spi_ioc_transfer tr_txrx[] = {
		{
                .tx_buf = (unsigned long)&tx,
                .rx_buf = 0,
                .len = 1+size,
                .delay_usecs = 0,
                .speed_hz = speed,
                .bits_per_word = bits,
		},
	};

     ioctl(fd, SPI_IOC_MESSAGE(1), &tr_txrx[0]);
     close(fd);
     //printf("write_buffer:");

     //printf("addr(%x)--",tx[0]);

     lora_rxtx_buf_print(3,buffer,size);

     SPIReadBuffer(addr, tx_readback,  size );
     //printf("write_buffer_read_back:");

     //printf("addr(%x)--",tx[0]);
     //lora_rxtx_buf_print(3,tx_readback,size);


}

void SPIReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    int fd;
//    //NSS = 0;
//    NSS_SET_LOW();
//
//    LoRa_SPI_ExchangeByte( addr & 0x7F );
//
//    for( i = 0; i < size; i++ )
//    {
//        buffer[i] = LoRa_SPI_ExchangeByte( 0 );
//    }
//
//    //NSS = 1;
//    NSS_SET_HIGH();

    uint8_t tx[1024]={0};
    uint8_t rx[1024]={0};
    memset(tx,0,1024);
    tx[0]=addr & 0x7F;
    fd = open(device, O_RDWR);
    	struct spi_ioc_transfer tr_txrx[] = {
    		{
                    .tx_buf = (unsigned long)&tx,
                    .rx_buf = rx,
                    .len = 1+size,
                    .delay_usecs = 0,
                    .speed_hz = speed,
                    .bits_per_word = bits,
    		},
    	};

         ioctl(fd, SPI_IOC_MESSAGE(1), &tr_txrx[0]);
         memcpy(buffer,&rx[1],size);
         close(fd);
         //printf("read_buffer:");

         //printf("addr(%x)--",tx[0]);
         //lora_rxtx_buf_print(3,buffer,size);


}
/*********************************************************************************************************
** Function name:           openSerial
** Descriptions:            open serial port at raw mod
** input paramters:         iNum        serial port which can be value at: 1, 2, 3, 4
** output paramters:        NONE
** Return value:            file descriptor
** Create by:               zhuguojun
** Create Data:             2008-05-19
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int openSerial(int iPort)
{
    int iFd;

    struct termios opt; 
    char cSerialName[15];

    if (iPort >= 8) {
        printf("no such serial port:ttymxc%d . \n", iPort-1);
        exit(1);
    }
    
    if (GiSerialFds[iPort-1] > 0) {
        return GiSerialFds[iPort-1];
    }

    sprintf(cSerialName, "/dev/ttymxc%d", iPort-1);
    printf("open serial name:%s \n", cSerialName);
    iFd = open(cSerialName, O_RDWR | O_NOCTTY);                        
    if(iFd < 0) {
        perror(cSerialName);
        return -1;
    }

    tcgetattr(iFd, &opt);      

    cfsetispeed(&opt, B115200);
    cfsetospeed(&opt, B115200);
    
    /*
     * raw mode
     */
    opt.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    opt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    opt.c_oflag &= ~(OPOST);
    opt.c_cflag &= ~(CSIZE | PARENB);
    opt.c_cflag |= CS8;

    /*
     * 'DATA_LEN' bytes can be read by serial
     */
    opt.c_cc[VMIN] = DATA_LEN;                                      
    opt.c_cc[VTIME] = 0;

    if (tcsetattr(iFd, TCSANOW, &opt)<0) {
        return   -1;
    }

    GiSerialFds[iPort - 1] = iFd; 

    return iFd;
}


static void pabort(const char *s)
{
	perror(s);
	abort();
}





void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev1.0)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word \n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n");
	exit(1);
}

void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

#define REG_LR_VERSION                              0x42
#define REG_LR_LNA                                  0x0C 
#define REG_LR_MODEMSTAT                            0x18 

/*
 * 示例程序为读MX25L1635E spiflash的id功能
*/
int spi_init(void)
{
	int ret = 0;
	int fd;
	uint8_t *tx;
	uint8_t *rx;
    uint8_t value;
	int size;
	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");
	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
	
    printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
    close(fd);

	return ret;
}

