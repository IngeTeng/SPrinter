
#ifndef _MYFUNCTIONS_
#define _MYFUNCTIONS_

#include "ScanPrintTypes.h"
#include "Finger.h"

typedef  void (*operate)(void);
// CSimLCDDlg 对话框
// Keys Defination： 0混合原稿 1单面双面 2排纸处理 3合并原稿 4身份证复印 5效用 6确认 7模式记忆 8纸张 9缩放 10质量 11浓度 12功能 18返回 19打印 20访问  21节能 22复位  23清除/停止 24开始 25ABC/123
enum KEYS {  HHYG = 0x125, DMSM = 0x113, PZCL = 0x112,  HBYG = 0x123, SFZFY = 0x122, XYONG = 0x153, QREN = 0x152, MSJY = 0x114, KPAPER = 0x131, KSCALE = 0x132, KQUALITY = 0x133, KTINT = 0x134, KFUNC = 0x124, UP = 0x145, DOWN = 0x142, LEFT = 0x144, RIGHT = 0x143, OK = 0x141,
                        RETURN = 0x135,    KPRINT = 0x151, ACCESS = 0x154, ESAVE = 0x115, RESET = 0x175, CLEARSTOP = 0x185, RUN = 0x184,    FLIP = 0x183,  NUM0 = 0x162, NUM1 = 0x182, NUM2 = 0x181, NUM3 = 0x174, NUM4 = 0x173, NUM5 = 0x172, NUM6 = 0x171, NUM7 = 0x165, NUM8 = 0x164,
                        NUM9 = 0x163,   STAR = 0x161,     CROSS = 0x155 ,  vKeyREADY = 0x01 ,   vKeyNULL = 0x00,  vKeyFINGER = 0x02};
class Pair
{
      public:
      KEYS   key;
      char   val;
      Pair(KEYS  K,  char v) { key = K;   val = v; }
};

typedef struct message_entry
{
    KEYS key;
    operate func;
}message_entry;

#define size_message_map       ( sizeof(message_map) / sizeof(message_entry))

// Stage Defination -3加电 -2等待启动  -1热机 0就绪 1浓度 2纸张 3纸盒 4尺寸 5类型 6缩放 7自动缩放 8固定比率 9手动缩放 10原稿质量 11功能
// 12消边 13消边距设定 14文件边距 15装订线边距 16XY不等缩放 17 XY缩放系数设置
// 18反文 19书本分离 20 印记 21页面格式 22扫描中  23打印中  24复印中 25 排纸处理 26 合并原稿  27 原稿方向 28 四合一  29 身份证 1000忙  999警告
enum STAGE {LOAD = -3, WAIT = -2,  HEAT = -1, READY = 0, TINT = 1, PAPER = 2, PAPERBOX = 3, PAPERSZ = 4, PAPERTYPE = 5, SCALE = 6, SCALEAUTO = 7, SCALEFIX = 8, SCALEMANUAL = 9,  QUALITY = 10,
	        FUNC = 11, ERASE = 12, ERASESETMM = 13, PAGEMARGIN = 14, MARGINSETMM = 15, XYSCALE = 16, XYSCALESET = 17, NEGTIVE = 18, BOOKSEP =19, STAMP = 20,   PAGEFORMAT = 21,
			SCANNING = 22, PRINTING = 23, SING_PING = 24, SPZCL = 25, SHBYG = 26, SYGFX = 27, S4To1 = 28, SSFZFY = 29,  SLEEP = 30,  NEXTSCAN = 31, HANDPAPER = 32, FINGERLOGIN = 33,  PASSWDLOGIN = 34,
			ADMIN = 35,  CHANGEPWD = 36,  ADDUSER = 37,  DELUSER = 38, USERMANAGE = 39, VERIFYALLDEL = 40, RUNON = 41, XIAOYONG = 42,  SMSJY = 43, SQREN = 44,  PANELRESETTIME = 45,   SLEEPTIMESET = 46,  BKPAPERSET = 47,
			REPEAT= 48,REPEAT_PRINT = 49,REPEAT_SCAN=50, REPEAT_SCANPRINT = 51,
			IDLE = 888,   SBUSY = 1000, WARNING = 999, SETWN = 666, MODE = 777, FIX = 331, GAIN = 333, TCRWAIT = 334, TCROK = 335, MYSQLERROR = 336,TEM = 337};

// 工作状态小图标： 18空白， 16分页， 20身份证， 24：2合一，25：4合一，23消边， 22装订， 19反转， 26印记, 21书本分离
enum UI_LCD_Status_PicId{P_NULL = 18, P_FENYE = 16, P_SFZ = 20, P_2TO1 = 24, P_4TO1 = 25, P_ERASEBORDER = 23, P_PAGEMARGIN = 22, P_BOOKSEP = 21, P_NEGTIVE = 19, P_STAMP = 26};

