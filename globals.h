#ifndef  __GLOBALS_H__
#define __GLOBALS_H__
#include "ScanPrintTypes.h"
#include "Finger.h"
#include "LcdUI.h"
#include "GPMC.h"
#include "os_def.h"
#include "time.h"
#include "image.h"
#include "SQLite3.h"
// MOVE_CTRL_REG
#define CIS_CLOCK_SET  0x0080
#define USB_PRINTER_FILE			  "/dev/g_printer"
#define FPGA_DEV                           "/dev/fpga"

extern int debug;
extern unsigned short Concentration;
extern BUF_USB _bufUsb;
extern RUNNING_STATE   _state;
extern PrnPara curPrnPara;
extern int _gpmc,  prn_scan_fd, prn_repprint_fd , prn_repscanprint_fd ,prn_repscan_fd , number ;
extern volatile   int    g_exit;
extern volatile   int    isUsbDump;                        // USB中断
extern ScanPrnPara  curSPPara;
extern short  prn_scan_Port;
extern int _newsockfd;
extern pthread_t    tid_sp;
extern pthread_t 	tid_prn;
extern pthread_t 	tid_rep_prn;
extern pthread_t 	tid_rep_sp;
extern pthread_t    tid_rep_scan;
extern u8  hand_ack[8];
extern volatile int  avaliableFrames ;                            // 多合一复印(或身份证复印)的已扫描页数
extern volatile int  g_OKContinueScanPressed ;            // 按键OK继续扫描
extern volatile int  g_RunPressed;
extern volatile STAGE curStage , lastStage;                     // 当前状态, 区分工作流
extern volatile KEYS   curKeyPress;                 // 当前按键,  上次按键
extern  serial_arg        seri_fd4,   seri_fd5;         //  绘制界面用的串口, 指纹串口
extern unsigned short  PrnCmdRegVal ,   PModeRegVal ;
extern volatile int UpdateUI;
extern volatile FingerOP  op ;
extern unsigned short  G_LED_Val ;
extern myRect   curScanRegion;
extern volatile int  curYPos;
extern unsigned short MoveCtrlVal;
extern  int  Y_ORI;
extern STAGE   oldStage ;
extern unsigned short  ScanCmdRegVal ;
extern int   nHandPaperSz;
extern int   nBoxPaperSz;
extern char szHandPaper[32];
extern char szBoxPaper[32];
extern SysProfile  SysData ;
extern AllKindsofPages  AllKindPages;
extern SetDateofWN SetofWN;
extern short  Scan4head_Gain[4] , Scan4head_Offset[4] , Scan3rgb_Exposure[3];   // 4 头增益，４头偏移，rgb 三线阵曝光
extern User_info user;
extern Admin_info admin;
extern Permission_info per;
extern int ScanImageNeedRotate;
extern int repeat_print_sign , repeat_scanprint_sign , repeat_scan_sign;
void RD_SysProfile(SysProfile &sp);
void WR_SysProfile(SysProfile &sp);
void RD_AllKindsofPages(AllKindsofPages &akps);
void WR_AllKindsofPages(AllKindsofPages &akps);
void UpdatePageSize(PageSize &sizedata);
void ReadPageSize(int &pageH , int &pageW , int &ncolormode ,int cate);
void WR_WN(SetDateofWN &wn);
void RD_WN(SetDateofWN &wn);
void WR_TEM(SetDateofWN &wn);
void RD_GAIN(SetDateofWN& wn);
void WR_crcWN(SetDateofWN &wn);
void RD_crcWN(SetDateofWN &wn);
void WR_MOSHI(TwoSP &spp);
void RD_MOSHI(TwoSP &spp);
void WR_TINT(int tint);
int RD_TINT();
char* PaperName(int drvPaperSz);
void SetScan4HeadOffsets();
void SetScan4HeadGains();
void setMaxGains(int colormode);
void setMaxExposures(int colormode);
void SetScan3rgbExposures();
int ComWriteCmd(char *cmd);
int checkPaperBox1Paper();
int handpaperfeeder_avaliable();   // 手动纸盘是否有纸张
int HandPaperMatch(int nPaperSz);  // 比较手动纸张是否和给定的值相匹配。
int PrnDataNeedRotate(int BoxPaperSz);   // 根据当前的打印纸盒纸张，判断是否要选装打印数据.
void GetPageSize(int &pageH, int &pageW );  // 根据纸张获得尺寸
void CalScale_Page_to_Scan();  // 根据纸张和扫描稿件，计算缩放比例
void PS7PressDown();//长舌压下确定纸张大小
void PS7PressUP();// 长舌弹起来时刻
//　移动到某处
void MoveTo(int YPos);
#endif
