/*
 * V1.0.001:The Original Version of DLwd Module
 * V1.0.002:Modified the syslog infomation
 * V2.0.000:Just Change Version Code
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <signal.h>
#include <sys/mman.h>

#include "../public/public.h"

#define VERSION "V1.00.000"
#define CHECK_DELAY	      30
#define MAX_LOG_FILE_NUM  10 //最大文件个数
#define COPY_FREQUENCY    10 //拷贝频率 10个messages.old 装成1个message.*(1~n)

#define PIPE_REBOOT  0xcc01  //WDT通知进程系统即将重启，需要自己保存数据

void check_status(void);
void wdog_sleep(int delay);
void feed_init(void);
void boll_will_restart(void);
s32 check_usb_insert_state(void);
void usb_terminal_update(void);
void get_pwr_stat(void);
void get_reboot_time(void);
void get_devid_param(void);
int check_comm_OK(void);
void check_running_file(void);
void write_testlog(s8 *srt);

u16 Devid = 0;
u8 UsbUping = 0;
time_t Restart_T[20];
extern u8 nDebugLevel; //定义在public.c
extern u8 nLogConsole;

void check_status(void)
{
	gpio_write_high(PIN_WDOG);
	if(gpio_read_pin(MPWR_DET)) 	gpio_write_low(RUN_LED);
	usleep(200000);
	gpio_write_high(RUN_LED);
	gpio_write_low(PIN_WDOG);
}


void wdog_sleep(int delay)
{
	int i;
	static u8 sleep_second;
	
	for(i=0; i<delay; i++)
	{
		if(((sleep_second%10)==0)&&(sleep_second!=0))	
		{		
			if(check_comm_OK() == 0)  check_status();
		}
		else
		{	
			if(gpio_read_pin(MPWR_DET)) 	gpio_write_low(RUN_LED);
			usleep(200000);	
			if(UsbUping == 0) gpio_write_high(RUN_LED);
		}	

		usleep(800000);
		if (sleep_second>=10) sleep_second = 0;
		sleep_second++;		
	}
}



//获取上次开机时，最后保存log的文件号
u8 get_file_no(void)
{
	int ret;
	c8 file_name[64]={0};
	u8 file_no = 0;
	struct stat file_stat;
	time_t last_ftime = 0;
	u8 last_fno = 0;
	
	for(file_no=0; file_no<MAX_LOG_FILE_NUM; file_no++)
	{
		sprintf(file_name, "/MeterRoot/LogFiles/messages.tar.gz.%d", file_no);	
		ret = stat(file_name, &file_stat);
		if(ret == -1)
		{
			return file_no;
		}
		
		if(file_stat.st_mtime > last_ftime)
		{
			last_ftime = file_stat.st_mtime;
			last_fno = file_no;
		}
	}
	
	last_fno++;
	if(last_fno >= MAX_LOG_FILE_NUM) 
	{
		last_fno = 0;
	}
	
	return last_fno;
}


int main(int argc, c8 *argv[])
{
	time_t CurTime, LastTime;
	struct tm *ptm;
	int i;
	int file_no = 0;  //flash盘 message 文件号
	u8 cmds[256] = {0x00};
	s32 count = 0;	
	u8 usbcnum = 0;
	
	
	system("/MeterRoot/TestTool/rtctest -r &");
	
	gpio_init();
	check_status();
	
	//gpio_write_low(INET_RST);
	//sleep(1);
	//gpio_write_high(INET_RST);
	
	system("killall -9 main &");
	usleep(100000);
	system("killall -9 comm &");
	
	wdog_sleep(5);
	LastTime = 0;
	
	time(&CurTime);
	ptm = localtime(&CurTime);
	sprintf(cmds, "System Boot at %04d-%02d-%02d %02d:%02d:%02d\n",
				ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	write_testlog(cmds);
	
	get_devid_param();
	
	system("/MeterRoot/EXEFiles/comm 2 0 &");
	wdog_sleep(3);
	system("/MeterRoot/EXEFiles/main 2 0 &");
	wdog_sleep(3);
	
	nDebugLevel = 2;
	if (argc >= 2)  nDebugLevel = (u8)atoi(argv[1]);
	nLogConsole = (argc >= 3) ? (u8)atoi(argv[2]) : 0;
	
	if(argc >= 4) 
	{
		wdog_sleep(atoi(argv[3]));
	}

	for(i=0; i<20; i++)  Restart_T[i] = 0;
	file_no = get_file_no();
	//write_log(MSG_DEBUG, "日志起始文件号:%d\n", file_no);
	
	
	while (1)
	{
		for(i=0; i<CHECK_DELAY; i++)
		{
			get_pwr_stat();		//检查外部电源的接入情况，当外部电源没电时，停止工作	 
			wdog_sleep(1);
			usbcnum ++;
			if( (usbcnum % 5) == 0)
			{
				if(check_usb_insert_state() == 0)
				{
					UsbUping = 0x55;
					usb_terminal_update();
					UsbUping = 0;
				}
				get_reboot_time();
			}
		}
		
		//拷贝log文件到flash盘
		if (access("/var/log/messages.old", F_OK) == 0)
		{			
			if(access("/MeterRoot/LogFiles/messages", F_OK) != 0)
			{
			   memset(cmds,0x00,sizeof(cmds));
			   sprintf(cmds, "mv /var/log/messages.old /MeterRoot/LogFiles/messages &");
			   system(cmds);
			   wdog_sleep(1);
			   count ++;
			}
			else if(count <= COPY_FREQUENCY-1)//最大拷贝次数
			{			   
			   memset(cmds,0x00,sizeof(cmds));
			   sprintf(cmds, "cat /var/log/messages.old>>/MeterRoot/LogFiles/messages && rm /var/log/messages.old &");
			   system(cmds);
			   wdog_sleep(1);
			   count ++;
			}
			else if(count >= COPY_FREQUENCY )
			{
			   memset(cmds,0x00,sizeof(cmds));
			   sprintf(cmds, "cd /MeterRoot/LogFiles/ && tar -czvf messages.%d.tar.gz messages && rm messages &",  file_no);
			   system(cmds); 
			   wdog_sleep(1);			   
			   file_no ++;
			   if (file_no >= MAX_LOG_FILE_NUM)
			   {
				   file_no = 0; 		
			   }
			   
			   count = 0;
			}												 
		}		

		/* Add Following Code For Time Adjust Operation */
		while (1)    // 检查是否有对时
		{
			time(&CurTime);
			if (LastTime == 0)
			{
				LastTime = CurTime;
			}
			if (((CurTime - LastTime) > 2*(CHECK_DELAY)) || (CurTime < LastTime))    //发生对时
			{
				//write_log(MSG_DEBUG, "> 2*(CHECK_DELAY).. CurTime:%d Last Time:%d\n", CurTime, LastTime);
				for(i=0; i<20; i++)  Restart_T[i] = 0;
				wdog_sleep(CHECK_DELAY);
				LastTime = CurTime + CHECK_DELAY;
			}
			else
			{
				LastTime = CurTime;
				break;
			}
		}
		
		check_running_file();

	}
}

