#include "PrintJob.h"
#include "os_def.h"
#include "BufUsb.h"
#include "PclParser.h"
#include "ScanPrintTypes.h"
#include "LcdUI.h"

#include "GPMC.h"
#include "regsdef.h"
#include <fcntl.h>
#include <unistd.h>
#include "image.h"
#include <time.h>
#include "pthread.h"
#include "globals.h"


struct copes{
    char * buf;
    int len;
    int pages;//每次打印的页数
};
copes copes_buf;
copes copes_file;
int dest_file;
int copy_count=-1;
#define COPE_LEN 1024*1024*128


unsigned short  PrnCmdRegVal = 0x6000,   PModeRegVal = 0;

// rlc : 0 压缩， 1 不压缩
void PrintJobSetting(PrnPara setting)
{
    if(debug) printf("H = %d, W = %d,  copies = %d,  resolution = %d, paper src = %d , rlc =  %d , paper_type = %d\n",setting.nLines, setting.nColumns,  setting.nCopies,   setting.nResolution,  setting.nPaperSrc, setting.nRlc, setting.nCopyTrue);
	//if(setting.nLines == 9921) setting.nLines = 9920;
}
/*
*	read usb and proc. returned when finish the configuratio. that is, data meeted	note, there may be some data left in the usb buffer.
*/
int ReadJobCfg(PrnPara *setting)
{
	int finished  = 0;
	PCL pcl;
	while(!finished)
	{
		if (ReadOnePacket2UsbBuf(&_bufUsb) == 0)      break;
		while(USB_DATA_SIZE_FN > USB_PROC_MIN_SIZE && !finished)
		{
			if(PclParse(_bufUsb.pStart, USB_DATA_SIZE_FN, &pcl))	{	return ERR_PARSE_PCL_FAILED;	}
			switch(pcl.cmd)
			{
                case PCL_COPIES:   			  setting->nCopies = pcl.value;			             break;
                case PCL_RASTER_HEIGHT:       setting->nLines   = pcl.value;    		             break;
                case PCL_RASTER_WIDTH:        setting->nColumns = pcl.value;		             break;
                case PCL_COLOR_MODE: 	      setting->nColorMode = pcl.value;  	         break;
                case PCL_RASTER_START:        setting->nRlc = pcl.value;  	finished = 1;	 break;//光栅数据开始命令，收到此命令表示之后为图像数据
                case PCL_RESOLUTION:  		   setting->nResolution = pcl.value;		         break;
                case PCL_PAPER_SRC:			   setting->nPaperSrc = pcl.value;                   break;  // 0 自动， 1 纸盒
                case PCL_PAPER_COPY:           setting->nCopyTrue = pcl.value;                 break;
                case PCL_PAGE_SIZE:            setting->nPaperSize = pcl.value;                 break;
                case PCL_PAPER_CMBMODE:        setting->nPrnCombineMode  = pcl.value;     break;  // 0 禁用， 2， 4， 6， 9 ， 16  合 1
                case PCL_SINGLE_DOUBLE:        setting->nSingle_Double    = pcl.value;        break;  // 0 单面， 1 长边翻转， 2短边翻转
			}
			_bufUsb.pStart += pcl.len;
		}//while
		UsbBufRearrange(&_bufUsb);
	}
	return 0;
}

/*
*	has by-effect. can't call continiously.
*/
int ConfirmNextPage(int* existNext)
{
	PCL pcl;
	*existNext = 0;

	if(USB_DATA_SIZE_FN < PCL_TAIL_LEN_AFTER_DATA)
	{
		ReadOnePacket2UsbBuf(&_bufUsb);
	}
	if(PclParse(_bufUsb.pStart, USB_DATA_SIZE_FN, &pcl))  //end of page
	{
		return ERR_PARSE_PCL_FAILED;
	}
	_bufUsb.pStart += pcl.len;

	if(PclParse(_bufUsb.pStart,USB_DATA_SIZE_FN,&pcl))  //maybe start maybe job end
	{
		return ERR_PARSE_PCL_FAILED;
	}
	if(pcl.cmd == PCL_RASTER_START)
	{
		*existNext = 1;
    #ifdef DEBUG
        if(debug) printf("More Pages Waiting... \n");
    #endif
	}
    #ifdef DEBUG
        else if(pcl.cmd == PCL_RESET)
        {
            if(debug) printf("All Pages Done \n");
        }
        else
        {
            if(debug) printf("wrong command. neither PCL_RESET nor PCL_RASTER_START \n");
        }
    #endif
	_bufUsb.pStart += pcl.len;
	return 0;
}

