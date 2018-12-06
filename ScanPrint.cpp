#include "Scan.h"
#include "BufUsb.h"
#include "GPMC.h"
#include "regsdef.h"
#include "Serial.h"
#include "ScanPrintTypes.h"
#include "Scan.h"
#include "PrintJob.h"
#include "ScanPrint.h"
#include "LcdUI.h"
#include "unistd.h"
#include "image.h"
#include  "math.h"
#include "time.h"
#include "pthread.h"
#include "fcntl.h"
#include "globals.h"

volatile int  avaliableFrames = 1;                            // 多合一复印(或身份证复印)的已扫描页数
volatile int  g_OKContinueScanPressed = 0;            // 按键OK继续扫描
volatile int  g_RunPressed = 0;                                // 按键开始强制执行
int  ScanImageNeedRotate = 0;
const int SP_RES = 600;
PageSize pagesize;
 int  Y_Mov_OFF  =  -127;            // 移动要均匀，需要启动提前量和到位空余量
 int  Y_Mov_PAD = 384;              // 在加上100多行的FIFO空白补充，每次移动到位后，curYPos +=  (YMoved + Y_Mov_PAD);
 int  Y_ZERO = 0;
 int  Y_WHITE_STRIP  =  800;
 //int  Y_ORI  =  1050;               // A B  down , goto  ori 控制扫描头初始化位置
 int  Y_ORI  =  990;               // A B  down , goto  ori 控制扫描头初始化位置
 int  Y_DEFAULT = 1800;     // A up ,          goto  default
 int  Y_A4W  =  4960 + Y_ORI;
 int  Y_A4H   =  7016 + Y_ORI;
 int  Y_A3H   =  9921 + Y_ORI;
 int  Y_SFZH =  1280 + Y_ORI;
 int  Y_SFZW=   2050 + Y_ORI;

volatile int  curYPos =  Y_ORI;    // 永远保证扫描头停靠在原点
myRect   curScanRegion(0,0, A3W, A3H);  //初始化扫描幅面Ａ3, 宽度和高度为图像空间，x,y 起点为扫描台面坐标系。

const int OneLineH = 40;

#define PS7DOWN     (0x1 << 5)
#define PS8DOWN     (0x1 << 4)
#define SCANNING    (0x1 << 9)
#define MOVING      (0x1 << 8)
#define GROUPPAGES 10    //可分页数
#define SFIFO_CLR 0x0002
unsigned short MoveCtrlVal   = 0x0280;  //  半步细分
unsigned short MoveStatusVal = 0x0000;

short  Scan4head_Gain[4] = {0x36,0x36,0x36,0x36}, Scan4head_Offset[4] = {0}, Scan3rgb_Exposure[3] = {3000,3000,3000};   // 4 头增益，４头偏移，rgb 三线阵曝光
void TestBorder(GImage *pImg);
void DoorPressDown();
void ReadScanData(GImage *pImg);
void ReadScanDataAndSent( GImage *pImg,  int  sockfd , int ncolormode);
void StepTo(int YPos);
int  DoWhiteStripCheckScan();  // 找白条位置
// 静态扫描一行
GImage *StillScanOneLine();
//判断图像是否同为横向或纵向
int  SameOrientation(GImage *src,  myRect  rt)
{
      return (src->H < src->W  && rt.H < rt.W ) || (src->H >= src->W && rt.H >= rt.W);
}

//判断图像是否同为横向或纵向
int  SameOrientation(int ph, int pw, int sh, int sw)
{
     return (ph < pw  && sh < sw ) || (ph >= pw && sh >= sw);//小边对小边，大边对大边则方向相同
}

// 以像素描述的图像坐标系下的扫描区域。
void UpdateScanRegion(myRect  &rt)
{
       curScanRegion = rt;
}
// Scan_Ctrl_Reg 12 bit 置１,  7276 ppl
void FullLinePixelsOpen()
{
       ScanCmdRegVal |= 0x1000;       set_reg(SCAN_CMD_REG,  ScanCmdRegVal);
}
// Scan_Ctrl_Reg 12 bit 置 0,  7016 ppl
void FullLinePixelsClose()
{
     ScanCmdRegVal &= ~0x1000;      set_reg(SCAN_CMD_REG,  ScanCmdRegVal);
}

//　移动到某处
void MoveTo(int YPos)
{
         short  steps = YPos - curYPos;
         set_reg(MOVE_STEP_REG,  fabs(steps));//设置移动步长

         if(YPos < curYPos)//左右移动判断
              MoveCtrlVal |=  (0x1 << 11);
         else
              MoveCtrlVal &=  0xF7FF;
         MoveCtrlVal |=  (0x01 << 4);  // Fast Move
         MoveCtrlVal |=  (0x01 << 3);  // Move_Start_CMD  Set
         set_reg(MOVE_CTRL_REG,  MoveCtrlVal);
         usleep(10000);
         MoveCtrlVal &=  0xFFF3 ;      // Move_Start_CMD Clear
         set_reg(MOVE_CTRL_REG,  MoveCtrlVal);
         usleep(10000);
         MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);  // 获得当前状态，若移动着，就等待。
         while(MoveStatusVal & MOVING)
         {
               usleep(US_TIME * 10);
               MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
         }
         curYPos += steps ;
         usleep(100000);
}

void  InitScanPos()
{
	    //usleep(500000);
        MoveTo(curYPos + 1500);
        usleep(500000);
        DoWhiteStripCheckScan();
        usleep(500000);
}
//  1   vertical,   0  horizontal (以用户面向角度为基准，　前后为竖直，　左右为横向)
// 依据扫描线图像宽和纸张高度传感器，确定幅面: A4,  A3,  SFZ.。
void   ScanImgLocation(GImage *scanImg, myRect &rt)
{
    float ave = 0, sum = 0;
    for(int p = 0; p < scanImg->W * scanImg->H; p++) sum += scanImg->bits[p];  ave = sum / (scanImg->W * scanImg->H); if(debug) printf("LineImage Ave %.2f, ScanImgLocation (w %d,h %d)\n",  ave, scanImg->W, scanImg->H);
	int left = 0, top = 0, right = 0, bottom = 0, V = ave > 255  ?  250 :  ave,  N_hit_pts = scanImg->H/3;
	int lfind = 0, tfind = 0, rfind = 0, bfind = 0, cnt = 0;

	for (int c = 0; !lfind && c < scanImg->W; c++)
	{
		cnt = 0;
		for (int r = 0; r < scanImg->H; r++)
		{
			if (scanImg->bits[r * scanImg->W + c]  > V)  { cnt++; }
			if (cnt >= N_hit_pts) { lfind = 1;  left = c;  break; }
		}
	}

	for (int c = scanImg->W - 1; !rfind && c > 0; c--)
	{
		cnt = 0;
		for (int r = 0; r < scanImg->H; r++)
		{
			if (scanImg->bits[r * scanImg->W + c] > V)  { cnt++; }
			if (cnt >= N_hit_pts) { rfind = 1;  right = c;  break; }
		}
	}

	rt.x = 0;  rt.y = 0;  rt.W = right;   rt.H = scanImg->H;
	if(debug)  printf("ScanImage Rt(x,y,w,h):( %d, %d, %d, %d)\n", rt.x, rt.y, rt.W, rt.H);
}

//单步移动到某处
void StepTo(int YPos)
{
         short  steps = YPos - curYPos;         set_reg(MOVE_STEP_REG,  fabs(steps));
         const char StpPtr =  32;                    set_reg(STEP_BY_SETP_PON_REG, StpPtr);  // 0 ~127

		 if(YPos < curYPos)     {   MoveCtrlVal |=  (0x1 << 11); }//设置左右移动
         else                               {   MoveCtrlVal &=  0xF7FF;  }
         MoveCtrlVal &=  0xFFEF;   // STEP Move
         MoveCtrlVal |=  (0x01 << 3);          set_reg(MOVE_CTRL_REG,  MoveCtrlVal);// Move_Start_CMD  Set
         usleep(1000);
         MoveCtrlVal &=  0xFFF3;                set_reg(MOVE_CTRL_REG,  MoveCtrlVal); // Move_Start_CMD Clear
         usleep(10000);
         MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);  // 获得当前状态，若移动着，就等待。
         while(MoveStatusVal & MOVING)
         {
               usleep(US_TIME * 10);             MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
         }
         curYPos += steps ;
         usleep(100000);
}

typedef struct PARA
{
      GImage *pImg;
      int sockfd;   //
      int ncolormode;
}PARA;

void * ReadScanImg(void *pData)
{
      ReadScanData((GImage*)pData);
}

