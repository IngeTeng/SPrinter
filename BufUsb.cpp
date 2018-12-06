#include "BufUsb.h"
#include <unistd.h>
#include <fcntl.h>
#include  <sys/types.h>       /* basic system data types */
#include  <sys/socket.h>      /* basic socket definitions */
#include  <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>       /* inet(3) functions */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <poll.h> /* poll function */
#include <limits.h>
#include "globals.h"


#define ONE_MB (1024 * 1024)
/*
*	read data from usb until [bytes] or timeup.
*	return: bytes readed
*/
void IniUsbBuffer(BUF_USB *buf)
{
	buf->pStart = buf->Buf;
	buf->pEnd = buf->Buf;
	buf->pBufEnd = buf->Buf + USB_BUFFER_SIZE;
}

/*
*	move the data to the head of the usb
*	return: data size in buffer
*	make sure call it is necessary to ensure the efficiency
*/
int UsbBufRearrange(BUF_USB *buf)
{
	int size = USB_DATA_SIZE_FN;
	//0718 add left controll
	int left = size%4;
	memmove(buf->Buf+left, buf->pStart, size);  // dst, src, size
	buf->pStart = buf->Buf+left;
	buf->pEnd = buf->Buf+size+left;
	return 0;
}

//int ReadOnePacket2UsbBuf(BUF_USB *buf)
//{
//	char buffer[256] = {0};
//	int n = read(prn_scan_fd, buffer, 255);
//	memcpy(buf->pEnd, buffer, n);
//	buf->pEnd += n;
//    return n;
//}
//将上位机发送的数据加入PCL命令的数据进行存储
int ReadOnePacket2UsbBuf(BUF_USB *buf)
{
	char buffer[256] = {0};
	int n , m;
	//如果已经保存上一次打印的数据，则进行读取
	if(repeat_print_sign ==1)
	{
		n = read(prn_repprint_fd, buffer, 255);
	}
	//如果是第一次进行使用重复打印的功能则进行存储
	else
	{
		n = read(prn_scan_fd, buffer, 255);
		m = write(prn_repprint_fd,buffer ,n);
		//printf("n is %d , m is %d\n",n , m);
	}
	memcpy(buf->pEnd, buffer, n);
	buf->pEnd += n;

    return n;
}

int GetMemLost(){
	long num_procs;
    long page_size;
    long num_pages;
    long free_pages;
    long long  mem;
    long long  free_mem;
    long ret = 0;
    num_procs = sysconf (_SC_NPROCESSORS_CONF);
    //printf ("CPU 个数为: %ld 个\n", num_procs);
    page_size = sysconf (_SC_PAGESIZE);
    //printf ("系统页面的大小为: %ld K\n", page_size / 1024 );
    num_pages = sysconf (_SC_PHYS_PAGES);
    //printf ("系统中物理页数个数: %ld 个\n", num_pages);
    free_pages = sysconf (_SC_AVPHYS_PAGES);
    //printf ("系统中可用的页面个数为: %ld 个\n", free_pages);
    mem = (long long) ((long long)num_pages * (long long)page_size);
    mem /= ONE_MB;
    free_mem = (long long)free_pages * (long long)page_size;
    free_mem /= ONE_MB;
    ret = free_mem*100 / mem;
    //printf ("总共有 %lld MB 的物理内存, 空闲的物理内存有: %lld MB ,剩余 %d %\n", mem, free_mem, ret);
    return ret;
}


