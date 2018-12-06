#include "Scan.h"
#include "BufUsb.h"
#include "GPMC.h"
#include "regsdef.h"
#include "Serial.h"
#include <unistd.h>
#include "fcntl.h"
#include "ScanPrintTypes.h"
#include "LcdUI.h"
#include "stdio.h"
#include "PrintJob.h"
#include "pthread.h"
#include "globals.h"
#include "ScanPrint.h"
//全局变量，因为LED灯控制也需要该寄存器bit13 置１
unsigned short  ScanCmdRegVal = 0x3000;

//  SCAN_CMD_REG
#define  SFIFO_CLR           0x0002
#define  SCAN_RST            0x0001
#define  SCAN_ENABLE    0x0010
#define  SCAN_START       0x0008
#define  SCAN_ARM_RW   0x0060
#define  SCAN_COLOR      0x0080

void GetScanResponse(ScanReq &sreq,  ScanRes & sres)
{
     sres.status = 0;   // wanbo
     sres.nInterl_Plane  =  1;  // plane
     sres.nRGB_BGR = 0;       // RGB 0
     sres.ScanImageWidth  =   (sreq.ScanRgnWidth  * 600 / 2.54  + 1) / 2 * 2;
     sres.ScanImageHeight =  sreq.ScanRgnHeight * 600 / 2.54;
     myRect rt(sreq.ScanRgnLeft * 600 / 2.54,   sreq.ScanRgnTop * 600 / 2.54,     (sres.ScanImageWidth+1) / 2 * 2 ,    sreq.ScanRgnHeight * 600 / 2.54);
     //printf("sreq.ScanRgnWidth=%f,sreq.ScanRgnHeight=%f\n",sreq.ScanRgnWidth,sreq.ScanRgnHeight);
     if(debug) printf("sreq.ColorMode =  %d\n",sreq.ColorMode);
     if(sreq.ColorMode == 2) { rt.x /= 8;   rt.W /= 8;  }
     UpdateScanRegion(rt);  // 确定扫描区域
     switch(sreq.ColorMode)
     {
          case 0:sres.BytesPerLine =  (sres.ScanImageWidth + 1) / 2 * 2 * 3;     break;
          case 1:
          case 2:
          default:  sres.BytesPerLine =  rt.W;           break;
     }

     if(sres.ScanImageHeight > 10000|| sres.ScanImageWidth > 7500)     sres.status = -1;
     if(debug) printf("sres.ScanImageWidth = %d  sres.ScanRgnHeight =  %d  sres.BytesPerLine  = %d, sres.status = %d\n",   sres.ScanImageWidth,   sres.ScanImageHeight,   sres.BytesPerLine, sres.status);
}