void * ReadScanImgAndSent(void *pData)
{
      PARA *ptr = (PARA*)pData;
      if(debug) printf("`~~~~~~~~~~~%d~~~~~~~",ptr->ncolormode);
      ReadScanDataAndSent(ptr->pImg, ptr->sockfd , ptr->ncolormode);
}
void * ScannerMoveAction(void* pData)
{
		 if(debug) printf("-------------------ScannerMoveAction---------------\n");
         int nColorMode = * ((int*)pData);
         MoveCtrlVal |=  (0x01 << 7);           set_reg(MOVE_CTRL_REG,  MoveCtrlVal);         usleep(10000);   // (1)ＣLOCK 时序使能.
         G_LED_Val &= 0xFDFF;                   set_reg(PadLEDReg,  G_LED_Val );                                                 // (2)清除红外感光测纸器
         if(curYPos !=  Y_ORI + Y_Mov_OFF)       {   StepTo(Y_ORI + Y_Mov_OFF);   }        // (3) 扫描位回归原点
         if(debug)  printf("curScanRegionH:  (x,y,w,h) (%d, %d, %d, %d),   curYPos: %d\n", curScanRegion.x, curScanRegion.y, curScanRegion.W,  curScanRegion.H,  curYPos);
        // (6) ColorMode ；　１真彩色，０gray or binary  (HardWare)
         unsigned char hwColorMode =  !nColorMode;        ScanCmdRegVal |= ( hwColorMode << 7 );           // bit 7　置１为真彩色,　否则灰度或２值，进一步判断
         if(nColorMode == 2) ScanCmdRegVal |= 0x4000;     set_reg(SCAN_CMD_REG, ScanCmdRegVal);         // ２值半色调直接扫描　bit 14  置１
         unsigned short  imgH = curScanRegion.H;         set_reg(MOVE_STEP_REG,   imgH + Y_Mov_PAD);
         MoveCtrlVal &=  0xF7FF;         MoveCtrlVal |=  (0x01 << 4);     // 正向　快移
         MoveCtrlVal |=  (0x01 << 2);   MoveCtrlVal &=  0xFFF4;     set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD  Set
         usleep(10000);
         MoveCtrlVal &=  0xFFFB ;      set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD Clear       (4) 启动扫描

        MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);                        // （６）获得当前状态，若移动着，就等待
        while(MoveStatusVal & MOVING)
        {
               usleep(US_TIME * 10);      MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
        }
        usleep(100000);
        curYPos += imgH + Y_Mov_PAD ;                                                       //  (7)  更新扫描头位置　　　
        MoveTo(Y_ORI + Y_Mov_OFF);                                                          // (8) 扫描头回到原点
        G_LED_Val |= (0x01 << 9);      set_reg(PadLEDReg,  G_LED_Val );   //(9) Open红外感光测纸器
}
//　灰度扫描
int   DoWhiteStripCheckScan()
{
         FullLinePixelsOpen();            // 全宽扫描，找黑块
         MoveCtrlVal |=  (0x01 << 7);           set_reg(MOVE_CTRL_REG,  MoveCtrlVal);         usleep(10000);   // (1)ＣLOCK 时序使能.
         ScanCmdRegVal &= ~( 0x01 << 7 );       set_reg(SCAN_CMD_REG, ScanCmdRegVal);       // bit 7　置0灰度或２值，这里是灰度；　

         unsigned short  imgH = 12000;         set_reg(MOVE_STEP_REG,   imgH + Y_Mov_PAD);
         const char StpPtr =  64;                     set_reg(STEP_BY_SETP_PON_REG, StpPtr);  // 0 ~127
         MoveCtrlVal  |=  (0x01 << 11);     MoveCtrlVal  |=  0x01;     MoveCtrlVal &=  0xFFE1;     MoveCtrlVal |=  (0x01 << 2);     set_reg(MOVE_CTRL_REG,  MoveCtrlVal); // 反向，搜索零位bit0 = 1, 　单步移动　　Scan_Move_CMD  Set
         usleep(10000);
         MoveCtrlVal &=  0xFFFB ;          set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD Clear       (4) 启动扫描

        MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);                        // （６）获得当前状态，若移动着，就等待
        while(MoveStatusVal & MOVING)
         {
               usleep(US_TIME * 10);      MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
         }
        MoveCtrlVal &=  0xFFF0;           // 退出零位搜索, bit0  = 0。
        curYPos =  Y_WHITE_STRIP;      //  (7)  更新扫描头位置　　　
        G_LED_Val |= (0x01 << 9);      set_reg(PadLEDReg,  G_LED_Val );   //(9) Open红外感光测纸器
        FullLinePixelsClose();         // 正常扫描，关闭全宽
        return  1;
}
// 灰度模式扫描黑条，获得４头偏移量
void GetScan4HeadOffsets()
{
        usleep(500000);    // 马达休息
        StepTo(Y_ZERO);   // 黑条
        GImage* pimg  = StillScanOneLine();
        uchar *ptr = pimg->bits;
        float sum = 0, black_avg = 0;
        int c = 0;
        for(sum = 0;  c < 1616;  c++)  {   sum +=  ptr[c];   }  black_avg =  sum  / 1616;   if(debug) printf("black_avg1  %0.2f\n", black_avg);  Scan4head_Offset[0]  = black_avg > 41.6  ?  0x0FF  :  black_avg * 256 / 41.6;
        for(sum = 0;  c < 3488;  c++)  {   sum +=  ptr[c];   }  black_avg =  sum  / 1872;   if(debug) printf("black_avg2  %0.2f\n", black_avg);  Scan4head_Offset[1]  = black_avg > 41.6  ?  0x0FF  :  black_avg * 256 / 41.6;
        for(sum = 0;  c < 5360;  c++)  {   sum +=  ptr[c];   }  black_avg =  sum  / 1872;   if(debug) printf("black_avg3  %0.2f\n", black_avg);  Scan4head_Offset[2]  = black_avg > 41.6  ?  0x0FF  :  black_avg * 256 / 41.6;
        for(sum = 0;  c < 7016;  c++)  {   sum +=  ptr[c];   }  black_avg =  sum  / 1656;   if(debug) printf("black_avg4 %0.2f\n", black_avg);   Scan4head_Offset[3]  = black_avg > 41.6  ?  0x0FF  :  black_avg * 256 / 41.6;
        delete pimg;
        if(debug) printf("Offset %02x, %02x, %02x, %02x\n", Scan4head_Offset[0], Scan4head_Offset[1], Scan4head_Offset[2], Scan4head_Offset[3]);
}

void SetScan4HeadOffsets()
{
        set_reg(AFE_SET_REG, (0x4D << 8)  +  23);  usleep(100000);
        set_reg(AFE_SET_REG, (0x4F << 8)  +  23);  usleep(100000);
        set_reg(AFE_SET_REG, (0x2B << 8)  +  23);  usleep(100000);
        set_reg(AFE_SET_REG, (0x2F << 8)  +  23);  usleep(100000);
}
// 灰度模式扫白条，获得４头增益量
void GetScan4HeadGains()
{
        usleep(500000);  // 马达休息
        StepTo(Y_WHITE_STRIP  - 100);   // 白条
        GImage* pimg  = StillScanOneLine();
        unsigned char *ptr = pimg->bits;
        float sum = 0, white_gain = 0, WhiteTarget = 255;
        int c = 0;
        for(sum = 0;  c < 1616;  c++)  {   sum +=  ptr[c];   }  white_gain =  WhiteTarget / (sum  / 1616);   if(debug) printf("white_ave  %.2f,  gain %.2f\n", sum  / 1616,  white_gain); Scan4head_Gain[0]  = white_gain > 5.85  ?  76 * 4.85 / 5.85   :  76 * (white_gain - 1)  / white_gain;
        for(sum = 0;  c < 3488;  c++)  {   sum +=  ptr[c];   }  white_gain =  WhiteTarget / (sum  / 1872);   if(debug) printf("white_ave  %.2f,  gain %.2f\n", sum  / 1872,  white_gain); Scan4head_Gain[1]  = white_gain > 5.85  ?  76 * 4.85 / 5.85   :  76 * (white_gain - 1)  / white_gain;
        for(sum = 0;  c < 5360;  c++)  {   sum +=  ptr[c];   }  white_gain =  WhiteTarget / (sum  / 1872);   if(debug) printf("white_ave  %.2f,  gain %.2f\n", sum  / 1872,  white_gain); Scan4head_Gain[2]  = white_gain > 5.85  ?  76 * 4.85 / 5.85   :  76 * (white_gain - 1)  / white_gain;
        for(sum = 0;  c < 7016;  c++)  {   sum +=  ptr[c];   }  white_gain =  WhiteTarget / (sum  / 1656);   if(debug) printf("white_vae  %.2f,  gain %.2f\n", sum  / 1656,  white_gain); Scan4head_Gain[3]  = white_gain > 5.85  ?  76 * 4.85 / 5.85   :  76 * (white_gain - 1)  / white_gain;
        delete pimg;
       // printf("Gain %02x, %02x, %02x, %02x\n", Scan4head_Gain[0], Scan4head_Gain[1], Scan4head_Gain[2], Scan4head_Gain[3]);
}

void SetScan4HeadGains()
{
		set_reg(AFE_SET_REG, (0x46 << 8) + Scan4head_Gain[0]);  usleep(100000);
		set_reg(AFE_SET_REG, (0x48 << 8) + Scan4head_Gain[1]);  usleep(100000);
		set_reg(AFE_SET_REG, (0x24 << 8) + Scan4head_Gain[2]);  usleep(100000);
		set_reg(AFE_SET_REG, (0x28 << 8) + Scan4head_Gain[3]);  usleep(100000);
}
void setMaxGains(int colormode)
{
		unsigned char val = colormode ?  0x3B : 0x3F;   // 彩色 3F， 灰度 3B.
		set_reg(AFE_SET_REG, (0x46 << 8) + val);  usleep(100000);
		set_reg(AFE_SET_REG, (0x48 << 8) + val);  usleep(100000);
		set_reg(AFE_SET_REG, (0x24 << 8) + val);  usleep(100000);
		set_reg(AFE_SET_REG, (0x28 << 8) + val);  usleep(100000);
}

//uchar tints[9] = {245, 220, 200, 180, 160, 145, 133, 123, 115};
uchar tints[9] = {250 ,225, 200, 175, 150, 128, 100, 75,50};//复印浓度值
// 彩色模式扫白条，获得3色曝光值
void GetScan3rgbExposures()
{
       int MAXE = 4000,  TE = 5, TARGET = 230;
       Scan3rgb_Exposure[0] = Scan3rgb_Exposure[1] = Scan3rgb_Exposure[2] = MAXE;    // max
       ScanCmdRegVal |= ( 0x01 << 7 );        set_reg(SCAN_CMD_REG, ScanCmdRegVal);       // bit 7　置1，这里是彩色；　
       set_reg(R_EXPOSURE_REG,  Scan3rgb_Exposure[0]);  set_reg(G_EXPOSURE_REG,  Scan3rgb_Exposure[1]);  set_reg(B_EXPOSURE_REG,  Scan3rgb_Exposure[2]);   // 最大曝光值
       GImage *pImg = new GImage(3*6, 7016);
       int LastExp[3] = {Scan3rgb_Exposure[0], Scan3rgb_Exposure[1], Scan3rgb_Exposure[2]};
       int n = 20;
       while(n>0)
       //while(1)
       {
			  n--;
              LastExp[0] = Scan3rgb_Exposure[0];  LastExp[1] = Scan3rgb_Exposure[1]; LastExp[2] = Scan3rgb_Exposure[2];
              MoveCtrlVal &= 0xFFF2; MoveCtrlVal |=  (0x01 << 1); set_reg(MOVE_CTRL_REG,  MoveCtrlVal); // Scan_Still_CMD
              usleep(50000);
              MoveCtrlVal &=  0xFFFD;      set_reg(MOVE_CTRL_REG,  MoveCtrlVal);   // Scan_Still_CMD Clear
              ReadScanData(pImg);
              unsigned char *ptr = pImg->bits;
              float sum = 0, aveR = 0, aveG = 0, aveB = 0;
              int c = 0;
              for(c = 0; c < pImg->W; c++)  { sum += ptr[c]; }  aveR = sum/pImg->W;
              if(fabs(aveR - TARGET) > TE )
              {
                    if(aveR > TARGET) { Scan3rgb_Exposure[0]  -= 100; }
                    else{  Scan3rgb_Exposure[0] +=30; }
                    set_reg(R_EXPOSURE_REG, Scan3rgb_Exposure[0]);
              }

              for(c = 0, sum = 0, ptr += pImg->W; c < pImg->W; c++)  { sum += ptr[c]; }  aveG = sum / pImg->W;
              if(fabs(aveG - TARGET) > TE )
              {
                    if(aveG > TARGET) { Scan3rgb_Exposure[1]  -= 100; }
                    else{  Scan3rgb_Exposure[1] +=30; }
                    set_reg(G_EXPOSURE_REG, Scan3rgb_Exposure[1]);
              }

              for(c = 0, sum = 0, ptr += pImg->W; c < pImg->W; c++)  { sum += ptr[c]; }  aveB = sum/pImg->W;
              if(fabs(aveB - TARGET) > TE )
              {
                    if(aveB > TARGET) { Scan3rgb_Exposure[2] -= 100; }
                    else{  Scan3rgb_Exposure[2] +=30; }
                    set_reg(B_EXPOSURE_REG, Scan3rgb_Exposure[2]);
              }
              if(debug) printf("AR %.2f, AG %.2f, AB %.2f  %d,%d,%d\n", aveR, aveG, aveB, Scan3rgb_Exposure[0], Scan3rgb_Exposure[1], Scan3rgb_Exposure[2]);
              if(fabs(aveR - TARGET) < TE &&  fabs(aveG - TARGET) < TE && fabs(aveB - TARGET) < TE) break;
              if(LastExp[0] == Scan3rgb_Exposure[0] && LastExp[1] == Scan3rgb_Exposure[1]  && LastExp[2] == Scan3rgb_Exposure[2] ) break;
       }
       delete pImg;
}

 void SetScan3rgbExposures()
 {
       set_reg(R_EXPOSURE_REG,  Scan3rgb_Exposure[0]); usleep(10000);
       set_reg(G_EXPOSURE_REG,  Scan3rgb_Exposure[1]); usleep(10000);
       set_reg(B_EXPOSURE_REG,  Scan3rgb_Exposure[2]); usleep(10000);
 }
