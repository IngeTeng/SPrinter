#ifndef	PXA27X_REGS_H
#define	PXA27X_REGS_H

#include "systypes.h"

// 按键 寄存器
#define PadLEDReg                              0x94              //  LED (W)
#define KeyPadReg                               0x94               // 按键寄存器         （R）
#define PRN_CMD_REG                        0x08               // 打印命令寄存器 （W）                                                      // 按位写，本地备份reg值
#define STATUS_PAPERSZ_REG            0x08               // 状态寄存器 ： 是否硬件初始化完毕， 作业是否结束， 传感器返回的纸张大小   （R）
#define HW_STATUS_REG                     0x27              // 硬件状态寄存器  （R）
#define PMODE_REG                            0x09               // 单双面，纸张来源寄存器，纸张方向，纸张大小 （W）   // 按位写，本地备份reg值
#define PAGEH_REG                             0x28               // 页高度，行数  （W）
#define LINEBYTES_REG                       0x27               // 行字节数          （W）
#define INK_STATUS_REG                     0x20               // 粉状态寄存器       （R）
#define PAGE_NUM_REG                     0x26               // 页寄存器           （W）
#define PAGES_PRINTED_REG            0x26               // 已打印的页数     ( R )
#define HW_SENSOR_REG                   0x2A               // 台面传感器        （R）

#define FPGA_DEV                        "/dev/fpga"
#define BUFFER_SIZE                     (10240+1)
#define GPMC_FIFO_WRITE            0x10                     // 打印FIFO写
#define GPMC_FIFO_READ              0x11                    // 打印FIFO读

#define GPMC_SFIFO_WRITE             0x90             // 扫描FIFO写
#define GPMC_SFIFO_READ               0x91              // 扫描FIFO读

#define DDR2_Mem_USED_LOW          0x19          //   DDR2 FIFO内128位有效单元数寄存器的低16位  (R)
#define DDR2_Mem_USED_HIGH         0x1A          //    DDR2 FIFO内128位有效单元数寄存器的高16位 (R)
#define FPGA_PrnFIFOStatus_Reg          0x1B         //	DDR2 FIFO体的状态寄存器   (R)

#define  SCAN_MOVE_STATE_REG     0x88           // 扫描台面传感器及状态寄存器， R
#define  SCAN_CMD_REG                    0x88          // 扫描命令寄存器 W
#define  MOVE_CTRL_REG                  0x89          // 运动扫描控制寄存器 W
#define  SCAN_LINE_PIXELS_REG      0xA0         // 设置图像宽度 W
#define  SCAN_HEIGHT_REG               0xA1         // 设置图像高度W
#define  SCAN_RES_REG                      0xA2         // 设置分辨率  W

#define  RD_ScanW_REG                      0xAC          // 测试用， 图像像素宽度
#define  HPC_FIFO_USED_LOW          0x99           //  HPC_FIFO存储体内128位有效数据单元 低16位读取端口  R
#define  HPC_FIFO_USED_HIGH          0x9A          //  HPC_FIFO存储体内128位有效数据单元 低16位读取端口  R

#define  AFE_SET_REG                        0xAF          // W 扫描增益寄存器
#define  MOVE_STEP_REG                  0x98           // W 扫描头移动步数
#define  STEP_BY_SETP_PON_REG    0x99            //W 单步速度入口表
#define  R_EXPOSURE_REG                0xB1            // R chanel  Exp    W
#define  G_EXPOSURE_REG                0xB2            // G chanel  Exp    W
#define  B_EXPOSURE_REG                0xB3            // B chanel  Exp    W
#define  TINT_REG                               0xB8            // Tint                  W
#endif