// colormode  In:   0 color,   1, gray,   2 black_white
void InitScanner(unsigned short dpi,  unsigned short  colormode, int Tint)
{
   //(1)  扫描时钟逻辑，不设置会烧掉电路板
    MoveCtrlVal |=  CIS_CLOCK_SET;            set_reg(MOVE_CTRL_REG, MoveCtrlVal);          usleep(10000);
    // (2) 初始化:   ( bit 15 : ０　bit14  ：0(1硬件半调，０ＡRM半调)  bit13：(未用)　   bit12: １（7276 perline,  0 7016 perline ）   bit 11~bit 8: 置1,  ４个扫描头应用，　 bit 6 1 Arm 写FIFO    bit5 1 Arm 读FIFO
    ScanCmdRegVal = 0x0F60;                   set_reg(SCAN_CMD_REG,  ScanCmdRegVal);
    //(3)  Reset  pulse
    ScanCmdRegVal |=  SCAN_RST;               set_reg(SCAN_CMD_REG,  ScanCmdRegVal);    // 最低位bit 0 置1  Rst 信号
    usleep(US_TIME);
    ScanCmdRegVal &=  ~SCAN_RST;              set_reg(SCAN_CMD_REG,  ScanCmdRegVal);    // 最低位置0  Rst 信号脉冲
    //(4) 异步FIFO清零
    ScanCmdRegVal |=  SFIFO_CLR;              set_reg(SCAN_CMD_REG,  ScanCmdRegVal);   // bit 1  FIFO clear 信号
    usleep(US_TIME);
    ScanCmdRegVal &=  ~SFIFO_CLR;             set_reg(SCAN_CMD_REG,  ScanCmdRegVal);  // bit1  FIFO clear 信号脉冲
   // (5) 放大倍数set
    set_reg(AFE_SET_REG, 0x6021);  usleep(20000);
    SetScan4HeadOffsets();
    setMaxGains(colormode);
    setMaxExposures(colormode);
    if(debug) printf("-----------------Tint is %x----------------\n",Tint);
    set_reg(TINT_REG,  Tint );
    // (6) ColorMode ；　１真彩色，０gray or binary  (HardWare)

    unsigned char hwColorMode =  !colormode;
    ScanCmdRegVal |= ( hwColorMode << 7 );           // bit 7　置１为真彩色,　否则灰度或２值，进一步判断
    if(colormode == 2) ScanCmdRegVal |= 0x4000;     set_reg(SCAN_CMD_REG, ScanCmdRegVal);         // ２值半色调直接扫描　bit 14  置１
    // (7) 扫描使能
    ScanCmdRegVal |=  SCAN_ENABLE;      set_reg(SCAN_CMD_REG, ScanCmdRegVal); //bit 4

    if(debug)printf("InitScanner  Set ScanCmdReg(WR 0x88H) : %2x\n", ScanCmdRegVal);
}

// 扫描流程
void* Scan(void* data)
{
         if(debug) printf("Scan Thread In  \n");
         PRN_SCAN_PARA *  psp = (PRN_SCAN_PARA*)data;
         int sfd = psp->fd;       curStage = SCANNING;       UpdateUI = 1;
         //  Read REQ
         ScanReq sreq = {0};
         int   nRead = 0,  szReq = sizeof(ScanReq);
         while( nRead < szReq )//读取扫描命令包数据，大小为szReq
         {
             nRead += read(sfd, &sreq+ nRead, szReq - nRead);
         }
         while((curSPPara.nUserID !=  psp->uid)&&(curSPPara.nMode==0))
	     {
	         sleep(3);    if(debug) printf("UserId Not Match, %d   %d\n", curSPPara.nUserID, psp->uid);
	         drawIDnotMatch();
	         if(g_exit) { if(!psp->isUsbfd) {close(psp->fd);}  return 0; }
	     }
	     while(!per.scan_per)
	     {
			drawIDnotHaveScanPermission();
			if(g_exit) { if(!psp->isUsbfd) {close(psp->fd);}  return 0; }
			sleep(1);
	     }
          // Send RES
         ScanRes sres = {0};       GetScanResponse(sreq, sres);
         int nWrite = 0,  szRes = sizeof(ScanRes);

         if(debug) printf("nWrite packet sres %d\n", nWrite);
         while(nWrite < szRes)
         {
              nWrite += write(sfd, &sres + nWrite, szRes - nWrite);
         }
         if(debug) printf("nWrite packet sres %d\n", nWrite);
         // Init Scanner
         if(0 == sres.status)
         {
             InitScanner(sreq.Resolution,  sreq.ColorMode, 128);
             DoScanAndSent(sreq.ColorMode, sfd);
             AllKindPages.scaned++;   WR_AllKindsofPages(AllKindPages);  // 更新内部扫描计数, 扫描计数不计入总数
             if(curSPPara.nMode==0)
             {
				 user.paper_nums = 1;
				 sprintf(user.handle_type,"%s","扫描");
				 get_time(user.handle_time);
				 user.user_id = psp->uid;
				 user.copys = 1;
				 sprintf(user.paper_type,"%.1lfcm*%.1lfcm",sreq.ScanRgnWidth,sreq.ScanRgnHeight);
				 insert_user_entry();
             }
         }

         if(!psp->isUsbfd) close(sfd);
         if(debug) printf("Leave Scan Thread  , %d, %d\n", curStage, UpdateUI);
         return   0;
}