void setMaxExposures(int colormode)
{
    if(colormode)
    {
	  //set_reg(R_EXPOSURE_REG,  1900);   usleep(10000);   set_reg(G_EXPOSURE_REG,  1900);  usleep(10000);   set_reg(B_EXPOSURE_REG,  1900);
	  set_reg(R_EXPOSURE_REG,  Scan3rgb_Exposure[0]); usleep(10000); set_reg(G_EXPOSURE_REG,  Scan3rgb_Exposure[1]); usleep(10000);  set_reg(B_EXPOSURE_REG,  Scan3rgb_Exposure[2]);
	}else
	{
	   set_reg(R_EXPOSURE_REG,  Scan3rgb_Exposure[0]); usleep(10000); set_reg(G_EXPOSURE_REG,  Scan3rgb_Exposure[1]); usleep(10000);  set_reg(B_EXPOSURE_REG,  Scan3rgb_Exposure[2]);
	}
}
void ScannerCalibrationAfterPosZeroFind()
{
       memset(Scan4head_Offset, 0, sizeof(Scan4head_Offset));
       GetScan4HeadOffsets();
       SetScan4HeadOffsets();
       memset(Scan4head_Gain, 0, sizeof(Scan4head_Gain));
       GetScan4HeadGains();
       setMaxGains(0);  // color is gray
       GetScan3rgbExposures();
}

/*
//　扫描
GImage* DoAScan(int nColorMode)
{
         MoveCtrlVal |=  (0x01 << 7);         set_reg(MOVE_CTRL_REG,  MoveCtrlVal);         usleep(10000);   // (1)ＣLOCK 时序使能.
         G_LED_Val &= 0xFDFF;                   set_reg(PadLEDReg,  G_LED_Val );                                                 // (2)清除红外感光测纸器
         if(curYPos !=  Y_ORI + Y_Mov_OFF)       {   StepTo(Y_ORI + Y_Mov_OFF);   }        // (3) 扫描位回归原点
         if(debug)  printf("curScanRegionH:  (x,y,w,h) (%d, %d, %d, %d),   curYPos: %d,  nColorMode %d\n", curScanRegion.x, curScanRegion.y, curScanRegion.W,  curScanRegion.H,  curYPos, nColorMode);
        // (6) ColorMode ；　１真彩色，０gray or binary  (HardWare)
         unsigned char hwColorMode =  !nColorMode;        ScanCmdRegVal |= ( hwColorMode << 7 );           // bit 7　置１为真彩色,　否则灰度或２值，进一步判断
         if(nColorMode == 2) ScanCmdRegVal |= 0x4000;     set_reg(SCAN_CMD_REG, ScanCmdRegVal);         // ２值半色调直接扫描　bit 14  置１
         unsigned short  imgH = curScanRegion.H;         set_reg(MOVE_STEP_REG,   imgH + Y_Mov_PAD);
         MoveCtrlVal &=  0xF7FF;         MoveCtrlVal |=  (0x01 << 4);     // 正向　快移
         MoveCtrlVal |=  (0x01 << 2);   MoveCtrlVal &=  0xFFF4;     set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD  Set
         usleep(10000);
         MoveCtrlVal &=  0xFFFB ;      set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD Clear       (4) 启动扫描

        unsigned short  nScanWidth =   (nColorMode == 2 ) ?  877  :  7016;   //  default  is Gray.
        GImage *pImg = new GImage(imgH, nScanWidth);
        pthread_t  tid =  0;   pthread_create(&tid, 0,  ReadScanImg,  pImg);  //(5) 读扫描图像数据
        MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);                        // （６）获得当前状态，若移动着，就等待
        while(MoveStatusVal & MOVING)
        {
               usleep(US_TIME * 10);      MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
        }
        usleep(100000);
        curYPos += imgH + Y_Mov_PAD ;                                                       //  (7)  更新扫描头位置　　　
        MoveTo(Y_ORI + Y_Mov_OFF);                                                          // (8) 扫描头回到原点
        G_LED_Val |= (0x01 << 9);      set_reg(PadLEDReg,  G_LED_Val );   //(9) Open红外感光测纸器

       //  Get Real Region
         myRect  rt(0,  0,  curScanRegion.W,  curScanRegion.H);
         if(nColorMode == 2) { rt.x /= 8;   rt.W /= 8; }
         ImgHeader imgheader;   SubImage(pImg, rt, imgheader);
         GImage *pRimg = new GImage(imgheader.H, imgheader.W);      if(debug)  printf("pImg->W :%d, pImg->H: %d,  imgHW : %d, imgHH: %d \n", pImg->W, pImg->H, imgheader.W, imgheader.H );
         pthread_join(tid, 0);
         unsigned char *ptr = imgheader.bits, *pd = pRimg->bits;
         for(int r = 0;  r < imgheader.H;  r++ )
         {
                for(int c = 0;  c < imgheader.W;   c++)
                {
                        pd[c] = ptr[c];
                }
                ptr += imgheader.step;     pd += imgheader.W;
         }

         delete   pImg;
         return  pRimg;
} */

//　扫描
GImage* DoAScan(int nColorMode)
{
        pthread_t  tid =  0;   pthread_create(&tid, 0,  ScannerMoveAction,  &nColorMode);  //(5) 扫描图像线程,扫描指定范围curScanRegion.H的幅面数据

        unsigned short  nScanWidth =   (nColorMode == 2 ) ?  877  :  7016;   //  default  is Gray.
        GImage *pImg = new GImage(curScanRegion.H,  nScanWidth);
        ReadScanData(pImg);   // 读扫描数据.pImgH*pImgW
        //  Get Real Region
         myRect  rt(0,  0,  curScanRegion.W,  curScanRegion.H);
         if(nColorMode == 2) { rt.x /= 8;   rt.W /= 8; }
         ImgHeader imgheader;   SubImage(pImg, rt, imgheader);
         GImage *pRimg = new GImage(imgheader.H, imgheader.W);      if(debug)  printf("pImg->W :%d, pImg->H: %d,  imgHW : %d, imgHH: %d \n", pImg->W, pImg->H, imgheader.W, imgheader.H );

         unsigned char *ptr = imgheader.bits, *pd = pRimg->bits;
         for(int r = 0;  r < imgheader.H;  r++ )
         {
                for(int c = 0;  c < imgheader.W;   c++)
                {
                        pd[c] = ptr[c];
                }
                ptr += imgheader.step;     pd += imgheader.W;
         }
         delete   pImg;

         return  pRimg;
}

int scanColorMode ;
//　扫描
void DoScanAndSent(int nColorMode,  int  sockfd)
{
		if(debug) printf("++++++++++++++++++++DoScanAndSent++++++++++++++++++\n");
        scanColorMode = nColorMode;
         MoveCtrlVal |=  (0x01 << 7);       set_reg(MOVE_CTRL_REG,  MoveCtrlVal);         usleep(10000);   // (1)ＣLOCK 时序使能.
         G_LED_Val &= 0xFDFF;               set_reg(PadLEDReg,  G_LED_Val );                                                 // (2)清除红外感光测纸器
         if(curYPos !=  curScanRegion.y  + Y_ORI  +  Y_Mov_OFF)       {   StepTo(curScanRegion.y +  Y_ORI + Y_Mov_OFF);   }   // (3) 扫描设置的y起始点
         if(debug)  printf("curScanRegionH:  %d,  curYPos: %d\n", curScanRegion.H,  curYPos);

         set_reg(MOVE_STEP_REG,   curScanRegion.H + Y_Mov_PAD);
         MoveCtrlVal &=  0xF7FF;        MoveCtrlVal |=  (0x01 << 4);     // 正向　快移
         MoveCtrlVal |=  (0x01 << 2);   MoveCtrlVal &=  0xFFF4;     set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD  Set
         usleep(10000);
         MoveCtrlVal &=  0xFFFB ;      set_reg(MOVE_CTRL_REG,  MoveCtrlVal);  // Scan_Move_CMD Clear       (4) 启动扫描

       unsigned short  nScanWidth =   (nColorMode == 2 ) ?  877  :  7016;   //  default  is Gray.
       int curImageH = curScanRegion.H;
       if(nColorMode == 0) curImageH *= 3;

        GImage *pImg = new GImage(curImageH, nScanWidth);
        PARA para = { pImg,  sockfd , nColorMode};                                                    //(5) 读扫描图像数据
        pthread_t  tid =  0;   pthread_create(&tid, 0,  ReadScanImgAndSent,  &para);
        MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);                        // （６）获得当前状态，若移动着，就等待
        while(MoveStatusVal & MOVING)
        {
               usleep(US_TIME * 10);      MoveStatusVal = get_reg(SCAN_MOVE_STATE_REG);
        }
        usleep(100000);
        curYPos += curScanRegion.H + Y_Mov_PAD ;                                                       //  (7)  更新扫描头位置　　　
        MoveTo(Y_ORI +  Y_Mov_OFF);                                                                                // (8) 扫描头回到原点
        G_LED_Val |= (0x01 << 9);      set_reg(PadLEDReg,  G_LED_Val );   //(9) Open红外感光测纸器
        pthread_join(tid, 0);
       delete   pImg;
}
// 静态扫描一行
GImage *StillScanOneLine()
{
         unsigned short  steps = 0;     set_reg(MOVE_STEP_REG,  steps);
        // (6) ColorMode ；　１真彩色，０gray or binary  (HardWare)
         ScanCmdRegVal &= ~( 0x01 << 7 );     set_reg(SCAN_CMD_REG, ScanCmdRegVal);       // bit 7　置０，这里是灰度；　
         ScanCmdRegVal &= ~0x4000;            set_reg(SCAN_CMD_REG, ScanCmdRegVal);         // 灰度扫描　bit 14  置0
         MoveCtrlVal &= 0xFFF2;       MoveCtrlVal |=  (0x01 << 1);    set_reg(MOVE_CTRL_REG,  MoveCtrlVal); // Scan_Still_CMD
         usleep(50000);
         MoveCtrlVal &=  0xFFFD;      set_reg(MOVE_CTRL_REG,  MoveCtrlVal);   // Scan_Still_CMD Clear
         GImage *pImg = new GImage(OneLineH, 7016);
         ReadScanData(pImg);
         return pImg;
}