//检查应用进程是否正常活动
void check_running_file(void)
{
	static u8 Ecount[4] = {0, 0, 0, 0};
	static u8 FAILopenrun = 0;
	FILE *fp = NULL;
	u8 i, j;
	time_t LogTime, crtt;	
	s8 ModuleInfo[24];
	int fd_debug = -1;
	struct stat MyStat;
	struct tm *ptm;
	s8 cmds[64];
	s8 str[128];
	
	
	//printf("check_running_file... Ecount= %d %d  FAILopenrun=%d\n",  Ecount[0],  Ecount[1], FAILopenrun);
	
	fp = fopen("/var/log/running", "r");
	if(fp == NULL)
	{
		FAILopenrun ++;
		if(FAILopenrun > 30)
		{
			//通知进程准备重启系统	
			boll_will_restart();	
			system("reboot");  		
			sleep(1000);
			return;
		}
		return;
	}
	FAILopenrun = 0;	
	
	for (i=0; i<2; i++)
	{		
		crtt = time(NULL);
		
		LogTime = 0;
		fseek(fp, i*24, SEEK_SET);
		fscanf(fp, "%s %d ", ModuleInfo, (u32*)&LogTime);
		
		//printf("check_running_file..1.\n");
		
		if( abs(crtt - LogTime) > (3*WDINTERVAL) )   //发现模块没按时喂软件狗
		{
			//printf("check_running_file..2.\n");
			
			write_log(MSG_INFO, "%s LogTime=%ld curT=%ld %d\n", ModuleInfo, LogTime, crtt, abs(crtt - LogTime));
			
			Ecount[i] ++;
			if(Ecount[i] < 3) continue;
							
			//当经常有模块需要重启时（2小时内达12次模块重启） 重新启动系统
			for(j=0; j<19; j++)
			{
				Restart_T[j] = Restart_T[j+1];
			}
			Restart_T[19] = crtt;
			
			write_log(MSG_INFO, "restart=%ld  curT=%ld  %ld\n", Restart_T[8], crtt, (crtt - Restart_T[8]));
			if( (crtt - Restart_T[8]) < 7200 )  
			{
				//printf("check_running_file..3.\n");
				
				write_testlog("restart Module fail in 2 hour, Wdog reboot system.\n");
				
				system("chmod 777 /MeterRoot/EXEFiles/* &");
				wdog_sleep(3);
				//通知进程准备重启系统	
				boll_will_restart();	
				system("reboot");  		
				sleep(1000);
			}
			
			//printf("check_running_file..4.\n");
			//写错误信息到testlog			
			fd_debug = open("/MeterRoot/testlog", O_RDONLY);
			if (fd_debug == -1)
			{
				//write_log(MSG_SYSERR, "Open TestLog for fstat error\n");
			}
			else
			{
				fstat(fd_debug, &MyStat);
				close(fd_debug);
				if (MyStat.st_size > 50000)
				{
					system("mv /MeterRoot/testlog /MeterRoot/testlog.old &");
				}
			}
			
			//printf("check_running_file..5.\n");
			
			sprintf(str, "Module %s Abnormal at %04d-%02d-%02d %02d:%02d:%02d\n"
					    ,ModuleInfo
						,ptm->tm_year + 1900
						,ptm->tm_mon + 1
						,ptm->tm_mday
						,ptm->tm_hour
						,ptm->tm_min
						,ptm->tm_sec);
			write_testlog(str);
			
			write_log(MSG_INFO, "restart program\n");
			
			memset(cmds, 0x00, sizeof(cmds));
		    sprintf(cmds, "/bin/killall -9 comm main&");
		    system(cmds); 
			wdog_sleep(5);
			
			system("/MeterRoot/EXEFiles/comm 2 0 &");
			wdog_sleep(3);
			system("/MeterRoot/EXEFiles/main 2 0 &");
			wdog_sleep(3);
		}
		else
		{
			Ecount[i] = 0;
		}
	}
	
	//printf("check_running_file..A.\n");

	if(fp != NULL) 
	{
		fclose(fp); 
		fp = NULL;
	}
	
	//printf("check_running_file...Ok\n");
}	


