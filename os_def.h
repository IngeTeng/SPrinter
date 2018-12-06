#ifndef OS_DEF_H_INCLUDED
#define OS_DEF_H_INCLUDED

#define DEBUG		//debug main rec thread.
#define DEBUG_PRINT_DDR
#include "regsdef.h"

#define USB_BUFFER_SIZE			4096
#define USB_BUFFER_HALF_SIZE	2048
#define USB_READ_INTERVAL_MS	1
#define USB_PROC_MIN_SIZE		16			                     //sure there is at least one cmd. comments added by mtj on 20100819
#define USB_TRANSFER_PACKAGE_SIZE		512 	    //byte
#define USB_TRANSFER_PACKAGE_SIZE_BIT16		256


#define PCL_COMMAN_MAX_SIZE  15              //每次读取数据应该将下一条命令读取到ｂｕｆ中
#define PCL_TAIL_LEN_AFTER_DATA		15  //raster end+reset+job end

#define USB_DATA_SIZE_FN	(_bufUsb.pEnd-_bufUsb.pStart)
#define USB_FREE_SIZE_FN	(_bufUsb.Buf+USB_BUFFER_SIZE-_bufUsb.pEnd)

#define WAIT_MOTION_CONROLLER_INTERVAL_SEC 30

#define ERR_MOTION_NOT_RESPONSIBLE 1001
#define ERR_PARSE_PCL_FAILED 1002
#define ERR_READ_CORRECTION_DATA 1003

typedef struct
{
	int nPageCount;                    // 本次需要打印作业的总页数 ，解析PCL获得　　　　　　　　　　
	int printed_count;                  //本次作业已经打印出来的页数，永远读寄存器获得
	unsigned short  status[2];	   // 前2个是系统严重错误，第3个是可续执行警告
}RUNNING_STATE;

typedef struct
{
	char Buf[USB_BUFFER_SIZE];
	char *pStart; 				//pointed to the offset of the data(going to read next) in the buffer
	char *pEnd;					//data in the buffer
	char *pBufEnd;				//const after implementation
}BUF_USB;

#endif