void  HwReset()
{

      //PrnCmdRegVal = 0x4000;
      PrnCmdRegVal = 0x6000;
      PModeRegVal = 0x3000;                              // Arm 读写永远为1
      PrnCmdRegVal |= HW_RESET;
      set_reg(PRN_CMD_REG,  PrnCmdRegVal);              // 系统唤醒， 系统复位， 高电平
      if(debug) printf("PrnCmdRegVal = %x\n",PrnCmdRegVal);
      usleep(US_TIME);
      PrnCmdRegVal &= 0xFFFE;
      set_reg(PRN_CMD_REG, PrnCmdRegVal);                // 然后恢复低电平， 形成一个脉冲信号，复位。
      usleep(US_TIME);
     // set_reg(0x25 ,0x07D0);                              //碳粉浓度阈值2000
}

int isSeriousError()
{
      return  _state.status[0] & 0xFF ;
}
// 返回是否处理了异常的标志，　0　异常未处理，结束作业，　1　处理了异常，继续进行。
int ExceptionHandler()
{
      while(  curStage == WARNING )
	  {
	        if(isSeriousError())  // 严重错误的话，结束打印， 恢复状态
	        {
	            PrnCmdRegVal |= PRINT_TASK_HALT;       set_reg(PRN_CMD_REG, PrnCmdRegVal);
                unsigned short val = get_reg(STATUS_PAPERSZ_REG);
                while( !(val | 0x4000) )  // 第14位 1 表示打印作业结束
                {
                        val = get_reg(STATUS_PAPERSZ_REG);         PrnCmdRegVal |= PRINT_TASK_HALT;           set_reg(PRN_CMD_REG,  PrnCmdRegVal);
                        usleep(200000);
                //        if(debug) printf(" read STATUS_PAPERSZ_REG , 等待打印作业结束: %02x\n", val);
                        if(g_exit)  return (0) ;
                }
	        }
	         usleep(1000000);
	         if(g_exit) {     PrnCmdRegVal |= PRINT_TASK_HALT;    set_reg(PRN_CMD_REG,  PrnCmdRegVal);   return (0) ;}
      }
      return 1;
}

 int  WaitHwInitOK()
 {
        int status  = get_reg(STATUS_PAPERSZ_REG);

        while(!( status & HW_INIT_READY))  // CMD_REG第 15位 为1 ， 表示初始化完毕。
        {
              if(debug) printf(" read STATUS_PAPERSZ_REG : %02x\n", status);
              status  = get_reg(STATUS_PAPERSZ_REG);
              sleep(3);
              if(g_exit) return (0) ;
        }
        return  ExceptionHandler();   // 检测硬件状态，正常可继续
 }
// 输入硬件纸张码，输出驱动纸张码.
int   GetPaperBoxPaper(int PaperBoxPaperVal)
{
       switch(PaperBoxPaperVal)
       {
             case 0x33:                                  //  Vertical  A4
             case 0x38:
             case 0x18:     return  9;               //   HorizontalA4
             case 0x3F:
             case 0x37:     return  8;              // A3
             case 0x10:                                   // B5 == , 16K==
             case 0x16:     return 13;             // B5 || =  16K ||
             case 0x1F:     return 12 ;           // B4== 8K ==
       }
       return 9;
}