// 检测扫描台面的纸张尺寸，确定扫描幅面. R纵向
void CalScanboardSize(int W)
{
        if(curSPPara.bIsSFZ)
        {
            if(debug) printf("ShFZ W:%d\n",W);
            //if(W < 1500  )
             if(W > 4000  )//身份证横向
            { curScanRegion.H = Y_SFZW - Y_ORI;     curScanRegion.y = 0;    curScanRegion.W = Y_SFZH - Y_ORI;  curScanRegion.x = 0;  if(debug) printf("-----test--------curScanRegion.W:%d\n",curScanRegion.W);}
            else              { curScanRegion.H = Y_SFZH - Y_ORI;      curScanRegion.y = 0;    curScanRegion.W = Y_SFZW - Y_ORI;
								if(debug) printf("Y_SFZW:%d\n",Y_SFZW);
								if(debug) printf("Y_SFZH:%d\n",Y_SFZH);
								if(debug) printf("Y_ORI:%d\n",Y_ORI);
								curScanRegion.x = 0;
								if(debug) printf("-----test--------curScanRegion.H:%d\n",curScanRegion.H);
								if(debug) printf("-----test--------curScanRegion.W:%d\n",curScanRegion.W); }
        }else
        {
            if(W < 4450 )
            {
                  curScanRegion.H = B5H;      curScanRegion.y = 0;     curScanRegion.W = B5W;     curScanRegion.x = 0;     if(debug) printf("B5\n");  // B5
            }else if(W < 4800)
            {
                  curScanRegion.H = K16H;      curScanRegion.y = 0;     curScanRegion.W = K16W;   curScanRegion.x = 0;    if(debug) printf("16K\n"); //16K
            }
            else if(W < 5500)
            {
                   curScanRegion.H = A4H;      curScanRegion.y = 0;     curScanRegion.W = A4W;   curScanRegion.x = 0;      if(debug) printf("A4\n"); // A4
            }else if(W < 6130)
            {
                  curScanRegion.H = B5W;      curScanRegion.y = 0;     curScanRegion.W = B5H;   curScanRegion.x = 0;        if(debug) printf("B5R\n"); //B5R
            }else if(W < 6800)
            {
                 curScanRegion.H = K16W;      curScanRegion.y = 0;     curScanRegion.W = K16H;   curScanRegion.x = 0;      if(debug) printf("16KR\n");// 16KR
            }else if(W < 7500)
            {
                 curScanRegion.H = A4W;      curScanRegion.y = 0;     curScanRegion.W = A4H;   curScanRegion.x = 0;         if(debug) printf("A4R\n");// A4R
            }
        }
}


// 检测扫描台面的纸张尺寸，确定扫描幅面.大型纸张只有一个方向
void A3B4CalScanboardSize(int W)
{
            if(W < 6200)
            {
                curScanRegion.H = B4H;      curScanRegion.y = 0;     curScanRegion.W = B4W;   curScanRegion.x = 0;          if(debug) printf("B4\n");
            }else if(W < 6500)
            {
                curScanRegion.H = K8H;      curScanRegion.y = 0;     curScanRegion.W = K8W;   curScanRegion.x = 0;          if(debug) printf("8K\n");
            }else{
				curScanRegion.H = A3H; 		curScanRegion.y = 0; 	 curScanRegion.W = A3W;   curScanRegion.x = 0;          if(debug) printf("A3\n");
			}
}


// 长舌压下时刻
void PS7PressDown()
{
       number--;
       unsigned short ScanTableStatusVal = get_reg(SCAN_MOVE_STATE_REG);  // 扫描台面状态和运动合一寄存器
       if(number == -1)
       {
		 ScanTableStatusVal = 0x4B4;
	    }

       if(debug) printf("----%02x----\n",ScanTableStatusVal);
       drawReadyToSP();
       if( ScanTableStatusVal & (0x01 << 6) ) // 传感器被遮住只有A3,B4,8K
       {
		   set_reg(G_EXPOSURE_REG,  2000);   usleep(10000); //绿色曝光设最大
		   if(curYPos != Y_DEFAULT)   StepTo(Y_DEFAULT);   //去Default， 扫幅面
		   GImage*  curImg = StillScanOneLine();//扫描一条数据出来
		   ScanImgLocation(curImg, curScanRegion);     if(debug) printf("curScanRegion W %d H %d\n", curScanRegion.W,  curScanRegion.H);//通过分析一条数据得到了当前扫描图像的大小
		   usleep(200000);
		   if(curYPos != Y_ORI + Y_Mov_OFF)    StepTo(Y_ORI + Y_Mov_OFF);  // 回位Ori，准备扫描
		   if(curImg) delete curImg;
		   A3B4CalScanboardSize(curScanRegion.W);
		 if(curSPPara.bAutoScale) { CalScale_Page_to_Scan();  }//如果是自动缩放，则需要根据纸盒内纸张大小进行缩小
		 else
		 {
				 int pH, pW;  GetPageSize(pH, pW);//获得纸盒纸张大小
				 if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
				 {
					 ScanImageNeedRotate  = 0;  // 固定倍率100%100，图像不要旋转.
					 if(curScanRegion.W  > pW || curScanRegion.H > pH)//如果纸盒内的纸比原稿要小,警告不匹配
					 {
						  drawScanPaperNotMatch();
					 }
				 }
		  }
		 if(curYPos != Y_ORI + Y_Mov_OFF)    StepTo(Y_ORI + Y_Mov_OFF);       // 回位Ori，准备扫描
		 UpdateUI = 1;  return;
	   }

		//不是A3B4纸，则需要使用扫描头来判断纸张是什么
       set_reg(G_EXPOSURE_REG,  2000);   usleep(10000); //绿色曝光设最大
       if(curYPos != Y_DEFAULT)   StepTo(Y_DEFAULT);   //去Default， 扫幅面
       GImage*  curImg = StillScanOneLine();//扫描一条数据出来
    //   int fd = open("/nfs/OneLine.raw", O_CREAT|O_RDWR); write(fd, curImg->bits, curImg->W*curImg->H); close(fd);
       ScanImgLocation(curImg, curScanRegion);     if(debug) printf("curScanRegion W %d H %d\n", curScanRegion.W,  curScanRegion.H);//通过分析一条数据得到了当前扫描图像的大小
       usleep(200000);
       if(curYPos != Y_ORI + Y_Mov_OFF)    StepTo(Y_ORI + Y_Mov_OFF);  // 回位Ori，准备扫描
       if(curImg) delete curImg;
       CalScanboardSize(curScanRegion.W);//通过这一条数据有效长度W，通过判断确定扫描幅面curScanRegion.H，curScanRegion.y，curScanRegion.W，curScanRegion.x
       if(curSPPara.bAutoScale) {  CalScale_Page_to_Scan();  }//如果是自动缩放，则需要根据纸盒内纸张大小进行缩小
       else
       {
                int pH, pW;  GetPageSize(pH, pW);//获得纸盒纸张大小
                if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
                {
                     ScanImageNeedRotate  = 0;  // 固定倍率100%100，图像不要旋转.
					 if(curScanRegion.W  > pW || curScanRegion.H > pH)
					 {
						  drawScanPaperNotMatch();//上大下小,警告不匹配
					 }
                }
       }

       UpdateUI = 1;
}

// 长舌弹起来时刻
void PS7PressUP()
{
	  number=1;
      if(curYPos != Y_DEFAULT)  StepTo(Y_DEFAULT);
}

//该线程在扫描仪启动获得当前扫描头的位置后启动；
//定时检测当前的扫描幅面，设置参数;
//如果盖板打开，则移动头到默认位置，若Ａ按下扫一行，ＡＢ按下扫一行并移动头到ＯＲＩ。
void* ScanPageSizeCheck(void* pData)
{
     int status = 1;
     while(1)
     {
            usleep(1000);

            unsigned short SMovStatus = get_reg(SCAN_MOVE_STATE_REG);
            if(curStage == READY || curStage == PAPER || curStage == PAPERBOX || curStage == PAPERSZ || curStage == HANDPAPER)
		    {
                unsigned short PS8 = SMovStatus & PS8DOWN;
                unsigned short PS7 = SMovStatus & PS7DOWN;         // 0 is UP, 1 is DOWN.

                if( !PS7  &&  status  )           {     PS7PressUP();       status = 0;     }
                if(PS7 && (!PS8)  && status == 0) {     PS7PressDown();     status = 1;     }

            }
            // Paper Size Check
            if(curSPPara.nPaperBox == 1)
            {
				checkPaperBox1Paper();
				curSPPara.nPaperSize = nBoxPaperSz;
				sprintf(curSPPara.szPaper,"%s", szBoxPaper);
            }
            if(curSPPara.bAutoScale) { CalScale_Page_to_Scan(); }

     }
    return 0;
}

// 根据纸张大小，利用600dpi获得的打印到的物理纸张上的图像宽度和高度　（横竖以操作人员面对机器为准，前后为竖，左右为横）
void GetPageSize(int &pageH, int &pageW )
{
        // pageH  pageW 面对打印头而言，　横向纵向面对用户方向而言，前后竖直为纵，左右为横。
       switch(curSPPara.nPaperSize)    //硬件检测到的纸张代码
       {
             case 0x33:  //  Ｖertical  A4        297*210
             pageH =   A4W;  pageW = A4H;  break;
             case 0x38:  //  Horizontal A4       210*297
             case 0x18:
             pageH =   A4H;  pageW = A4W;  break;
             case 0x3F:
             case 0x37:   //  A3                     297*420
             pageH =   A3H;  pageW = A3W;  break;
             case 0x30:    // B5== 182*257, 16K== 195*270 毫米
             case 0x10:
             if(SysData.B_KPaper) {   pageH =   B5H;  pageW = B5W; break; }//设置B型纸优先
             else    {   pageH =   K16H;  pageW = K16W;   break;  }		 //设置K型纸优先
             case 0x16:
             case 0x36:   // B5||   257*182mm
              if(SysData.B_KPaper) { pageH =   B5W;  pageW = B5H;  break; }
              else{  pageH =   K16W;  pageW = K16H;   break; }
             case 0x1F:   // B4== 257*364, 8K== 270*390mm
             if(SysData.B_KPaper) {  pageH =   B4H;  pageW = B4W;  break; }
             else {  pageH =  K8H;  pageW = K8W;  break;}
             default:
             pageH =  A3H;  pageW = A3W;  break;
       }
}

