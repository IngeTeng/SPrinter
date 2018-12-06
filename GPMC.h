#ifndef GPMC_H_INCLUDED
#define GPMC_H_INCLUDED

#define FPGA_DEV "/dev/fpga"
//校对文件路径定义
int Mem_Avaliable();

void set_reg(unsigned short reg_add, unsigned short value);

unsigned short get_reg(unsigned short reg_add);

int  write_fifo( unsigned short fifo_add, void *buf, int count);

int read_fifo(unsigned short fifo_add, unsigned short *buf, int count);

#define  US_TIME   10000

#endif // GPMC_H_INCLUDED