// 根据纸盒中的纸张尺寸，判断是否要旋转打印驱动传来的数据。
int PrnDataNeedRotate(int BoxPaperSz)
{
      switch(BoxPaperSz & 0x0F)
      {
            case 0x06:                                          // B5 || =  16K ||   ( 0x16 )
            case 0x03:  return 1;                         // 0x03 A4 Vertical,
      }
      return  0;
}

 int  b_Rotate = 0;                                           //纸盒中的打印纸Vertical 放置时，应该旋转打印数据，　纸盘中默认纵向，不旋转。
 void SetJobSettings()
 {
           unsigned short rval = 0x0000, psz = 0;  // 纸张来源 0 表示自动，１纸盒　(打印驱动下传的值)
           if ( !curPrnPara.nPaperSrc && curSPPara.nPaperBox == 2) // 若打印驱动设置为自动选纸（0），而机器设置了手动纸张;  手动纸盘有纸准备好, 则设置为手动进纸 (bit9置1)
            {
                   if(handpaperfeeder_avaliable() )
                   {
                       while( ! HandPaperMatch(curPrnPara.nPaperSize) )  // 手动纸张和打印纸张尺寸不一致
                       {
                              curStage = HANDPAPER;   UpdateUI = 1;  sleep(2);
                       }
                       curStage = PRINTING;
                       PModeRegVal |= (  1<<  9 );       set_reg(PMODE_REG, PModeRegVal);     psz = nHandPaperSz; // （1）纸张来源 bit 9 置1表示手动纸张有效, 0 表示纸盒有效
                   }
            }else  // 打印选址纸盒 或者 机器设置纸盒
            {
                  PModeRegVal &=  ~(1<<  9);                 set_reg(PMODE_REG, PModeRegVal);       // （1）纸张来源(打印驱动用0 表示自动，１表示纸盒) ，FPGA  bit 9 置1表示手动纸张有效, 0 表示纸盒有效。
                  rval = get_reg(STATUS_PAPERSZ_REG) ;  psz = (rval  & 0x3F) ;                                // box paper size
                  while(GetPaperBoxPaper(psz) != curPrnPara.nPaperSize )                                  // 但如果纸盒纸张大小不符，则尝试手动纸盘并等待。
                  {
					//printf("curPrnPara.nPaperSize=%x",curPrnPara.nPaperSize);
                    if(debug)   printf("Papersize in PaperBox NOT Match,   rval & 0xFF = %02x\n", rval & 0x3F);
                    unsigned short  wVal = get_reg(HW_SENSOR_REG);    wVal |=  0x0001;     set_reg(HW_SENSOR_REG,  wVal);   // 纸张不符报警，　0x2A REG  bit 0
                    if(g_exit)    {  PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);      wVal &=  0xFFFE;    set_reg(HW_SENSOR_REG,  wVal);    oldStage = READY;   UpdateUI = 1;   return; }
                    rval = get_reg(STATUS_PAPERSZ_REG) ;    psz = (rval  & 0x3F) ;      // box paper size
                    sleep(1);
                }
        }
       PModeRegVal |=  psz;        set_reg(PMODE_REG, PModeRegVal);   // 纸张大小设置       // （2） 纸张大小  0x33 , 0x38, 0x37 = 0x30 + 7(bit0~bit3)
       unsigned short  wVal = get_reg(HW_SENSOR_REG);    wVal &=  0xFFFE;     set_reg(HW_SENSOR_REG,  wVal);     // 清除纸张不符　0x2A REG 　 bit 0
       PModeRegVal  |=      (curPrnPara.nSingle_Double  << 10 ) ;    set_reg(PMODE_REG,  PModeRegVal);                      // （3）单双面打印设置/ bit 10 ：  0 单面打印， 1 长边翻转， 2 短边翻转
      // 压缩与否 Arm Wr bit 13,  Arm Read  bit 12 置1,
       PModeRegVal |= 0x3000;    PModeRegVal |= ( (!curPrnPara.nRlc) << 14 );    set_reg(PMODE_REG, PModeRegVal);  //  nRlc 0 , 未压缩数据， 14 bit 置1， 使得解压缩不进行。
       //  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 作业开始信号 bit 5 置1
       PrnCmdRegVal  |= 0x00E2;    set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
       usleep(US_TIME);
       PrnCmdRegVal &= 0xFFFD;    set_reg(PRN_CMD_REG,  PrnCmdRegVal);    // 1111 1101    //  bit 1 脉冲信号

       b_Rotate = PrnDataNeedRotate(psz);  // 纸盒中的纸张如果类似A４ Vertical (纸张长边做为打印头一行),  需要旋转页面；　
      // if(curPrnPara.nLines==9921) curPrnPara.nLines = 9920;
       if(b_Rotate)
       {
             set_reg(PAGEH_REG,(curPrnPara.nColumns +7)/8*8);     set_reg(LINEBYTES_REG,  (curPrnPara.nLines + 7) / 8 );
       }
       else
       {
		if(curPrnPara.nLines==9921)	set_reg(PAGEH_REG, 9920);//需要保证发送偶数个数据量，而A3 9921需要改为9920×887
		else set_reg(PAGEH_REG, curPrnPara.nLines);
			set_reg(LINEBYTES_REG,  (curPrnPara.nColumns + 7) / 8 );   //页宽，字节单位
			if(debug) printf("H curPrnPara.nLines = %d,W curPrnPara.nColumns = %d\n",curPrnPara.nLines ,curPrnPara.nColumns);
       }
       if(debug)   printf("bRotate: %d,  PaserSrc: %d,  PaperSz: %02x,  PModeRegVal: %02x\n", b_Rotate,  curPrnPara.nPaperSrc,  psz,  PModeRegVal);
 }


