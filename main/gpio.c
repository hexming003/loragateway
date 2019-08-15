/*
 * gpio.c
 *
 *  Created on: May 8, 2019
 *      Author: root
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#define	DEV_PATH	"/sys/class/gpio/gpio68/value"	    // 输入输出电平值设备
#define	EXPORT_PATH	"/sys/class/gpio/export"	    // GPIO设备导出设备
#define	DIRECT_PATH	"/sys/class/gpio/gpio68/direction"	// GPIO输入输出控制设备
#define	    OUT		"out"
#define	    IN		"in"
#define	    GPIO	"68"				// GPIO3_4
#define	HIGH_LEVEL	"1"
#define	LOW_LEVEL	"0"

#define GPIO3_REG_BASE   (0x20A4000)
#define MAP_SIZE        0xFF

static int get_led(void);

#if 1
int gpioInitialise(void)
{
	int ret;
	int fd_dev, fd_export, fd_dir;
	char buf[10], direction[4];
	fd_export = open(EXPORT_PATH, O_WRONLY);		    // 打开GPIO设备导出设备
	if(fd_export < 0) {
		perror("open export:");
		return -1;
	}

	write(fd_export, GPIO, strlen(GPIO));
	fd_dev = open(DEV_PATH, O_RDWR);		    // 打开输入输出电平值设备
	if(fd_dev < 0) {
		perror("open gpio:");
		return -1;
	}

	fd_dir = open(DIRECT_PATH, O_RDWR);		    // 打开GPIO输入输出控制设备
	if(fd_dir < 0) {
		perror("open direction:");
		return -1;
	}
	ret = read(fd_dir, direction, sizeof(direction));		    // 读取GPIO3_4输入输出方向
			if(ret < 0) {
				perror("read direction:");
				close(fd_export);
				close(fd_dir);
				close(fd_dev);
				return -1;
			}
			printf("default direction:%s", direction);
	strcpy(buf, IN);
	ret = write(fd_dir, buf, strlen(IN));
	if(ret < 0) {
		perror("write direction:");
		close(fd_export);
		close(fd_dir);
		close(fd_dev);
		return -1;
	}

	ret = read(fd_dir, direction, sizeof(direction));
	if(ret < 0) {
		perror("read direction:");
		close(fd_export);
		close(fd_dir);
		close(fd_dev);
		return -1;
	}
	close(fd_export);
	close(fd_dir);
	close(fd_dev);
	return 0;
}
#if 0
int dio0_read_state(void)
{
		int ret;
		int fd_dev=0;
		char buf[10]={0};
		fd_dev = open(DEV_PATH, O_RDONLY);		    // 打开输入输出电平值设备
		if(fd_dev<0)
		{
		printf("open gpio fault\r\n");
		return 0;
		}
		ret = read(fd_dev, buf, sizeof(buf));			    // 读取GPIO3_4输入电平值
		if(ret < 0) {
			perror("read gpio:");
			close(fd_dev);
			gpioInitialise();
		printf("read_fail");
		return 0;
		}

		close(fd_dev);

		if(buf[0]!=0x30)
		{
			return 1;
			//printf("input level:%s",buf);
		}
		else
		{
		return 0;
		}
}
#endif

unsigned int *map_base;

int dio0_read_state(void)
{
    int dio0_value;

    dio0_value = get_led();
    #if 0
    if(((*(volatile unsigned int *)(map_base))&(1<<4))==0)
    	return 0;
    else
    	return 1;
    #endif

    return dio0_value;
    
}




static int dev_fd;

int gpio_to_mem(void)
{
    dev_fd = open("/dev/mem", O_RDWR | O_NDELAY);

    if (dev_fd < 0)
    {
        printf("open(/dev/mem) failed.");
        return 0;
    }

    map_base=(int * )mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, GPIO3_REG_BASE );

    if(dev_fd)
        close(dev_fd);
    return 0;
}
#endif
#include <fcntl.h>      /* File control definitions */
#include <stdio.h>      /* Standard input/output */
#include <string.h>
#include <stdlib.h>
#include <termio.h>     /* POSIX terminal control definitions */
#include <sys/time.h>   /* Time structures for select() */
#include <unistd.h>     /* POSIX Symbolic Constants */
#include <assert.h>
#include <errno.h>      /* Error definitions */
#include <sys/mman.h>

#define MAP_SIZE		4096UL
#define MAP_MASK		(MAP_SIZE - 1)
#define ADDR_MASK		0xfffff000

#define GPIO_START_ADDR 	0xB8003000

//LED toggle use PG5
#define GPIOB_DIR				0xB8003180
#define GPIOB_DOUT				0xB8003184
#define GPIOB_DIN    			0xB8003188
#define GPIOB_PUEN				0xB80031A0
#define GPIOG5_PIN_NUM          (5)
#define GPIOG4_PIN_NUM          (4)

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

static int led_init(void)
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

	value = mem_read(GPIOB_DIR);
	mem_write(GPIOB_DIR,value | (0 << GPIOG5_PIN_NUM));		//set PG5 direct is input
	
	value = mem_read(GPIOB_DIR);
	mem_write(GPIOB_DIR,value | (1 << GPIOG4_PIN_NUM));		//set PG4 direct is output

    value = mem_read(GPIOB_PUEN);
	mem_write(GPIOB_PUEN, value | (1 << GPIOG4_PIN_NUM));
    
	value = mem_read(GPIOB_DOUT);
	mem_write(GPIOB_DOUT, value | (1 << GPIOG4_PIN_NUM));
#if 0
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
#endif
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
static void set_led(int is_on)
{
	unsigned int value = mem_read(GPIOB_DOUT);
	if (is_on)
		mem_write(GPIOB_DOUT, value | (1 << GPIOG5_PIN_NUM));  //set PB4 data to 1
	else
		mem_write(GPIOB_DOUT, value & (~(1 << GPIOG5_PIN_NUM)));  //set PB4 data to 0

}
static int get_led(void)
{
	int value = mem_read(GPIOB_DIN);

    return ((value & (1 << GPIOG5_PIN_NUM)) >> GPIOG5_PIN_NUM) & 0x01;  
}

int set_lora_reset_pin_high(void)
{
    
    unsigned int value;
    
    value = mem_read(GPIOB_DOUT);
	mem_write(GPIOB_DOUT, value | (1 << GPIOG4_PIN_NUM));
}
int set_lora_reset_pin_low(void)
{
    
    unsigned int value;
    
    value = mem_read(GPIOB_DOUT);
	mem_write(GPIOB_DOUT, value | (0 << GPIOG4_PIN_NUM));
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

int led_test(int argc, char *argv[])
{
	int ret;
	int i;
	ret = led_init();
	if (ret == 0)
	{
		fprintf(stderr, "int failed\n");
		return -1;
	}

	for (i = 0; i < 1; i++)
	{
	    #if 0
		set_led(1);
        printf("on\n");
		sleep(2);
		set_led(0);
        printf("off\n");
		sleep(2);
        #endif
        if(get_led())
        {
            printf("on\n");
        }
        else 
            printf("off\n");
        
		sleep(1);
	}
	
	return 0;
}