typedef struct stage_entry
{
	STAGE stage;
	operate draw;
}stage_entry;

#define size_stage_map       ( sizeof(stage_map) / sizeof(stage_entry))

typedef struct  paper_info
{
    int paperSz;
	char* paperName;
}paper_info;

typedef struct Item
{
	int idx;          //列表中第idx项
	char* str;        //该选择项的文本信息。
	int theValue;     //特别的数值意义：如 纸张A4 对应 9
}Item;


// 对于那些有下拉列表选项的界面，用ItemList记录其下拉列表信息。
typedef struct  ItemList
{
	STAGE stage;  //具体哪个界面
	int cnt;      //共有几项选择条目
	int cur_sel;  //当前选中项
	Item* lst;    //列表项
	int last_sel; //上次选择项
}ItemList;

// 数值控件
typedef struct Edit
{
	STAGE stage;   // 窗口状态
	char  text[8]; // 字符输入
	int   cur_idx; // 当前位置字符
	int   maxL;    // 最大字符长度
	int   minV;    // 最小有效值
	int   maxV;    // 最大有效值
}Edit;

typedef struct FingerOP
{
        STAGE   cmd;    // add 0,  del 1, match 2,  delall 3,  ucnt  4
        u16       uid;     // uid
        int         ret;     // 返回值.
        int         pushOK;
}FingerOP;

//主消息循环
void*  Lcd_display(void*);
void*  KeysListen(void *);
void*  ScanFinger(void*);
void*  LoginLock(void*);

// draw stages;
void drawLOAD();
void drawWAIT();
void drawHEAT();
void drawREADY();
void drawTINT();
void drawPAPER();
void drawPAPERBOX();
void drawPAPERSZ();
void drawPAPERTYPE();
void drawSCALE();
void drawSCALEAUTO();
void drawSCALEFIX();
void drawSCALEMANUAL();
void drawQUALITY();
void drawFUNC();
void drawERASE();
void drawERASESETMM();
void drawPAGEMARGIN();
void drawMARGINSETMM();
void drawXYSCALE();
void drawXYSCALESET();
void drawNEGTIVE();
void drawBOOKSEP();
void drawSTAMP();
void drawPAGEFORMAT();
void drawSCANNING();
void drawREPEAT_SCANING();
void drawPRINTING();
void drawREPEAT_PRINTING();
void drawSING_PING();
void drawREPEAT_SCANPRINTING();
void drawIDnotMatch();
void drawTonerWillEmpty();
void drawMainFlash();
void drawTonerRecover();
void drawAAddToner();
void drawBAddToner();
void drawPZCL();
void drawHBYG();
void draw4To1();
void drawYGFX();  // 原稿方向
void drawBUSY();
void drawWARNING();
void drawNEXTSCAN();
void drawHANDPAPER();
void drawFINGERLOGIN();    // 指纹登陆
void drawPASSWDLOGIN();   // 管理员密密码登陆
void drawADMIN();                 // 管理员界面
void drawCHANGEPWD();
void drawADDUSER();
void drawDELUSER();
void drawUSERMANAGE();
void drawVERIFYALLDEL();
void drawSQREN();  // 确认
void drawSXIAOYONG(); // 效用
void drawSMSJY();  // 模式记忆
void drawPANEL_RESET_TIME_SET();
void drawSLEEP_TIME_SET();
void drawBKPAPER_SET();
void drawSETWN();
void drawMODE();
void drawFIX();
void drawGAIN();
void drawTEM();
void drawMYSQLERROR();
void drawTCROK();
void drawTCRWAIT();
void drawIDnotHavePrintPermission();
void drawIDnotHaveScanPermission();
void drawComAndPageRun();
void drawMemUsed();
void drawREPEAT();//显示重复打印的选择界面
// press keys function;
void OnvKeyREADY();    // just jump to Ready UI;
void OnvKeyFINGER(); // jump to FINGER UI;
void OnHHYG();
void OnDMSM();
void OnPZCL();
void OnHBYG();
void OnSFZFY();
void OnXYONG();
void OnQREN();
void OnMSJY();
void OnPAPER();
void OnSCALE();
void OnQUALITY();
void OnTINT();
void OnFUNC();
void OnUP();
void OnDOWN();
void OnLEFT();
void OnRIGHT();
void OnOK();
void OnRETURN();
void OnPRINT();
void OnACCESS();
void OnESAVE();
void OnRESET();
void OnCLEARSTOP();
void OnRUN();
void OnFLIP();
void OnNUM();
void OnSTAR();
void OnCROSS();
void DoUIOperationCheck();
void drawScanPaperNotMatch();
void drawReadyToSP();
void TCR_ADJUST();
void drawPageChange();
#endif