void SetJobSettingsNotAction()//不发送作业开始信号,只是单纯设置走流程
 {
           unsigned short rval = 0x0000, psz = 0;  // 纸张来源 0 表示自动，１纸盒　(打印驱动下传的值)
           if ( !curPrnPara.nPaperSrc && curSPPara.nPaperBox == 2) // 若打印驱动设置为自动选纸（0），而机器设置了手动纸张;  手动纸盘有纸准备好, 则设置为手动进纸 (bit9置1)
            {
                   if(handpaperfeeder_avaliable() )
                   {
                       while( ! HandPaperMatch(curPrnPara.nPaperSize) )  // 手动纸张和打印纸张尺寸不一致
                       {
                              curStage = HANDPAPER;   UpdateUI = 1;  sleep(2);
                       }
                       curStage = PRINTING;
                       PModeRegVal |= (  1<<  9 );       set_reg(PMODE_REG, PModeRegVal);     psz = nHandPaperSz; // （1）纸张来源 bit 9 置1表示手动纸张有效, 0 表示纸盒有效
                   }
            }else  // 打印选址纸盒 或者 机器设置纸盒
            {
                  PModeRegVal &=  ~(1<<  9);                 set_reg(PMODE_REG, PModeRegVal);       // （1）纸张来源(打印驱动用0 表示自动，１表示纸盒) ，FPGA  bit 9 置1表示手动纸张有效, 0 表示纸盒有效。
                  rval = get_reg(STATUS_PAPERSZ_REG) ;  psz = (rval  & 0x3F) ;                                // box paper size
                  while(GetPaperBoxPaper(psz) != curPrnPara.nPaperSize )                                  // 但如果纸盒纸张大小不符，则尝试手动纸盘并等待。
                  {
					//printf("curPrnPara.nPaperSize=%x",curPrnPara.nPaperSize);
                    if(debug)   printf("Papersize in PaperBox NOT Match,   rval & 0xFF = %02x\n", rval & 0x3F);
                    unsigned short  wVal = get_reg(HW_SENSOR_REG);    wVal |=  0x0001;     set_reg(HW_SENSOR_REG,  wVal);   // 纸张不符报警，　0x2A REG  bit 0
                    if(g_exit)    {  PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);      wVal &=  0xFFFE;    set_reg(HW_SENSOR_REG,  wVal);    oldStage = READY;   UpdateUI = 1;   return; }
                    rval = get_reg(STATUS_PAPERSZ_REG) ;    psz = (rval  & 0x3F) ;      // box paper size
                    sleep(1);
                }
        }
       PModeRegVal |=  psz;        set_reg(PMODE_REG, PModeRegVal);   // 纸张大小设置       // （2） 纸张大小  0x33 , 0x38, 0x37 = 0x30 + 7(bit0~bit3)
       unsigned short  wVal = get_reg(HW_SENSOR_REG);    wVal &=  0xFFFE;     set_reg(HW_SENSOR_REG,  wVal);     // 清除纸张不符　0x2A REG 　 bit 0
       PModeRegVal  |=      (curPrnPara.nSingle_Double  << 10 ) ;    set_reg(PMODE_REG,  PModeRegVal);                      // （3）单双面打印设置/ bit 10 ：  0 单面打印， 1 长边翻转， 2 短边翻转
      // 压缩与否 Arm Wr bit 13,  Arm Read  bit 12 置1,
       PModeRegVal |= 0x3000;    PModeRegVal |= ( (!curPrnPara.nRlc) << 14 );    set_reg(PMODE_REG, PModeRegVal);  //  nRlc 0 , 未压缩数据， 14 bit 置1， 使得解压缩不进行。
       //  打印参数设置完成 bit 6 置1， FIFO异步清零 bit  1 来个 脉冲， 功能模块使能 bit 7  置1， 不设置作业开始信号 bit 5 置1
       PrnCmdRegVal  |= 0x00C2;    set_reg(PRN_CMD_REG, PrnCmdRegVal);     // 1 1 1 0 0 0 1 0
       usleep(US_TIME);
       PrnCmdRegVal &= 0xFFFD;    set_reg(PRN_CMD_REG,  PrnCmdRegVal);    // 1111 1101    //  bit 1 脉冲信号

       b_Rotate = PrnDataNeedRotate(psz);  // 纸盒中的纸张如果类似A４ Vertical (纸张长边做为打印头一行),  需要旋转页面；　
       if(b_Rotate)
       {
             set_reg(PAGEH_REG,(curPrnPara.nColumns +7)/8*8);     set_reg(LINEBYTES_REG,  (curPrnPara.nLines + 7) / 8 );
       }
       else
       {
		if(curPrnPara.nLines==9921)	set_reg(PAGEH_REG, 9920);
		else set_reg(PAGEH_REG, curPrnPara.nLines);
			 set_reg(LINEBYTES_REG,  (curPrnPara.nColumns + 7) / 8 );   //页宽，字节单位
       }
       if(debug)   printf("bRotate: %d,  PaserSrc: %d,  PaperSz: %02x,  PModeRegVal: %02x\n", b_Rotate,  curPrnPara.nPaperSrc,  psz,  PModeRegVal);
 }

char* PaperName(int drvPaperSz)
{
    switch (drvPaperSz)
    {
           case 0x8: return "A3";
           case 0x9: return "A4";
           case 0x12:return "B4(8K)";
           case 0x13:return "B5(16K)";
    }
    return "Unknown";
}

