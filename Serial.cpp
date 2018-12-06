/***************************************************
Filename:	serial_test.c
Description:测试串口2、3、4、5四个串口的接收和发送
Author:		liuc
Date:		2012-11-17
Modified History:
****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "Serial.h"
#include "globals.h"

#define FALSE -1
#define TRUE   0

serial_arg    seri_fd4,  seri_fd5;
/*设置串口通信速率*/
static int speed_arr[] = {B230400, B115200, B57600, B38400, \
		B19200, B9600, B4800, B2400, B1800, B1200, B600, B300};
static int name_arr[]  = {230400,  115200,  57600,  38400,  \
		19200,  9600,  4800,  2400,  1800,  1200,  600,  300};

/***************************************************
Name:       set_speed
Function:   设置波特率
Input:      无
Return:     FALSE TRUE
****************************************************/
int set_speed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;

	tcgetattr(fd, &Opt);

	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{
		if (speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);

			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0)
			{
				perror("tcsetattr fd1");
				return FALSE;
			}

			tcflush(fd,TCIOFLUSH);
			return TRUE;
		}
    }

	return FALSE;
}

/***************************************************
Name:       set_parity
Function:   设置串口数据位，停止位和效验位
Input:      1.fd          打开的串口文件句柄
      		2.databits    数据位  取值  为 7  或者 8
      		3.stopbits    停止位  取值为 1 或者2
      		4.parity      效验类型  取值为 N,E,O,S
Return:
****************************************************/
int set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if ( tcgetattr( fd,&options) != 0)
	{
		perror("SetupSerial 1");
		return(FALSE);
    }
	options.c_cflag &= ~CSIZE;

	/* 设置数据位数*/
	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			fprintf(stderr,"Unsupported data sizen");
			return (FALSE);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB; 				/* Clear parity enable */
			options.c_iflag &= ~INPCK;					/* Enable parity checking */
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;					/* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;					/* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK; 					/* Disnable parity checking */
			break;

		case 'S':
		case 's': 										/*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported parityn");
			return (FALSE);
	}

    /*  设置停止位*/
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported stop bitsn");
			return (FALSE);
	}

	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;

	tcflush(fd,TCIFLUSH);
	options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
	options.c_cc[VTIME] = 150;							// 设置超时 15 seconds
	options.c_cc[VMIN] = 0;								// Update the options and do it NOW
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}

	return (TRUE);
}

int open_dev(char *dev)
{
	int fd = open(dev, O_RDWR );

	if (-1 == fd)
	{
		if(debug) printf("Can't Open Serial Port:%s\n",dev);
		return -1;
	}
	else
		return fd;
}

void *recv_date(void *args)
{
	struct serial_arg thread_arg;
	int nread;

	thread_arg = *(struct serial_arg *)args;

	while (1)
	{
		memset(thread_arg.recv_buf, 0, sizeof(thread_arg.recv_buf));
		if ((nread = read(thread_arg.fd, thread_arg.recv_buf, 1024)) > 0)
		{
			if(debug) printf("\nSerial %d:\n",thread_arg.ser_num);
			if(debug) printf("Len: %d\n", nread);
			if(debug) printf("Pri: %s\n", thread_arg.recv_buf);
		}
	}
}

void serial4_LCD_open()
{
	//串口初始化
	seri_fd4.ser_num = 1;
	seri_fd4.fd = open_dev(SERIAL4);
	if(seri_fd4.fd < 0)
	{
		if(debug) printf("Can't Open %s !!!\n\r",SERIAL4);		return ;
	}
	set_speed(seri_fd4.fd, 9600);
	//数据位为 8  停止位为 1 校验位 NONE
	set_parity(seri_fd4.fd, 8, 1, 'N');
}

void serial5_Finger_open()
{
	//串口初始化
	seri_fd5.ser_num = 1;
	seri_fd5.fd = open_dev(SERIAL5);
	if(seri_fd5.fd < 0)
	{
		if(debug) printf("Can't Open Finger Serial %s !!!\n\r",SERIAL5);		return ;
	}
	set_speed(seri_fd5.fd, 19200);
	//数据位为 8  停止位为 1 校验位 NONE
	set_parity(seri_fd5.fd, 8, 1, 'N');
}