//检查外部电源的接入情况，当外部电源没电时，停止工作
void get_pwr_stat(void)
{
	u8 count;
	
	count = 0;
	while( (gpio_read_pin(MPWR_DET) == 0) && (count < 3) )  //市电停电
	{
		count ++; 
		wdog_sleep(1);	
	}
	
	if( (gpio_read_pin(MPWR_DET) == 0) && (count >= 3) )  //停电超过3秒
	{
		boll_will_restart();
		write_log(MSG_SYSERR, "no power, it will shutdown\n");
		wdog_sleep(20);
		if (gpio_read_pin(MPWR_DET) == 0)
		{
			time_t tt;
			struct tm curtm;
			c8 str[128];
			
			tt = time(NULL);
			localtime_r(&tt, &curtm);			
			sprintf(str, "power down, it will shutdown %04d-%02d-%02d %02d:%02d:%02d\n",
							curtm.tm_year + 1900, curtm.tm_mon + 1, curtm.tm_mday,
							curtm.tm_hour, curtm.tm_min, curtm.tm_sec);
			write_testlog(str);
			gpio_write_low(MBAT_CTRL);
		}
	}
}


/*
 * 函数名  : get_devid_param
 * 函数描述: 读参数文件，或得节点号
 * 输入参数:
 * 输出    :
 * 返回值  :  0
 */
void get_devid_param(void)
{
	FILE *fp;
	c8 buf[128];
	u8 dbuf[8];

	fp = fopen(PARASET_FILE, "rb");
	if(fp == NULL)
	{
		write_log(MSG_INFO, "get PARA error %s\n",  PARASET_FILE);
		return;
	}
	while(fgets(buf, sizeof(buf), fp))
	{
		if(memcmp(buf, "DevID", 5) == 0)
		{
			buf[20] = 0;
			strtohex(dbuf, buf+8);
			
			Devid = dbuf[4];
			Devid = Devid*0x100 + dbuf[5];
			break;
		}
	}
	
	if(fp != NULL) fclose(fp);

	return;
}