void DoPrint()
{
	int existNext = 0;
	int len = (curPrnPara.nColumns + 7) / 8 * curPrnPara.nLines ;
	unsigned char * buf = new unsigned char[len],  *ptrData = buf;
	int  PrnStartCmd = 0;
	_state.nPageCount = _state.printed_count = 0;
	if(!buf)
	{  curStage = WARNING;       PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("serious,  not  enough print mem\n"); pthread_exit(0) ; }  // 结束打印 ， bit 3 置1

     get_time(user.handle_time);
	 sprintf(user.handle_type, "%s", "打印");
	 sprintf(user.paper_type, "%s", PaperName(curPrnPara.nPaperSize));
	 user.user_id =  curSPPara.nUserID;
	 user.copys = curPrnPara.nCopies;
     GImage *img_rotated = 0;
	do{
	        int l = 0,  count = 0,  i = 0;
            PCL pcl;
            while(l < curPrnPara.nLines)
            {
                if(USB_FREE_SIZE_FN < USB_TRANSFER_PACKAGE_SIZE)
                {
                    UsbBufRearrange(&_bufUsb);
                }
                //parse line data
                if(PclParse(_bufUsb.pStart, USB_DATA_SIZE_FN, &pcl))
                {
                    curStage = WARNING;    PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("PclParse  Error\n");  delete[] buf;  pthread_exit(0)  ;
                }
                if(pcl.cmd != PCL_RASTER_DATA)
                {
                    curStage = WARNING;      PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("PclParse  Error\n");  delete[] buf;   pthread_exit(0)  ;
                }

                if(USB_DATA_SIZE_FN < pcl.len + pcl.value + PCL_COMMAN_MAX_SIZE )
                {
                    if (ReadOnePacket2UsbBuf(&_bufUsb) == 0)     break;
                }
                else
                {
                    memcpy(buf + count,  _bufUsb.pStart + pcl.len,  pcl.value);    // dst, src, size
                    count += pcl.value;   //逐行拷贝压缩数据
                    l++;
                    _bufUsb.pStart += pcl.len + pcl.value;
                }
            }
            ConfirmNextPage(&existNext);


            // 页面需要旋转
            if(b_Rotate)
            {
				   if(debug) printf("before new GImage  rotated row:%d, col:%d\n", curPrnPara.nColumns, (curPrnPara.nLines + 7) / 8);
                   img_rotated = new GImage((curPrnPara.nColumns + 7) / 8 * 8,  (curPrnPara.nLines + 7) / 8);
                    if(!img_rotated)
                    {
                         curStage = WARNING;       PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("Serious,  not  enough print mem to Rotate Page Image\n");  delete[] buf;    pthread_exit(0)  ;
                    }
                   GImage img_src(curPrnPara.nLines, (curPrnPara.nColumns+7) / 8, 0,  buf);
                   bzero(img_rotated->bits, img_rotated->H * img_rotated->W );
                   if(debug) printf("before RotateHalfTone90 \n");
                   RotateHalfTone90(img_src,  img_rotated);
                   if(debug) printf("After RotateHalfTone90 \n");
                   ptrData = img_rotated->bits;
                   count = img_rotated->H * img_rotated->W;
            }

            for(i = 0;  i < curPrnPara.nCopies ;  i++)
            {

                  if(!ExceptionHandler() )
                   {
                             PrnCmdRegVal |= PRINT_TASK_HALT;      set_reg(PRN_CMD_REG,  PrnCmdRegVal);   delete[] buf;      if(img_rotated)  delete img_rotated;       pthread_exit(0)  ;
                   }
                  while(Mem_Avaliable() <  count)  // 若FIFO空间不足一页，则等待FIFO.
                   {
                           usleep(US_TIME);    if(debug) printf("Waiting  FIFO\n ");   if(g_exit) {    delete[] buf;      if(img_rotated)  delete img_rotated;       pthread_exit(0) ;    }
                   }

                   if(debug) printf("before write_fifo\n");

                  // if(debug){ int fR =  open("/nfs/Page3.raw", O_RDWR|O_CREAT);    write(fR,  img_rotated->bits,  img_rotated->H*img_rotated->W);  close(fR); };
                  if(curPrnPara.nLines == 9921) count = (curPrnPara.nColumns + 7) / 8 * 9920;//A3需要少发送一行数据，保证偶数据量
                  //printf("count = %d\n",count);
                   write_fifo (GPMC_FIFO_WRITE,  ptrData,  count);      _state.nPageCount ++;


				  {
				     AllKindPages.printed++;   AllKindPages.total++;   //WR_AllKindsofPages(AllKindPages);
				     user.paper_nums =  _state.nPageCount;
				     if(curSPPara.nMode==0)//在安全模式且没有ID和权限错误下统计
				     {
						 if(user.paper_nums == 1) insert_user_entry();
						 //else update_user_entry(&mysql, user.paper_nums);
				     }
                  }  // 内部打印计数

                   if(  _state.nPageCount  > 0  || !existNext)
		           {
		                  usleep(US_TIME);         set_reg(PAGE_NUM_REG,   _state.nPageCount);
		                  if(!PrnStartCmd)
		                  {

		                        usleep(US_TIME);   PrnStartCmd = 1;    PrnCmdRegVal |=  PRINT_START;         set_reg(PRN_CMD_REG,  PrnCmdRegVal);     // 下达打印开始命令 //  bit  4 置 1
		                  }
		           }
		           usleep(US_TIME);   _state.printed_count = get_reg(PAGES_PRINTED_REG);  // 已打印份数

		           if(debug) printf("Printed Pages %d pages in fifo = %d\n", _state.printed_count,  _state.nPageCount);

           }
           if(img_rotated)  { if(debug) printf("before delete img_rotated\n"); delete img_rotated; img_rotated = 0;}

    }while(existNext );
	 if(curSPPara.nMode==0) update_user_entry(user.paper_nums);//更新用户打印页数
     write_fifo(GPMC_FIFO_WRITE,  buf,  256);    //再写入２５６字节的FIFO填充数据
     if(debug) printf("before delete buf\n");
     delete[] buf;
}

