#include "GPMC.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include"regsdef.h"
#include "globals.h"

#define  MEMSZ  256*1024*1024
//读取DDR 数据Mem剩余可用字节数, each unit is 128bit
int Mem_Avaliable()
{
    unsigned int high, low, count;
    high = get_reg(DDR2_Mem_USED_HIGH);
    low  = get_reg(DDR2_Mem_USED_LOW);
    count = MEMSZ - ( (low | high << 16) << 4 );
    if(debug) printf("high =%d low =%d count= %d\n", high, low, count);
    return count;
}

// 设置某个寄存器的值，可用
void set_reg(unsigned short reg_add,unsigned short value)
{
    unsigned short buf[2];
    buf[0] = reg_add*2;  //要读取的寄存器地址
    buf[1] = value;
    write(_gpmc,buf,2+2);
}

//读取某个状态寄存器的值，返回值为读取到的寄存器的值，可用
unsigned short get_reg(unsigned short reg_add)
{
    unsigned short buf[2];
    buf[0] = reg_add*2;
    read(_gpmc, buf, 2);
    return buf[0];
}


 // 写fifo函数，可用，参数３count 表示ｂｕｆ中存放的要写入的数据的字节数。可用
int  write_fifo( unsigned short fifo_add, void *buf, int count)
{
    unsigned short * buf_tmp = (unsigned short *)malloc(count + 2);
    buf_tmp[0] = fifo_add * 2;
    memcpy(buf_tmp+1, buf, count);
    int len = write(_gpmc, buf_tmp, count + 2);
    free(buf_tmp);
    return len;
}


// 读校正数据，可用，参数１表示读取的地址，参数２表示读取数据存放地址，参数３表示读取的字节数
int read_fifo(unsigned short fifo_add, unsigned short *buf, int count)
{
    buf[0]   = fifo_add * 2;
    int len  = read(_gpmc, buf,  count);
    return len;
}