void CalScale_Page_to_Scan()// 根据纸张和扫描稿件，计算缩放比例
{
       int PW = 0, PH = 0;
       GetPageSize(PH, PW);
       ScanImageNeedRotate  = 0;
       if(!curSPPara.bIsSFZ && curSPPara.nCombineMode == 0 && (!curSPPara.bIsXYScaleSet))
       {
           if(!SameOrientation(PH, PW, curScanRegion.H, curScanRegion.W))  // 方向不同，需要旋转。
           {
              ScanImageNeedRotate  = 1;
              curSPPara.nScaleX = curSPPara.nScaleY =  PW * 100  / curScanRegion.H;
           }
		   else  // 方向相同
		   {
			  curSPPara.nScaleX = curSPPara.nScaleY =  PW * 100  / curScanRegion.W;
		   }
       }
}
void SetPrintJobInfo(int pageH, int pageW)
{
      PModeRegVal &= ~(1 << 9);//纸张来源位置0,默认纸盒
      if(curSPPara.nPaperBox == 2)  PModeRegVal |= ( 1 <<  9 );   set_reg(PMODE_REG, PModeRegVal);               // （1）纸张来源 bit 9 置1表示手动纸张有效(此时nPaperBox==2), 0表示纸盒有效
      PModeRegVal |= curSPPara.nPaperSize  ;                      set_reg(PMODE_REG, PModeRegVal);               // 纸张大小设置              // （2） 纸张大小  0x33 , 0x38, 0x37 = 0x30 + 7(bit0~bit3)
      PModeRegVal  |=    (curSPPara.nSingle_Double  << 10 ) ;     set_reg(PMODE_REG, PModeRegVal);    // （3）单双面打印设置// bit 10 ：  0 单面打印， 1 长边翻转， 2 短边翻转      // 压缩与否 Arm Wr bit 13,  Arm Read  bit 12 置1,
      PModeRegVal |= 0x3000;    PModeRegVal |= ( 1 << 14 );       set_reg(PMODE_REG, PModeRegVal);      //  nRlc 0 , 未压缩数据， 14 bit 置1， 使得解压缩不进行。
       ////  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
       //PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
       usleep(US_TIME);
       PrnCmdRegVal &= 0xFFFD;   set_reg(PRN_CMD_REG,  PrnCmdRegVal);    // 1111 1101    //  bit 1 脉冲信号
	   if(debug) printf("pageH = %d,pageW = %d\n",pageH,pageW);
       set_reg(PAGEH_REG,  pageH);           //页高
       set_reg(LINEBYTES_REG, pageW );    //页宽，字节单位
}

void  ReadScanData(GImage *pImg)
{
        int totalDataSz    = pImg->W * pImg->H;  // printf("ReadScanData In\n");   if(debug)  printf("pImg->W :%d, pImg->H: %d,  totalSz %d \n", pImg->W, pImg->H, totalDataSz );
        int OnceReadSz  = 120*1024;    // DO NOT CHANGE THIS, IT IS DRIVER RELATIVE
        int nRead = 0,  nBytesLeft = totalDataSz;
        unsigned short  high = 0, low = 0;   g_exit = 0;
        while(nRead < totalDataSz)
        {
                 low = get_reg(HPC_FIFO_USED_LOW);         high = get_reg(HPC_FIFO_USED_HIGH);
                 int avaliableDataBytes =  (low  | (high  << 16 ) ) <<  4;                 //128 bit = 16 Bytes, even number!
                 if  (avaliableDataBytes  >   OnceReadSz )
                 {
                             read_fifo(GPMC_SFIFO_READ, (unsigned short*)(pImg->bits + nRead),  OnceReadSz);
                             nRead += OnceReadSz;
                  }else
                  {
                          usleep(1000);
                 }
                 if(g_exit) { if(debug) printf("ClearStop Pressed,  Scan Halt\n");      tid_sp  = 0;   curStage = READY;      UpdateUI = 1;   return ; }

                 nBytesLeft = totalDataSz - nRead;   //printf("nRead %d , Left %d, avaliableDataBytes %d  high %02x,  low %02x\n", nRead, nBytesLeft, avaliableDataBytes, high, low);
                 if(nBytesLeft < OnceReadSz &&  avaliableDataBytes >= nBytesLeft)     {     break;      }
        }
       read_fifo(GPMC_SFIFO_READ, (unsigned short*)(pImg->bits + nRead),  nBytesLeft);
   //    printf("nRead %d, ReadScanData Out \n", nRead + nBytesLeft);
}

void  ReadScanDataAndSent( GImage *pImg,  int  sockfd ,int ncolormode)
{
        int totalDataSz    = pImg->W * pImg->H;
        pagesize.hight = pImg->H;
        pagesize.weight = pImg->W;
        pagesize.totalsize = totalDataSz;
        pagesize.cate = 1;
        pagesize.ncolormode = ncolormode;
        if(ncolormode >0)
        {
			 UpdatePageSize(pagesize);
        }

        if(debug) printf("totalDataSz %d,   W %d, H %d,  scanRegionW %d, scanRegionH %d \n", totalDataSz, pImg->W, pImg->H,  curScanRegion.W, curScanRegion.H);
        int OnceReadSz  = 120*1024;    // DO NOT CHANGE THIS, IT IS DRIVER RELATIVE
        int nRead = 0,  nBytesLeft = totalDataSz,  nNewR = 0;
        unsigned short  high = 0, low = 0,  H = pImg->H;
        unsigned char * ptr = pImg->bits;
        int total = 0;
        int test_num = 0;
        if(ncolormode >0)
        {
			prn_repscan_fd = open("/media/repeat_scan.raw", O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);//打开即将写入的目标文件
			lseek(prn_repscan_fd,0L,SEEK_SET);
			if(prn_repscan_fd < 0)
			{
				if(debug) printf("Open  file error\n");
			}
        }

        while(nRead < totalDataSz)
        {
                 low = get_reg(HPC_FIFO_USED_LOW);         high = get_reg(HPC_FIFO_USED_HIGH);
                 int avaliableDataBytes =  (low  | (high  << 16 ) ) <<  4;                 //128 bit = 16 Bytes, even number!
                 if  (avaliableDataBytes  >   OnceReadSz )
                 {
                             read_fifo(GPMC_SFIFO_READ, (unsigned short*)(pImg->bits + nRead),  OnceReadSz);
                             nRead += OnceReadSz;      nNewR += OnceReadSz;
                             if(nNewR > 1200 * pImg->W)  // 传送400行数据
                             {
                                       for(int r = 0;  r <1200;  r++)
                                       {
                                              write(sockfd,  ptr + curScanRegion.x,   curScanRegion.W);     ptr += pImg->W;
                                       }
                                       H -= 1200;      nNewR  = 0;
                             }
                  }else
                  {
                          usleep(10000);
                 }
                 if(g_exit) { if(debug) printf("ClearStop Pressed,  Scan Halt\n");    tid_sp  = 0;   curStage = READY;      UpdateUI = 1;   return ; }

                 nBytesLeft = totalDataSz - nRead;
                 if(nBytesLeft < OnceReadSz &&  avaliableDataBytes >= nBytesLeft)     {     break;      }
          //   if(debug) printf("nRead %d,  nBytesLeft %d,  avaliableBytes %d , high %2x, low %2x \n",   nRead, nBytesLeft, avaliableDataBytes, high, low);
        }
       read_fifo(GPMC_SFIFO_READ, (unsigned short*)(pImg->bits + nRead),  nBytesLeft);  if(debug) printf("nBytesLeft %d\n",  nBytesLeft);
	   if(ncolormode >0)
	   {
			test_num = write(prn_repscan_fd, (unsigned short*)(pImg->bits),  totalDataSz);  //开始向板子中写入数据
               //total += test_num;
			if(debug) printf("^^^^^^^^^^^^^^^^^^^^^^%d , %d^^^^^^^^^^^^^^^\n" , test_num , totalDataSz);
			close(prn_repscan_fd);
	   }


       for(int r = 0;   r < H;  r++)
       {
              write(sockfd,  ptr + curScanRegion.x,   curScanRegion.W);        ptr += pImg->W;
       }
}

void  PrintOnePage(GImage *theHalftonePage, int  &pages,  int totalPages, int &PrnStartCmd)
{
      if(debug) printf("PrintOnePage  In \n");
      if( !ExceptionHandler() )  {  if(debug) printf("Exception Not Handle,  Not Print One Page\n ");   return ; }

      while(Mem_Avaliable() < theHalftonePage->W * theHalftonePage->H)  // 若FIFO空间不足一页，则等待FIFO.
       {
              usleep(US_TIME);    if(debug) printf("Waiting  FIFO\n ");   if(g_exit) {    if(debug) printf("ClearStop Pressed,  Clear Print\n ");    return ;    }
        }
       if(g_exit) {    if(debug) printf("ClearStop Pressed,  Clear Print\n ");   return ;  }
//	   TestBorder(theHalftonePage);
       write_fifo (GPMC_FIFO_WRITE,  theHalftonePage->bits,  theHalftonePage->W * theHalftonePage->H );  // theHalftonePage->W  is packed  as  Bytes　
       pages ++;
      {
          AllKindPages.duplicated++;    AllKindPages.total++;  // WR_AllKindsofPages(AllKindPages);
          if(curSPPara.nMode==0)
          {
			  user.paper_nums = pages;
			  if(pages == 1) insert_user_entry();
			 // else update_user_entry(&mysql, pages);
          }
      }  // 更新内部复印计数

        if(pages % 4  == 3 ||  pages == totalPages)//写入fifo数据满3页或者达到总页数发送打印开始命令
		 {
		       set_reg(PAGE_NUM_REG, pages);
		       if(!PrnStartCmd)
		        {
		             PrnStartCmd = 1;
		             PrnCmdRegVal |=  PRINT_START;       //  bit  4 置 1
		             set_reg(PRN_CMD_REG,  PrnCmdRegVal);   // 下达打印开始命令
		        }
		  }
		  _state.printed_count = get_reg(PAGES_PRINTED_REG);  // 已打印份数
	     if(debug)  printf("Printed Pages %d pages in fifo = %d\n", _state.printed_count, pages);
         if(debug) printf("PrintOnePage  Out \n");
}