void DoNotPrint()//为了usb打印，只接受上位机数据，而不向fifo写，单纯走完流程
{
	int existNext = 0;
	int len = (curPrnPara.nColumns + 7) / 8 * curPrnPara.nLines ;
	unsigned char * buf = new unsigned char[len],  *ptrData = buf;
	int  PrnStartCmd = 0;
	_state.nPageCount = _state.printed_count = 0;
	if(!buf)
	{  curStage = WARNING;       PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("serious,  not  enough print mem\n"); pthread_exit(0) ; }  // 结束打印 ， bit 3 置1

     //get_time(user.handle_time);
	 //sprintf(user.handle_type, "%s", "打印");
	 //sprintf(user.paper_type, "%s", PaperName(curPrnPara.nPaperSize));
	 user.user_id =  curSPPara.nUserID;
     GImage *img_rotated = 0;
	do{
	        int l = 0,  count = 0,  i = 0;
            PCL pcl;
            while(l < curPrnPara.nLines)
            {
                if(USB_FREE_SIZE_FN < USB_TRANSFER_PACKAGE_SIZE)
                {
                    UsbBufRearrange(&_bufUsb);
                }
                //parse line data
                if(PclParse(_bufUsb.pStart, USB_DATA_SIZE_FN, &pcl))
                {
                    curStage = WARNING;    PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("PclParse  Error\n");  delete[] buf;  pthread_exit(0)  ;
                }
                if(pcl.cmd != PCL_RASTER_DATA)
                {
                    curStage = WARNING;      PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    if(debug) printf("PclParse  Error\n");  delete[] buf;   pthread_exit(0)  ;
                }

                if(USB_DATA_SIZE_FN < pcl.len + pcl.value + PCL_COMMAN_MAX_SIZE )
                {
                    if (ReadOnePacket2UsbBuf(&_bufUsb) == 0)     break;
                }
                else
                {
                    memcpy(buf + count,  _bufUsb.pStart + pcl.len,  pcl.value);    // dst, src, size
                    count += pcl.value;   //逐行拷贝压缩数据
                    l++;
                    _bufUsb.pStart += pcl.len + pcl.value;
                }
            }
            ConfirmNextPage(&existNext);

//
//             页面需要旋转
//            if(b_Rotate)
//            {
//				   printf("before new GImage  rotated row:%d, col:%d\n", curPrnPara.nColumns, (curPrnPara.nLines + 7) / 8);
//                   img_rotated = new GImage((curPrnPara.nColumns + 7) / 8 * 8,  (curPrnPara.nLines + 7) / 8);
//                    if(!img_rotated)
//                    {
//                         curStage = WARNING;       PrnCmdRegVal |= PRINT_TASK_HALT;     set_reg(PRN_CMD_REG, PrnCmdRegVal);    printf("Serious,  not  enough print mem to Rotate Page Image\n");  delete[] buf;    pthread_exit(0)  ;
//                    }
//                   GImage img_src(curPrnPara.nLines, (curPrnPara.nColumns+7) / 8, 0,  buf);
//                   bzero(img_rotated->bits, img_rotated->H * img_rotated->W );
//                   printf("before RotateHalfTone90 \n");
//                   RotateHalfTone90(img_src,  img_rotated);
//                   printf("After RotateHalfTone90 \n");
//                   ptrData = img_rotated->bits;
//                   count = img_rotated->H * img_rotated->W;
//            }

            for(i = 0;  i < curPrnPara.nCopies ;  i++)
            {
                  if(!ExceptionHandler() )
                   {
                             PrnCmdRegVal |= PRINT_TASK_HALT;      set_reg(PRN_CMD_REG,  PrnCmdRegVal);   delete[] buf;      if(img_rotated)  delete img_rotated;       pthread_exit(0)  ;
                   }
                  while(Mem_Avaliable() <  count)  // 若FIFO空间不足一页，则等待FIFO.
                   {
                           usleep(US_TIME);    if(debug) printf("Waiting  FIFO\n ");   if(g_exit) {    delete[] buf;      if(img_rotated)  delete img_rotated;       pthread_exit(0) ;    }
                   }
                   if(debug) printf("before write_fifo\n");
                   //write_fifo (GPMC_FIFO_WRITE,  ptrData,  count);      _state.nPageCount ++;

				  {
				     //AllKindPages.printed++;   AllKindPages.total++;   WR_AllKindsofPages(AllKindPages);
				     //user.paper_nums =  _state.nPageCount;
//				     if(curSPPara.nMode==0&&!unMatch)//在安全模式且没有ID和权限错误下统计
//				     {
//						 if(user.paper_nums == 1) insert_user_entry(&mysql);
//						 else update_user_entry(&mysql, user.paper_nums);
//				     }
                  }  // 内部打印计数

                   if(  _state.nPageCount  > 0  || !existNext)
		           {
		                  usleep(US_TIME);         set_reg(PAGE_NUM_REG,   _state.nPageCount);
		                  if(!PrnStartCmd)
		                  {
		                        usleep(US_TIME);   PrnStartCmd = 1;    PrnCmdRegVal |=  PRINT_START;         set_reg(PRN_CMD_REG,  PrnCmdRegVal);     // 下达打印开始命令 //  bit  4 置 1
		                  }
		           }
		           usleep(US_TIME);   _state.printed_count = get_reg(PAGES_PRINTED_REG);  // 已打印份数
		           if(debug) printf("Printed Pages %d pages in fifo = %d\n", _state.printed_count,  _state.nPageCount);
           }
           if(img_rotated)  { if(debug) printf("before delete img_rotated\n"); delete img_rotated; img_rotated = 0;}
    }while(existNext );

     //write_fifo(GPMC_FIFO_WRITE,  buf,  256);    //再写入２５６字节的FIFO填充数据
     if(debug) printf("before delete buf\n");
     delete[] buf;
}

