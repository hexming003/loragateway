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


#define DLUSBWORK_SH  "dlusbw.sh"
void wdog_sleep(int delay);


/*
  功能说明:     判断U盘插入状态
  输入参数:     无
  输出参数:     无
  返回值说明:   TRUE U盘插入
                FALSE U盘未插入
  作者:         ChenLe
*/
s32 check_usb_insert_state(void)
{
	u8 buf[1024] = {0};
	FILE *fp;

	if ((fp = fopen("/proc/mounts", "r")) == NULL)
	{
		write_log(MSG_SYSERR, "can't open /proc/mounts\n");
		return -1;
	}

	while (!feof(fp))
	{
		fgets(buf, sizeof(buf), fp);
		if (strstr(buf,"/mnt") != 0)
		{
			write_log(MSG_INFO, " read u-disk input\n");
			return 0;
		}
	}
	fclose(fp);
	return -1;
}


		            
/*
  功能说明:    执行U盘升级
  输入参数:    无
  输出参数:    无
  返回值说明:   无
  作者:         ChenLe
*/
void usb_terminal_update(void)
{
	s32 i;
	c8 scmd[128];
	
	//检查升级文件是否存在
	sprintf(scmd, "/mnt/%s", DLUSBWORK_SH);
	if(access(scmd, F_OK) != 0)
	{
		//如果不存在
		return;
	}
	
	write_log(MSG_INFO, "dlusbw.sh file is OK...\n");
	
	sprintf(scmd, "rm -rf /tmp/updata.OK");
	system(scmd);
	wdog_sleep(1);
	
	//内核默认挂裁的u盘为dos格式，只能识别8+3的文件名 所以这里重新挂载
	system("umount -f /mnt");
	//挂为fat格式
	system("mount -t vfat /dev/sda1 /mnt");
	write_log(MSG_INFO, "usb remount to vfat!\n");
	wdog_sleep(1);
	
	//复制脚本文件到tmp目录下
	sprintf(scmd, "cp /mnt/%s /tmp/  &&  chmod 777 /tmp/*", DLUSBWORK_SH);
	system(scmd);
	wdog_sleep(1);
	
	i = 0;
	sprintf(scmd, "/tmp/%s", DLUSBWORK_SH);
	while( (access(scmd, F_OK) != 0) && (i < 10) )
	{
		wdog_sleep(1);
		i++;
	}	
	if(access(scmd, F_OK) != 0)
	{
		//如果不存在
		return;
	}
	write_log(MSG_INFO, "doing sh...\n");
	//执行脚本
	sprintf(scmd, "chmod 777 /tmp/%s && sh /tmp/%s", DLUSBWORK_SH, DLUSBWORK_SH);
	system(scmd);

	//等待脚本的完成
	i = 0;
	while( (access("/tmp/updata.OK", F_OK) != 0) && (i < 30) )
	{
		wdog_sleep(5);
		i++;
	}

	//升级过程完成，卸载u盘
	system("umount -f /mnt");
	system("umount -f /mnt");
	
	write_log(MSG_INFO, "done sh OK...\n");
}



