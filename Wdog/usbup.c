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
  ����˵��:     �ж�U�̲���״̬
  �������:     ��
  �������:     ��
  ����ֵ˵��:   TRUE U�̲���
                FALSE U��δ����
  ����:         ChenLe
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
  ����˵��:    ִ��U������
  �������:    ��
  �������:    ��
  ����ֵ˵��:   ��
  ����:         ChenLe
*/
void usb_terminal_update(void)
{
	s32 i;
	c8 scmd[128];
	
	//��������ļ��Ƿ����
	sprintf(scmd, "/mnt/%s", DLUSBWORK_SH);
	if(access(scmd, F_OK) != 0)
	{
		//���������
		return;
	}
	
	write_log(MSG_INFO, "dlusbw.sh file is OK...\n");
	
	sprintf(scmd, "rm -rf /tmp/updata.OK");
	system(scmd);
	wdog_sleep(1);
	
	//�ں�Ĭ�ϹҲõ�u��Ϊdos��ʽ��ֻ��ʶ��8+3���ļ��� �����������¹���
	system("umount -f /mnt");
	//��Ϊfat��ʽ
	system("mount -t vfat /dev/sda1 /mnt");
	write_log(MSG_INFO, "usb remount to vfat!\n");
	wdog_sleep(1);
	
	//���ƽű��ļ���tmpĿ¼��
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
		//���������
		return;
	}
	write_log(MSG_INFO, "doing sh...\n");
	//ִ�нű�
	sprintf(scmd, "chmod 777 /tmp/%s && sh /tmp/%s", DLUSBWORK_SH, DLUSBWORK_SH);
	system(scmd);

	//�ȴ��ű������
	i = 0;
	while( (access("/tmp/updata.OK", F_OK) != 0) && (i < 30) )
	{
		wdog_sleep(5);
		i++;
	}

	//����������ɣ�ж��u��
	system("umount -f /mnt");
	system("umount -f /mnt");
	
	write_log(MSG_INFO, "done sh OK...\n");
}