//检查当前时间，每隔7天进行一次系统复位
void get_reboot_time(void)
{
	time_t tt;
	struct tm curtm;
	c8 str[128];

	tt = time(NULL);
	localtime_r(&tt, &curtm);
	
	//printf("Devid = %d\n", Devid);
	
	if((Devid % 7) == curtm.tm_wday)
	{
		if( (curtm.tm_hour == 2) && (curtm.tm_min == 2))
		{
			if(curtm.tm_sec < 40)
			{
				boll_will_restart();

				sprintf(str, "daily reboot, it will shutdown %04d-%02d-%02d %02d:%02d:%02d\n",
							curtm.tm_year + 1900, curtm.tm_mon + 1, curtm.tm_mday,
							curtm.tm_hour, curtm.tm_min, curtm.tm_sec);
				write_testlog(str);
				
				wdog_sleep(20);
				system("reboot");
				sleep(1000);
			}
		}
	}
}



void SendRebootPipeMsg(s32 fd)
{
	s32 i;
	struct flock lock, mylock;
	u8 buf[16] = {0x00}, CLen, checksum;

	buf[0] = 0x1B;
	buf[1] = 0x1B;
	buf[2] = 0;
	buf[3] = 0;
	buf[4] = 0x01;
	buf[5] = 0xCC;
	buf[6] = 0;

	checksum = 0;
	for (i=0; i<7; i++) checksum += buf[i];
	buf[7] = checksum;
	buf[8] = 0xB1;
	buf[9] = 0xB1;
	CLen = 10;
	
	mylock.l_type = F_WRLCK;
    mylock.l_start = 0;
    mylock.l_whence = SEEK_SET;
    mylock.l_len = 0;
    mylock.l_pid = getpid();

    i = 3;
    while(i)
    {
    	lock = mylock;
    	if( fcntl(fd, F_SETLK, &lock) == 0 )
    	{
    		if(write(fd, buf, CLen) != CLen)
			{
				write_comm(MSG_COMMR+MSG_INFO, "pipe send msg failed:", buf, CLen);
			}
            lock = mylock;
            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lock);
            
            write_comm(MSG_COMMR+MSG_DEBUG, "wdt send msg:", buf, CLen);
            break;
    	}
    	else{
    		i--;
    		sleep(1);
    	}
    }
}


void boll_will_restart(void)
{
	int fdw = 0;
	
	fdw = open(PATH_C2M_PIPE, O_WRONLY | O_NONBLOCK, 0);
	if(fdw > 0)
	{
		SendRebootPipeMsg(fdw);		
	}	
	close(fdw); 
	fdw = 0;
	
	sleep(1);
	fdw = open(PATH_M2C_PIPE, O_WRONLY | O_NONBLOCK, 0);
	if(fdw > 0)
	{
		SendRebootPipeMsg(fdw);		
	}	
	close(fdw); 
	fdw = 0;
}	
	
	
	
int check_comm_OK(void)
{
	static u8 FAILopencomm = 0;
	FILE *fp = NULL;
	time_t ctt = 0;
	time_t CommTime;	
	s8 ModuleInfo[24];
	c8 str[128];
	
	//printf("chcke_comm_Ok FAILopencomm=%d\n", FAILopencomm);
	
	fp = fopen("/var/log/comming", "r");
	if(fp == NULL)
	{
		FAILopencomm ++;
		if(FAILopencomm > 100)
		{
			//通知进程准备重启系统	
			boll_will_restart();	
			system("reboot");  		
			sleep(1000);
			return 1;
		}
		return 0;	
	}
	FAILopencomm = 0;	
	
	CommTime = 0;
	fscanf(fp, "%s %d", ModuleInfo, (u32*)&CommTime);
	fclose(fp);
	fp = NULL;
	ctt = time(NULL);	
	if( abs(ctt - CommTime) < 1000 )   //1000秒内没通信
	{
		return 0;
	}
	
	sprintf(str, "commlog is overtime ctt=%ld, Comt=%ld\n", ctt, CommTime);
	write_testlog(str);
	return 1;
}


void write_testlog(s8 *str)
{
	FILE *fp_debug;
	
	fp_debug = fopen("/MeterRoot/testlog", "a");
	if (fp_debug == NULL)
	{
		write_log(MSG_SYSERR, "open testlog error\n");
	}
	else
	{
		fprintf(fp_debug, str);
		fclose(fp_debug);
	}
}
	