// 装订线余白
void pageMargin(myRect &prt)
{
	if(debug) printf("pageMargin In, colormode %d\n", curSPPara.nColormode);
	int pixels = curSPPara.nMaginmm * 600 / 25.4;
	if(curSPPara.nColormode == 2)  // 扫描半调图.
	{
	    pixels /= 8;
	}
	switch (curSPPara.nPageMargin)
	{
	case 1:  // left margin
		prt.x += pixels; prt.W -= pixels;
		break;
	case 2:	 // top margin
		prt.y += pixels; prt.H -= pixels;
		break;
	case 3:  // right margin
		prt.W -= pixels;
		break;
	default:
		break;
	}
	if(debug) printf("pageMargin Out\n");
}

void CutBorder(GImage *pImg)
{
	int rpixels = curSPPara.nErasemm * 600 / 25.4,  cpixels = rpixels;//根据毫米长度计算出需要填充的像素点宽度
	char val = 255;					//削边填充白色
	if(curSPPara.nColormode == 2)  // 扫描半调图.
	{
	    cpixels /= 8; val = 0;
	}
	if(debug) printf("In Cutborder %d, colorMode %d\n", cpixels, curSPPara.nColormode);
	uchar *ptr = pImg->bits;
	switch (curSPPara.nEraseBorder)
	{
	case 1:  // left cut
		for (int r = 0; r < pImg->H; r++)
		{
			for (int c = 0; c < cpixels; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		break;
	case 2:	 // top cut
		for (int r = 0; r < rpixels; r++)
		{
			for (int c = 0; c < pImg->W; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		break;
	case 3:  // surround cut
		for (int r = 0; r < rpixels; r++)
		{
			for (int c = 0; c < pImg->W; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		ptr = pImg->bits + (pImg->H - rpixels)* pImg->W;
		for (int r = 0; r < rpixels; r++)
		{
			for (int c = 0; c < pImg->W; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		ptr = pImg->bits;
		for (int r = 0; r < pImg->H; r++)
		{
			for (int c = 0; c < cpixels; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		ptr = pImg->bits;
		for (int r = 0; r < pImg->H; r++)
		{
			for (int c = pImg->W - cpixels; c < pImg->W; c++)
			{
				ptr[c] = val;
			}
			ptr += pImg->W;
		}
		break;
	default:
		break;
	}
	if(debug) printf("Cutborder Out\n");
}

void TestBorder(GImage *pImg)
{
	//nt rpixels = curSPPara.nErasemm * 600 / 25.4,  cpixels = rpixels;//根据毫米长度计算出需要填充的像素点宽度
	//char val = 255;					//削边填充白色
	//if(curSPPara.nColormode == 2)  // 扫描半调图.
	//{
	//    cpixels /= 8; val = 0;
	//}
	//printf("In Cutborder %d, colorMode %d\n", cpixels, curSPPara.nColormode);
	uchar *ptr = pImg->bits;
	if(debug) printf("TEST Out ,pImg->W =%d,pImg->H=%d\n",pImg->W,pImg->H);
		for (int r = 0; r < pImg->H; r++)
		{
			for (int c = 0; c < pImg->W; c++)
			{
			    if(r==0&&c<10) ptr[c] = 0xaa;
			    else if((r==0)&&(c>=(pImg->W-10))) ptr[c] = 0xbb;
			    else if(r==1&&c<10) ptr[c] = 0xcc;
				//if(r==0&&c==0) ptr[c] = 0xaa;
				//else if(c==0&&r!=0) ptr[c] = 0xbb;
				//else if(c==pImg->W-1) ptr[c] = 0x55;
				else if((r==pImg->H-1)&&(c>=(pImg->W-10))) ptr[c] = 0xdd;
			}
			ptr += pImg->W;
		}

}



// 单帧图像curImg ， 页面布局不旋转。
void DirectPageLayout(GImage *ThePage, GImage *curImg)
{
	if (debug) printf("DirectPageLayout In\n");    float  s = clock();

	myRect  scan_rt(0, 0, curImg->W, curImg->H);  	CutBorder(curImg);
	myRect  prt(0, 0, ThePage->W, ThePage->H);      pageMargin(prt);                  // 纸张上的渲染区域

	double drx = curSPPara.nScaleX / 100., dry = curSPPara.nScaleY / 100.;
	int DH = scan_rt.H * dry, DW = scan_rt.W * drx;
	int max_WH = Max(DH, DW);
	int max_PageWH = Max(prt.H, prt.W);

	ImgHeader  img_header = { 0 };
	if (max_WH >= max_PageWH)                                                    // print only Page size  area  out
	{
		int MinW = Min(prt.W, DW), MinH = Min(DH, prt.H);	prt.W = MinW;  prt.H = MinH;
		int sx = scan_rt.x, sy = scan_rt.y, sw = MinW / drx, sh = MinH / dry;
		myRect srt(sx, sy, sw, sh);
		SubImage(curImg, srt, img_header);  if (debug) printf(" maxScanWH >= PageWH, same orientation, before  DirectPaste\n"); // 获得该区域图像描述　img_header.
		Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
	}
	else                                                                                           // <=  print all the scan A3  and resized image.
	{
		myRect srt = scan_rt;
		SubImage(curImg, srt, img_header);      // 获得该区域图像描述　img_header.
		prt.W = Min(prt.W, DW);  prt.H = Min(prt.H, DH);
		if (debug) printf(" max_WH < min_PageWH, Not same orientation, before  DirectPaste\n");
		Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
	}

	if (debug) printf("DirectPageLayout Leave,  time: %.2f \n", (clock() - s) / CLOCKS_PER_SEC);
}


// 单帧2值图像不支持缩放和旋转的页面布局。
void PageLayoutBinary(GImage *ThePage, GImage *curImg)
{
	if (debug) printf("PageLayoutBinary In\n");    float  s = clock();

   	myRect  scan_rt(0, 0, curImg->W, curImg->H);  	CutBorder(curImg);
	myRect  prt(0, 0, ThePage->W, ThePage->H);      pageMargin(prt);                  // 纸张上的渲染区域

    ImgHeader  img_header = { 0 };
	SubImage(curImg, scan_rt, img_header);                 // 获得该区域图像描述　img_header.
    P(ThePage, prt, img_header);

	if (debug) printf("PageLayoutBinary Leave,  time: %.2f \n", (clock() - s) / CLOCKS_PER_SEC);
}


// 单帧图像curImg is always A3 size， 页面布局。
void PageLayoutGray(GImage *ThePage, GImage *curImg)
{
	if (debug) printf("PageLayout In\n");    float  s = clock();
    if(curSPPara.nPaperBox && !curSPPara.bAutoScale || curSPPara.nPageMargin || curSPPara.nEraseBorder)
    {
       DirectPageLayout(ThePage, curImg);
       return ;
    }  // 非自动纸张，　非自动倍率，　或者裁边 或者页边距的， 都直接复印。

	myRect  scan_rt(0, 0, curImg->W, curImg->H);  	CutBorder(curImg);
	myRect  prt(0, 0, ThePage->W, ThePage->H);      pageMargin(prt);                  // 纸张上的渲染区域

	double drx = curSPPara.nScaleX / 100., dry = curSPPara.nScaleY / 100.;
	int DH = scan_rt.H * dry, DW = scan_rt.W * drx;
	int max_WH = Max(DH, DW);   int max_PageWH = Max(prt.H, prt.W);	  int min_PageWH = Min(prt.H, prt.W);
	int same_orientation = SameOrientation(ThePage, myRect(scan_rt.x, scan_rt.y, DW, DH));   // 稿件方向和纸张方向是否一致（按照物理扫描和打印进纸动作，水平为行宽，竖直为页高）

	ImgHeader  img_header = { 0 };
	if (max_WH > max_PageWH)                                                    // print only Page size  area  out
	{
		if (same_orientation)
		{
			int MinW = Min(prt.W, DW), MinH = Min(DH, prt.H);	prt.W = MinW;  prt.H = MinH;
			int sx = scan_rt.x, sy = scan_rt.y, sw = MinW / drx, sh = MinH / dry;
			myRect srt(sx, sy, sw, sh);
			SubImage(curImg, srt, img_header);  if (debug) printf(" maxScanWH > PageWH, same orientation, before  Paste\n"); // 获得该区域图像描述　img_header.
			Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
		}
		else  // Rotate Need
		{
			int MinW = Min(prt.W, DH), MinH = Min(DW, prt.H); prt.x += prt.W - MinW;  prt.W = MinW;   prt.H = MinH;
			int sx = scan_rt.x, sy = scan_rt.y, sw = MinW / dry, sh = MinH / drx;
			myRect srt(sx, sy, sw, sh);
			SubImage(curImg, srt, img_header);  if (debug) printf("maxScanWH > PageWH, Not  same orientation , before  R90Paste\n");// 获得该区域图像描述　img_header.
			R90Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
		}
	}
	else                                                                                           // <=  print all the scan A3  and resized image.
	{
		if (same_orientation)
		{
			myRect srt = scan_rt;
			SubImage(curImg, srt, img_header);                 // 获得该区域图像描述　img_header.
			if (max_WH < min_PageWH && DH != DW)               //  rotate 能旋转，则省空间。
			{
				int MinW = Min(prt.W, DH), MinH = Min(DW, prt.H); prt.x += prt.W - MinW;  prt.W = MinW;    prt.H = MinH;
				if (debug) printf(" max_WH <= min_PageWH, same orientation, before  R90Paste\n");
				R90Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
			}
			else
			{
				prt.W = Min(DW, prt.W);  prt.H = Min(DH, prt.H);
				if (debug) printf(" max_WH >= min_PageWH, same orientation, before  Paste\n");
				Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
			}
		}
		else
		{
			myRect srt = scan_rt;
			SubImage(curImg, srt, img_header);      // 获得该区域图像描述　img_header.
			if (max_WH <= min_PageWH)                 // No旋转，省空间。
			{
				prt.W = Min(prt.W, DW);  prt.H = Min(prt.H, DH);
				if (debug) printf(" max_WH <= min_PageWH, Not same orientation, before  Paste\n");
				Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);

			}
			else
			{
				int MinW = Min(prt.W, DH), MinH = Min(DW, prt.H); prt.x += prt.W - MinW;  prt.W = MinW;    prt.H = MinH;
				if (debug) printf(" max_WH >= min_PageWH, Not same orientation, before  R90Paste\n");
				R90Paste(ThePage, prt, img_header, curSPPara.nScaleX == curSPPara.nScaleY);
			}
		}
	}

	if (debug) printf("PageLayou Leave,  time: %.2f \n", (clock() - s) / CLOCKS_PER_SEC);
}

void OnePageLayout(GImage *TheOne, GImage *ASub, int nColor)
{
    if(nColor == 1)
    {
       PageLayoutGray(TheOne, ASub);
    }
    else if(nColor == 2)
    {
       PageLayoutBinary(TheOne, ASub);
    }
}

// 根据当前应用场景和缩放系数，安排页面布局。
void PageLayout(GImage *TheOne, GImage *ASub, int ithPatchNow)
{
	myRect scan_rt(0, 0, ASub->W,  ASub->H);
	int same_orientation = SameOrientation(TheOne, scan_rt);   if(debug) printf("PageLayout In, ithPatchNow %d ,  AsubW  %d,  ASubH %d, \n", ithPatchNow,  ASub->W, ASub->H);
	ImgHeader imgheader;
	SubImage(ASub, scan_rt, imgheader);
	// rotate and paste and resize.
	if (curSPPara.bIsSFZ)
	{
		SFZ_SP(TheOne, imgheader, ithPatchNow);
	}
	else if (curSPPara.nCombineMode == 1) // 2 合 1
	{
		Two_to_One(TheOne, imgheader, same_orientation, ithPatchNow);
	}
	else  // 4 合 1
	{
		Four_to_One(TheOne, imgheader, same_orientation, ithPatchNow);
	}
}
char* SPaperName(int hwPaperCode)
{
   switch (hwPaperCode)
    {
         case 0x33:  return "A4||";
         case 0x38:
         case 0x18:  return "A4==";
         case 0x3F:
         case 0x37:  return "A3";
         case 0x10:  return "B5_16K==";
         case 0x1F:  return "B4_8K==";
         case 0x16:  return "B5_16K||";
         default:    return "UNKNOW";
    }
}

void NegtiveImg(GImage * curImg)
{
    unsigned char  byte  = curSPPara.nNegtive ?  0x00 :  0xff;
    unsigned char * p = curImg->bits ;
    for(int  i = 0;  i <  curImg->H;  i++)
    {
         for(int  c = 0;  c < curImg->W;  c++)
         {
            p[c]    ^=    byte;
         }
         p += curImg->W;
    }
}
// 复印流程,  按开始按钮启动该线程.
void* ScanPrint(void*)
{
       if(debug) printf("ScanPrint Thread In\n");

	   DoUIOperationCheck();//做互斥检查
       int pageH, pageW;   GetPageSize(pageH, pageW);//由传感器值判断纸盒（纸盒或手动）中纸张，将数据长宽赋给pageH,pageW
       curSPPara.nColormode = 1;      // 1 gray, 2 binary
       if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100 && !ScanImageNeedRotate && !curSPPara.bIsSFZ)//x100%y100%，图像不需要旋转，也不是身份证复印，则直接扫描出二值图即可
       {
          curSPPara.nColormode = 2;
          pageW /= 8;   // 半调扫描，宽度除8.灰度一个点8bit数据，半调只有1bit非黑即白
       }
       if(debug) printf("PageH %d, PageW %d, curScanRegion.W %d, curScanRegion.H %d, nColor %d\n", pageH, pageW, curScanRegion.W, curScanRegion.H, curSPPara.nColormode);                                                          // 根据纸张检测结果，获得当前打印页面图像的宽度和高度 600 dpi
       if(!curSPPara.bAutoScale && curSPPara.nColormode == 2)//这里我注释掉了之前判断纸张不匹配则终止，现在当为固定倍率时，直接以纸盒尺寸为准扫描打印,这里不用担心固定或者手动倍率不为100%时的情况，因为针对半调图一定是100%倍率的
       {
//         if(curScanRegion.W / 8  > pageW || curScanRegion.H  > pageH)//除以8是因为上面将pageW已经除8
//          {
//              drawScanPaperNotMatch();  tid_sp = 0;    curStage = READY;     UpdateUI = 1;   return 0;//这里不匹配直接终止复印
//          }
          curScanRegion.y = 0;    curScanRegion.x = 0;
          curScanRegion.W = pageW *8; curScanRegion.H = pageH;//将扫描大小以纸盘纸张大小为准
       }

       GImage* pCurScanImg = 0,   thePage(pageH, pageW, curSPPara.nColormode == 1 ? 0xFF: 0);   // 定义扫描图像pCurScanImg和thePage，生成实际要打印出来页面图像大小，图像大小由pageH, pageW也就是由纸盒内纸张幅面决定
	   if(debug) printf("+++++++++++++++%d++++++++++++++++++\n",curSPPara.nTint);
	   if(debug) printf("+++++++++++++++tints  is %x++++++++++++++\n" ,tints[curSPPara.nTint]);
       InitScanner(600,  curSPPara.nColormode, tints[curSPPara.nTint]);

       avaliableFrames = 0;
       int pages_write_to_fifo = 0;
       int PrnStartCmd = 0;
       GImage  *pDst = 0;
	   GImage* GroupsImg[GROUPPAGES];//存放分页图的指针数组,最多存5张图
	   int NextScanWaitTimes = 0;
	   int NextScanWaitTimesVal = 0;//得到实际分页扫描的页数
	   int totalpages = 0;
       if(curSPPara.bIsSFZ || curSPPara.nCombineMode || curSPPara.groups_pages)  // gray image mode.
       {
               NextScanWaitTimes = curSPPara.bIsSFZ  ? 2 : ( curSPPara.nCombineMode > 1 ?  4: 2);      // 剩余扫描次数
               if(curSPPara.groups_pages) NextScanWaitTimes = GROUPPAGES;//分页复印因为内存空间不足最多存储5页

               if(curSPPara.bIsSFZ)
			   {
					if(debug) printf("curScanRegion.W\n" , curScanRegion.W);
                    if(curScanRegion.W > 4000)
                    {
						if(debug) printf("curScanRegion.W > 4000\n");
						curScanRegion.H = Y_SFZW - Y_ORI;       curScanRegion.y = 0;    curScanRegion.W = Y_SFZW - Y_ORI;  curScanRegion.x = 0;
                    }
                    else if(curScanRegion.W > 1500)   //  纵向
					{
						curScanRegion.H = Y_SFZH - Y_ORI;       curScanRegion.y = 0;    curScanRegion.W = Y_SFZW - Y_ORI;  curScanRegion.x = 0;
						if(debug) printf("curScanRegion.W > 1500\n");
					}
                    else
                    {
                       curScanRegion.H = Y_SFZW - Y_ORI;       curScanRegion.y = 0;    curScanRegion.W = Y_SFZH - Y_ORI;  curScanRegion.x = 0;
                       if(debug) printf("curScanRegion.W 其他\n");
					}
			   }
			   if(curSPPara.groups_pages)//分页打印
			   {

				   while(NextScanWaitTimes)//每次扫描一张原稿,所有原稿扫描完成后跳出循环去打印
				   {
						GroupsImg[avaliableFrames] = DoAScan(curSPPara.nColormode);
						//NextScanWaitTimesVal++;
						avaliableFrames ++;
						NextScanWaitTimes --;
					   //扫描异步FIFO清零,不清零将会造成图像一个个偏移
						ScanCmdRegVal |=  SFIFO_CLR;            set_reg(SCAN_CMD_REG,  ScanCmdRegVal);   // bit 1  FIFO clear 信号
						usleep(US_TIME);
						ScanCmdRegVal &=  ~SFIFO_CLR;           set_reg(SCAN_CMD_REG,  ScanCmdRegVal);  // bit1  FIFO clear 信号脉冲
						//if(pCurScanImg){ delete pCurScanImg; pCurScanImg = 0; }
						if(0 == NextScanWaitTimes) break;

						 g_OKContinueScanPressed = 0;
						 curStage = NEXTSCAN;  UpdateUI = 1;  g_RunPressed = 0;
						 while(!g_OKContinueScanPressed )         // 等待按键OK 继续扫描；按键清除退出。
						 {
								  //drawMemUsed();//打印剩余空间大小
							      usleep(US_TIME);
							      if(g_exit){   tid_sp = 0;     curStage = READY;   UpdateUI = 1;     return  0;  }           // 退出
								  if(g_RunPressed)//中途点击开始
								  {
									  drawComAndPageRun();//
									  goto TAG3;//分页中途点击开始，直接打印已经存在的幅面
								  }
						 }
					}
					goto TAG3;//扫描完原稿，直接跳去分页打印
                }
				while(NextScanWaitTimes)//每次扫描一张原稿pCurScanImg，将其缩小拼接到thepage中后，清除pCurScanImg，循环多合一最后跳出循环去打印
				{
						pCurScanImg =  DoAScan(1);

						avaliableFrames ++;
						NextScanWaitTimes --;
						PageLayout(&thePage, pCurScanImg, avaliableFrames - 1);               if(pCurScanImg){ delete pCurScanImg; pCurScanImg = 0; }
						if(0 == NextScanWaitTimes) break;
						 g_OKContinueScanPressed = 0;
						 curStage = NEXTSCAN;  UpdateUI = 1;  g_RunPressed = 0;
						 while(!g_OKContinueScanPressed )         // 等待按键OK 继续扫描；　按键开始则直接打印；　按键清除退出。
						 {
							   usleep(US_TIME);
							   if(g_exit){     tid_sp = 0;     curStage = READY;   UpdateUI = 1;     return  0;  }               // 退出
							   if(g_RunPressed){
							        drawComAndPageRun();
									InitPrinter();                if(debug) printf("After InitPrinter \n");
									//  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
									PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
									goto TAG;//多合一中途点击开始，也可以直接去打印已经拼接后的图
							   }    // 跳出循环开始打印　　
						 }
				}

               InitPrinter();                                 if(debug) printf("After InitPrinter \n");
			   //  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
			   PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
          }
       else//普通打印
       {
			  InitPrinter();                                 if(debug) printf("After InitPrinter \n");
			  //  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
			  PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
              pCurScanImg = DoAScan(curSPPara.nColormode);//利用curScanRegion.W, curScanRegion.H参数，得到这个幅面的扫描数据
              pDst = pCurScanImg;


              int fR =  open("/media/repeat_scanprint.raw", O_RDWR|O_CREAT);    write(fR,  pDst->bits,  pDst->H*pDst->W);  close(fR);
              //更新数据库中的复印纸张的尺寸和字节数
              pagesize.hight = pDst->H;
              pagesize.weight = pDst->W;
              pagesize.totalsize = pDst->H*pDst->W;
              pagesize.cate = 2;
              pagesize.ncolormode = 2;
              UpdatePageSize(pagesize);
              if(debug) printf("++++++++++++++++++++curSPPara.nColormode: %d++++++++++++++++=\n",curSPPara.nColormode);

              //int fR =  open("/nfs/Page1.raw", O_RDWR|O_CREAT);    write(fR,  pDst->bits,  pDst->H*pDst->W);  close(fR);
		      if(curSPPara.nColormode == 2){ NegtiveImg(pDst); } // 半调图要反转黑白
              // 扫描一次
              if(curSPPara.nColormode == 1  || (curSPPara.nEraseBorder || curSPPara.nPageMargin))  //  灰度图或者半调图要削边或者页边距有设置
              {
                 OnePageLayout(&thePage, pDst, curSPPara.nColormode);  pDst = &thePage;  delete pCurScanImg;
              }
       }


TAG:    // 打印
        if(curSPPara.nColormode == 1)  // gray image halftone by software.
        {
            pDst = new GImage(thePage.H, (thePage.W+7) / 8);   HalfToneB(&thePage,  pDst);
        }

        if(g_exit)   { goto TAG2; }

        //InitPrinter();                                 if(debug) printf("After InitPrinter \n");                                                     // 打印硬件复位并初始化
        SetPrintJobInfo(pDst->H, pDst->W);             if(debug) printf("After SetPrintJobInfo \n");
                                                 // 下达打印参数
        //if(debug){ int fR =  open("/nfs/Page2.raw", O_RDWR|O_CREAT);    write(fR,  pDst->bits,  pDst->H*pDst->W);  close(fR); };
        get_time(user.handle_time);
		sprintf(user.handle_type, "%s", "复印");
		sprintf(user.paper_type, "%s",SPaperName(curSPPara.nPaperSize));
		user.user_id =  curSPPara.nUserID;
		user.copys = curSPPara.nCopy;
		//TestBorder(pDst);
        for(int i = 0;  i < curSPPara.nCopy;  i++)
		{
                if(g_exit) {   tid_sp = 0;     curStage = READY;    UpdateUI = 1;      goto TAG2; }
                PrintOnePage(pDst, pages_write_to_fifo, curSPPara.nCopy, PrnStartCmd);

         }
		if(curSPPara.nMode==0)  update_user_entry(pages_write_to_fifo);//更新当前用户已打印页数

       WaitThisPrintJobFinish();
       goto TAG2;                                // 等待打印结束
TAG3://分页打印流程
	  InitPrinter();                                 if(debug) printf("After InitPrinter \n");
	   //  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
	  PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
	 for(int i = 0;  i < curSPPara.nCopy;  i++)//外部循环打印份数
	 {
		for(int j = 0;  j < avaliableFrames;  j++)//分页复印因为内存空间不足暂时最多存储10页
		{//内部循环每一份的分页数
			  pDst = GroupsImg[j];
			  if(i==0){
				if(curSPPara.nColormode == 2){ NegtiveImg(pDst); } // 半调图要反转黑白,并且只有在第一次发送图像时
			  }
			  // 扫描一次
			  if(curSPPara.nColormode == 1  || (curSPPara.nEraseBorder || curSPPara.nPageMargin))  //  灰度图或者半调图要削边或者页边距有有设置
			  {
				 OnePageLayout(&thePage, pDst, curSPPara.nColormode);  pDst = &thePage; // delete pCurScanImg;
			  }
			  if(curSPPara.nColormode == 1)  // gray image halftone by software.
			  {
				 pDst = new GImage(thePage.H, (thePage.W+7) / 8);   HalfToneB(&thePage,  pDst);
			  }
			  if(g_exit)   { goto TAG2; }
			  if(j==0&&i==0)//因为发送的扫描图像尺寸相同，所以只在第一页第一次发送的时候向寄存器写定义幅面大小
			  {
				SetPrintJobInfo(pDst->H, pDst->W);             if(debug) printf("After SetPrintJobInfo 1 \n");
				get_time(user.handle_time);
				sprintf(user.handle_type, "%s", "复印");
				sprintf(user.paper_type, "%s",SPaperName(curSPPara.nPaperSize));
				user.user_id =  curSPPara.nUserID;
				user.copys = curSPPara.nCopy;
			  }
			  //if(j==1){ int fR =  open("/nfs/Page1.raw", O_RDWR|O_CREAT);    write(fR,  pDst->bits,  pDst->H*pDst->W);  close(fR); };
			  if(g_exit) {   tid_sp = 0;     curStage = READY;    UpdateUI = 1;      goto TAG2; }
		      PrintOnePage(pDst, pages_write_to_fifo, curSPPara.nCopy*avaliableFrames, PrnStartCmd);//这里乘以2是因为要得到分页打印所需所有的页数，这里只存了两页图，所以乘以2
		}
	 }
	  if(curSPPara.nMode==0)  update_user_entry(pages_write_to_fifo);//更新当前用户已打印页数

	  WaitThisPrintJobFinish();


TAG2:
	 // printf("before %d\n",GetMemLost());
	  for(int i = 0;  i < avaliableFrames-1;  i++)//释放内存,这里减1是因为只释放除最后一张图外的前几个幅面数据，而pDst此时指向最后一幅图，在后面释放
	  {
		 if(GroupsImg[i]){ delete GroupsImg[i]; GroupsImg[i] = 0; }
	  }
	 // printf("after %d\n",GetMemLost());
      if(pDst && curSPPara.nColormode == 1) delete  pDst;
      //printf("last %d\n",GetMemLost());

      {//目的是防止最后一页缺纸时，状态检测太快，使得补纸后状态又恢复到复印
		   sleep(1);
		while(curStage==WARNING&&oldStage==SING_PING){
			sleep(1);
		  }
	  }
      tid_sp = 0;        curStage = READY;      UpdateUI = 1;            //  终止线程时可判断该值。

      return 0;
}



void* RepeatScanPrint(void*)
{
       if(debug) printf("____________________Repeat ScanPrint Thread In__________________\n");
	   DoUIOperationCheck();//做互斥检查
       int pageH, pageW;   GetPageSize(pageH, pageW);//由传感器值判断纸盒（纸盒或手动）中纸张，将数据长宽赋给pageH,pageW

       curSPPara.nColormode = 1;      // 1 gray, 2 binary

       if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100 && !ScanImageNeedRotate && !curSPPara.bIsSFZ)//x100%y100%，图像不需要旋转，也不是身份证复印，则直接扫描出二值图即可
       {
          curSPPara.nColormode = 2;
          pageW /= 8;   // 半调扫描，宽度除8.灰度一个点8bit数据，半调只有1bit非黑即白
       }
       if(debug) printf("PageH %d, PageW %d, curScanRegion.W %d, curScanRegion.H %d, nColor %d\n", pageH, pageW, curScanRegion.W, curScanRegion.H, curSPPara.nColormode);                                                          // 根据纸张检测结果，获得当前打印页面图像的宽度和高度 600 dpi

      // GImage* pCurScanImg = 0,   thePage(pageH, pageW, curSPPara.nColormode == 1 ? 0xFF: 0);  // 定义扫描图像pCurScanImg和thePage，生成实际要打印出来页面图像大小，图像大小由pageH, pageW也就是由纸盒内纸张幅面决定


       int pages_write_to_fifo = 0;
       int PrnStartCmd = 0;

	   int realPageH , realPageW ,totalsize,nColormode;
		GImage *pDst = new GImage(4960,  7016 );
		if(debug) printf("((((((((((((((((((((repeat_scanprint_sign :%d , repeat_scan_sign : %d))))))))))))))))))))\n",repeat_scanprint_sign,repeat_scan_sign);
	   if(repeat_scanprint_sign ==1 ) //重复复印流程
	   {
			ReadPageSize(realPageH , realPageW, nColormode ,2);
			curSPPara.nColormode =nColormode;
			pDst->H = realPageH;
			pDst->W = realPageW;
			InitPrinter();                                 if(debug) printf("After InitPrinter \n");
			//  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
			PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
			int fR =  open("/media/repeat_scanprint.raw", O_RDWR|O_CREAT);   totalsize =  read(fR,  pDst->bits, pDst->H*pDst->W);  close(fR);
			NegtiveImg(pDst);
	   }
	   else if(repeat_scan_sign ==1 ) //重复扫描扫描
	   {
			ReadPageSize(realPageH , realPageW ,nColormode,1);

			curSPPara.nColormode =nColormode;
			if(curSPPara.nColormode == 2)	//二值重复
			{
				pDst->H = realPageH;
				pDst->W = realPageW;
				InitPrinter();                                 if(debug) printf("After InitPrinter \n");
				//  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
				PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
				int fR =  open("/media/repeat_scan.raw", O_RDWR|O_CREAT);   totalsize =  read(fR,  pDst->bits, pDst->H*pDst->W);  close(fR);
				NegtiveImg(pDst);

			}
			else if(curSPPara.nColormode == 1) //灰度扫描
			{
				GImage *pSrc = new GImage(realPageH,  realPageW );
				pDst->H = realPageH;
				pDst->W = realPageW/8;
				InitPrinter();                                 if(debug) printf("After InitPrinter \n");
				//  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
				PrnCmdRegVal  |= 0x00E2;     set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
				int fR =  open("/media/repeat_scan.raw", O_RDWR|O_CREAT);   totalsize =  read(fR,  pSrc->bits, pSrc->H*pSrc->W);  close(fR);
				curSPPara.nNegtive = 0;
				HalfToneB(pSrc,  pDst);
				delete pSrc;
			}

	   }
		if(g_exit)   { goto TAG2; }

TAG:    // 打印

        //InitPrinter();                                 if(debug) printf("After InitPrinter \n");
        if(debug) printf("--------------pDst->H:%d  ,  pDst->W:%d-------------\n" , pDst->H,pDst->W);                                          // 打印硬件复位并初始化
        SetPrintJobInfo(pDst->H, pDst->W);             if(debug) printf("After SetPrintJobInfo \n");
		// 下达打印参数
        get_time(user.handle_time);
		sprintf(user.handle_type, "%s", "复印");
		sprintf(user.paper_type, "%s",SPaperName(curSPPara.nPaperSize));
		user.user_id =  curSPPara.nUserID;
		user.copys = curSPPara.nCopy;
//		printf("user.handle_type is %s\n" , user.handle_type);
//		printf("user.paper_type is %s\n" , SPaperName(curSPPara.nPaperSize));
//		printf("curSPPara.nPaperSize is %x " , curSPPara.nPaperSize);
//		printf("user.user_id is %d\n" , curSPPara.nUserID);
//		printf("user.copys is %d\n" , curSPPara.nCopy);


        for(int i = 0;  i < curSPPara.nCopy;  i++)
		{
                if(g_exit) {   tid_sp = 0;     curStage = READY;    UpdateUI = 1;      goto TAG2; }
                PrintOnePage(pDst, pages_write_to_fifo, curSPPara.nCopy, PrnStartCmd);

         }
		//if(curSPPara.nMode==0)  update_user_entry(&mysql, pages_write_to_fifo);//更新当前用户已打印页数

       WaitThisPrintJobFinish();
       goto TAG2;                                // 等待打印结束

TAG2:

      if(pDst && curSPPara.nColormode == 1) delete  pDst;

      {//目的是防止最后一页缺纸时，状态检测太快，使得补纸后状态又恢复到复印
		   sleep(1);
		while(curStage==WARNING&&oldStage==SCANNING){
			sleep(1);
		  }
	  }
	  curStage = READY;      UpdateUI = 1;            //  终止线程时可判断该值。

	  OnRESET();
	  drawReadyToSP();
      return 0;
}