// 下发打印数据完成命令，检测打印结束状态
void  WaitThisPrintJobFinish()
{
        // 下发打印数据下传完成命令 bit2  置1
       PrnCmdRegVal |=  PRINT_DATA_SENT;      set_reg(PRN_CMD_REG, PrnCmdRegVal);   usleep(US_TIME);       if(debug)   printf("print finish PrnCmdRegVal =%02x\n",PrnCmdRegVal);
       unsigned short status  = get_reg(STATUS_PAPERSZ_REG);
       int lastcount = 0;
       while(!( status  & PRINT_END))  //  bit 14 为1  表示打印结束
       {
               _state.printed_count = get_reg(PAGES_PRINTED_REG);

				if(lastcount != _state.printed_count)
				{
					printf("IN,%0.4f\n",Concentration/4096.0*3.3);
					//获取预测浓度位置
					Concentration=get_reg(0x0041);
					//
					char t7[32] = { 0 }; snprintf(t7, 32, "t6.txt=\"%0.4f\"", Concentration/4096.0*3.3);
					ComWriteCmd(t7);
					lastcount = _state.printed_count;
					printf("lastcount = %d\n",lastcount);
				}
                if(debug) printf("PRINT_DATA_SENT FINISH,  Now Printed Pages %d\n", _state.printed_count);
			   drawPageChange();//界面显示当前已经打印页数


              if( !ExceptionHandler()  ||  g_exit) return ;
              usleep(500000);//这个延时不能太长，否则硬件已经结束打印，软件还没检测到完成信号而开始打印信号bit5还在置高，会重复打印
              status  = get_reg(STATUS_PAPERSZ_REG);
       }
       _state.printed_count = 0;//已打印页数清0
       WR_AllKindsofPages(AllKindPages);//更新打印页数
        // 作业结束了， CmdReg复位
		PrnCmdRegVal  =  0x6000;     set_reg(PRN_CMD_REG, PrnCmdRegVal);         curStage  = READY;
}

