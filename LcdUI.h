
#ifndef _MYFUNCTIONS_
#define _MYFUNCTIONS_

#include "ScanPrintTypes.h"
#include "Finger.h"

typedef  void (*operate)(void);
// CSimLCDDlg �Ի���
// Keys Defination�� 0���ԭ�� 1����˫�� 2��ֽ���� 3�ϲ�ԭ�� 4���֤��ӡ 5Ч�� 6ȷ�� 7ģʽ���� 8ֽ�� 9���� 10���� 11Ũ�� 12���� 18���� 19��ӡ 20����  21���� 22��λ  23���/ֹͣ 24��ʼ 25ABC/123
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

// Stage Defination -3�ӵ� -2�ȴ�����  -1�Ȼ� 0���� 1Ũ�� 2ֽ�� 3ֽ�� 4�ߴ� 5���� 6���� 7�Զ����� 8�̶����� 9�ֶ����� 10ԭ������ 11����
// 12���� 13���߾��趨 14�ļ��߾� 15װ���߱߾� 16XY�������� 17 XY����ϵ������
// 18���� 19�鱾���� 20 ӡ�� 21ҳ���ʽ 22ɨ����  23��ӡ��  24��ӡ�� 25 ��ֽ���� 26 �ϲ�ԭ��  27 ԭ�巽�� 28 �ĺ�һ  29 ���֤ 1000æ  999����
enum STAGE {LOAD = -3, WAIT = -2,  HEAT = -1, READY = 0, TINT = 1, PAPER = 2, PAPERBOX = 3, PAPERSZ = 4, PAPERTYPE = 5, SCALE = 6, SCALEAUTO = 7, SCALEFIX = 8, SCALEMANUAL = 9,  QUALITY = 10,
	        FUNC = 11, ERASE = 12, ERASESETMM = 13, PAGEMARGIN = 14, MARGINSETMM = 15, XYSCALE = 16, XYSCALESET = 17, NEGTIVE = 18, BOOKSEP =19, STAMP = 20,   PAGEFORMAT = 21,
			SCANNING = 22, PRINTING = 23, SING_PING = 24, SPZCL = 25, SHBYG = 26, SYGFX = 27, S4To1 = 28, SSFZFY = 29,  SLEEP = 30,  NEXTSCAN = 31, HANDPAPER = 32, FINGERLOGIN = 33,  PASSWDLOGIN = 34,
			ADMIN = 35,  CHANGEPWD = 36,  ADDUSER = 37,  DELUSER = 38, USERMANAGE = 39, VERIFYALLDEL = 40, RUNON = 41, XIAOYONG = 42,  SMSJY = 43, SQREN = 44,  PANELRESETTIME = 45,   SLEEPTIMESET = 46,  BKPAPERSET = 47,
			REPEAT= 48,REPEAT_PRINT = 49,REPEAT_SCAN=50, REPEAT_SCANPRINT = 51,
			IDLE = 888,   SBUSY = 1000, WARNING = 999, SETWN = 666, MODE = 777, FIX = 331, GAIN = 333, TCRWAIT = 334, TCROK = 335, MYSQLERROR = 336,TEM = 337};

// ����״̬Сͼ�꣺ 18�հף� 16��ҳ�� 20���֤�� 24��2��һ��25��4��һ��23���ߣ� 22װ���� 19��ת�� 26ӡ��, 21�鱾����
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
	int idx;          //�б��е�idx��
	char* str;        //��ѡ������ı���Ϣ��
	int theValue;     //�ر����ֵ���壺�� ֽ��A4 ��Ӧ 9
}Item;


// ������Щ�������б�ѡ��Ľ��棬��ItemList��¼�������б���Ϣ��
typedef struct  ItemList
{
	STAGE stage;  //�����ĸ�����
	int cnt;      //���м���ѡ����Ŀ
	int cur_sel;  //��ǰѡ����
	Item* lst;    //�б���
	int last_sel; //�ϴ�ѡ����
}ItemList;

// ��ֵ�ؼ�
typedef struct Edit
{
	STAGE stage;   // ����״̬
	char  text[8]; // �ַ�����
	int   cur_idx; // ��ǰλ���ַ�
	int   maxL;    // ����ַ�����
	int   minV;    // ��С��Чֵ
	int   maxV;    // �����Чֵ
}Edit;

typedef struct FingerOP
{
        STAGE   cmd;    // add 0,  del 1, match 2,  delall 3,  ucnt  4
        u16       uid;     // uid
        int         ret;     // ����ֵ.
        int         pushOK;
}FingerOP;

//����Ϣѭ��
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
void drawYGFX();  // ԭ�巽��
void drawBUSY();
void drawWARNING();
void drawNEXTSCAN();
void drawHANDPAPER();
void drawFINGERLOGIN();    // ָ�Ƶ�½
void drawPASSWDLOGIN();   // ����Ա�������½
void drawADMIN();                 // ����Ա����
void drawCHANGEPWD();
void drawADDUSER();
void drawDELUSER();
void drawUSERMANAGE();
void drawVERIFYALLDEL();
void drawSQREN();  // ȷ��
void drawSXIAOYONG(); // Ч��
void drawSMSJY();  // ģʽ����
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
void drawREPEAT();//��ʾ�ظ���ӡ��ѡ�����
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