// 打印流程
void* TPrint(void *data)
{
    if(debug) printf("TPrint Thread In\n");

    prn_repprint_fd =  open("/media/repeat_print", O_RDWR|O_CREAT);
	lseek(prn_repprint_fd,0L,SEEK_SET);

    PRN_SCAN_PARA *psp = (PRN_SCAN_PARA*)data;
    if(debug) printf("----repeat_print_sign  = %d------\n");
    if(repeat_print_sign !=1)
    {
		curStage = PRINTING;
		UpdateUI = 1;
    }
    IniUsbBuffer(&_bufUsb);
    // 读主机打印数据
	int ret = ReadJobCfg(&curPrnPara);   // 0 is success
	if(debug) PrintJobSetting(curPrnPara);
	if(ret)   //若读取job未成功
	{
        if(fcntl(psp->fd, F_SETFL, O_NONBLOCK) == -1)
        {
            perror("fcntl");        close(psp->fd);     return 0;
        }
        char buff[100];       while (read(psp->fd, buff, 100) > 0 );        close(psp->fd);		return 0;
	}

	int unMatch = 0;
	//暂时注释掉
	if(((curSPPara.nUserID !=  psp->uid)&&(curSPPara.nMode==0))||!per.print_per) unMatch = 1;
	if(repeat_print_sign == 1) unMatch = 0;


 	if(unMatch==0)//ID匹配且拥有打印权限
 	{
		//  硬件复位
		HwReset(); 	         if(debug) printf("HWReset Done, Next WaitHwInitOK\n");
		// 等待硬件复位完毕，并且等硬件状态就绪
		WaitHwInitOK();      if(debug) printf("WaitHwInitOK Done, Next SetJobSettings\n");
		//  设置打印参数
		SetJobSettings();    if(debug) printf("SetJobSettings Done, Next DoPrint\n");
		// 打印数据下传，页寄存器修改, 开始打印指令下达
		DoPrint();
		if(debug) printf("DoPrint Done, Next WaitThisPrintJobFinish\n");
 	}else{
		while((curSPPara.nUserID !=  psp->uid)&&(curSPPara.nMode==0))//id不匹配界面提示，直到点击取消退出
		{
		//权限和ID检测必须放在上位机数据全部发送完毕之后，因为对于usb打印来说，如果数据没有发送完会造成poll函数循环监听到，无法连续打印
			if(debug) printf("UserId Not Match, finger_id=%d user=%d\n", curSPPara.nUserID,  psp->uid);
			drawIDnotMatch();
			if(g_exit){
				  //  硬件复位
				  HwReset(); 	       if(debug) printf("HWReset Done, Next WaitHwInitOK\n");
				  // 等待硬件复位完毕，并且等硬件状态就绪
				  WaitHwInitOK();      if(debug) printf("WaitHwInitOK Done, Next SetJobSettings\n");
				  //  设置打印参数
				  //SetJobSettings();    if(debug) printf("SetJobSettings Done, Next DoPrint\n");
				  SetJobSettingsNotAction();
				  //为了usb打印，只接受上位机数据，而不向fifo写，单纯走完流程
				  DoNotPrint();
				  PrnCmdRegVal |= PRINT_TASK_HALT;
				  set_reg(PRN_CMD_REG, PrnCmdRegVal);
				  if(!psp->isUsbfd) { close(psp->fd);}
				  return 0;
			 }
			sleep(1);
		}

		while(!per.print_per&&(curSPPara.nMode==0))//没有打印权限界面提示，直到点击取消退出
		{
			drawIDnotHavePrintPermission();
			if(g_exit){
				  //  硬件复位
				  HwReset(); 	       if(debug) printf("HWReset Done, Next WaitHwInitOK\n");
				  // 等待硬件复位完毕，并且等硬件状态就绪
				  WaitHwInitOK();      if(debug) printf("WaitHwInitOK Done, Next SetJobSettings\n");
				  //  设置打印参数
				  //SetJobSettings();    if(debug) printf("SetJobSettings Done, Next DoPrint\n");
				   SetJobSettingsNotAction();
				  //为了usb打印，只接受上位机数据，而不向fifo写，单纯走完流程
				  DoNotPrint();
				  PrnCmdRegVal |= PRINT_TASK_HALT;
				  set_reg(PRN_CMD_REG, PrnCmdRegVal);
				  if(!psp->isUsbfd) { close(psp->fd);}
				  return 0;
			 }
			sleep(1);
		 }
 	}
	if(!psp->isUsbfd) close(psp->fd);
 	// 等待打印结束
 	WaitThisPrintJobFinish();    if(debug) printf("PrintJobFinish Done\n");

	 {//目的是防止最后一页缺纸时，状态检测太快，使得补纸后状态又恢复到打印
		  sleep(1);
		  while(curStage==WARNING&&oldStage==PRINTING){
			sleep(1);
		  }
	 }
	if(repeat_print_sign ==1)
	{
		pthread_join(tid_rep_prn, 0);
		tid_rep_prn = 0;    curStage = READY;      UpdateUI = 1;            //  终止线程时可判断该值。
	}
	else
	{
		tid_prn = 0 ;   curStage = READY;      UpdateUI = 1;            //  终止线程时可判断该值。
	}
	OnRESET();
	drawReadyToSP();
	return 0 ;
}

void  InitPrinter()
{
       HwReset();
       if(debug) printf("HWPrinter_Reset OK, Next WaitHwInitOK\n");
       WaitHwInitOK();
}


