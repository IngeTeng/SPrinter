#include "LcdUI.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "Serial.h"
#include "os_def.h"
#include "ScanPrint.h"
#include "pthread.h"
#include "GPMC.h"
#include "Finger.h"
#include "globals.h"
#include "PrintJob.h"
#include "SQLite3.h"
#include "BufUsb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//ȫ�ֱ�������ΪLED�ƿ���Ҳ��Ҫ�üĴ���bit13 �ã�
const int LstItemsPerScreen = 4; // ���������ʾ4��ѡ��� ���ϱ��⹲5��
const int nTintMax = 9;             // ���Ũ�ȼ���Ϊ9��
const int nTintMin = 0;             // ��СŨ�ȼ���Ϊ0��
const int nTintLevels = nTintMax - nTintMin + 1;

volatile KEYS curKeyPress =  HHYG;       // ��ǰ����
volatile STAGE curStage = FINGERLOGIN;        // ��ǰ״̬
volatile STAGE lastStage = LOAD;          // ��һ��״̬
volatile int   lastselidx = -1;                      // �ϴ�ѡ��
volatile int  curLRIdx = nTintMax /2;   // Ũ�����һ�������
volatile int  var_x_y = 0;              // ͬһ��������xy�������ݣ�0 ѡ��x, 1 y.
PrnPara curPrnPara = {0};           // ��ǰ��ӡ����
int   bTScanMov = 0;
int   nHandPaperSz = 0x37;        // A3
int   nBoxPaperSz   = 0x37;         // A3
char szHandPaper[32]  = {"sA3"};
char szBoxPaper[32] = {" "};
SysProfile  SysData = {0,  300,  60};
AllKindsofPages  AllKindPages = {0};  // ��������
SetDateofWN SetofWN = {0};
const ScanPrnPara DefSP =//Ĭ�ϲ���ֵ
{
            0,                 // nWidth ͼ�����ؿ��
            0,                 // nHeight
            1,                 // nPaperBox
            0,                 // nPaperType
            "��ֽͨ",          //  szPaperType
            0,                 // nPaperSize
            " ",               // szPaper
            1,                 //  nCopy
            100,               //  nScaleX
            100,               //  nScaleY
            1,                 //  bAutoScale��Ĭ���Զ�����,�̶�100%���ʲ�����ֽ��ֽ��Ϊ׼��ӡ
            0,                 // nQuality
            "�ı�/��Ƭ",        //szQuality
            4,                  // nTint
            1,                  // bAutoTint
            0,                  // nEraseBorder
            10,                // nErasemm
            0,                  // nPageMargin
            10,
            0,                  // nNegtive
            0,                  // nStamp
            0,                  // nStampMode
            0,                  // bISSFZ
            0,                 // group_pages
            0,                 // nCombineMode
            0,                 // n4To1
            0,                 // nBookSep
            1,                 // nSingleDouble
            1,                 // bAutoPaper
            0,                 //  UserID
            0 				   //  Mode
};
Pair   pairs[10] = {  {NUM0,  '0' } , {NUM1,  '1' } , {NUM2,  '2' } , {NUM3,  '3' } , {NUM4,  '4' } , {NUM5,  '5' } , {NUM6,  '6' } , {NUM7,  '7' } , {NUM8,  '8' } , {NUM9,  '9' }  };

ScanPrnPara curSPPara =  DefSP;     // ��ǰ��ӡ����
char szBottomInfo[64] = { 0 }, szReadyT4[64] = {0};//�ײ���Ϣ��������t4��Ϣ

// Key-OnKey
message_entry message_map[] =
{
 //   {vKeyFINGER,OnvKeyFINGER},
 //   {vKeyREADY, OnvKeyREADY},
	{HHYG, OnHHYG},			//���ԭ��
	{DMSM, OnDMSM},			//����˫��
	{PZCL, OnPZCL},			//��ֽ����
	{HBYG, OnHBYG},			//�ϲ�ԭ��
	{SFZFY, OnSFZFY},		//���֤��ӡ
	{XYONG,OnXYONG},		//Ч��
	{QREN,OnQREN},			//ȷ��
	{MSJY,OnMSJY},			//ģʽ����
	{KPAPER,OnPAPER},       //ֽ��
	{KSCALE,OnSCALE},       //����
	{KQUALITY,OnQUALITY},	//����
	{KTINT,OnTINT},			//Ũ��
	{KFUNC,OnFUNC},			//����
	{UP,OnUP},				//��
	{DOWN, OnDOWN},			//��
	{LEFT,OnLEFT},          //��
	{RIGHT,OnRIGHT},        //��
	{OK,OnOK},              //OK
	{RETURN,OnRETURN},      //����
	{KPRINT,OnPRINT},        //��ӡ
	{ACCESS,OnACCESS},		//����
	{ESAVE,OnESAVE},		//����
	{RESET,OnRESET},		//��λ
	{CLEARSTOP,OnCLEARSTOP},//���/ֹͣ
	{RUN,OnRUN},            //��ʼ
	{FLIP,OnFLIP},          //ABC...<->123...
	{NUM0,OnNUM},           //0
	{NUM1,OnNUM},           //1
	{NUM2,OnNUM},           //2
	{NUM3,OnNUM},           //3
	{NUM4,OnNUM},			//4
	{NUM5,OnNUM},			//5
	{NUM6,OnNUM},			//6
	{NUM7,OnNUM},			//7
	{NUM8,OnNUM},			//8
	{NUM9,OnNUM},           //9
	{STAR,OnSTAR},          //*
	{CROSS,OnCROSS} ,        //#
};

//Stage-OnStage
stage_entry stage_map[] =
{
	{LOAD,drawLOAD},                    //ϵͳ�ӵ�����
	{WAIT,drawWAIT},                    //�ȴ�
	{HEAT,drawHEAT},                    //�Ȼ�
	{READY,drawREADY},                  //����
	{TINT,drawTINT},                    //����Ũ��
	{PAPER,drawPAPER},                  //����ֽ��
	{PAPERBOX,drawPAPERBOX},            //����ֽ����Դ��λ�ã�
	{PAPERSZ,drawPAPERSZ},              //����ֽ�ųߴ�
	{PAPERTYPE,drawPAPERTYPE},          //����ֽ������
	{SCALE,drawSCALE},                  //��������ϵ��
	{SCALEAUTO,drawSCALEAUTO},          //�Զ�����
	{SCALEFIX,drawSCALEFIX},            //�̶����ű���
	{SCALEMANUAL,drawSCALEMANUAL},      //�ֶ����ű���
	{QUALITY,drawQUALITY},              //����ԭ������
	{FUNC,drawFUNC},                    //��������
	{ERASE,drawERASE},                  //����
	{ERASESETMM,drawERASESETMM},        //����ֵ�趨
	{PAGEMARGIN,drawPAGEMARGIN},        //�ļ��߾�
	{MARGINSETMM,drawMARGINSETMM},      //װ���߱߾�ֵ�趨
	{XYSCALE,drawXYSCALE},              //XY ��������
	{XYSCALESET,drawXYSCALESET},        //XY ����ϵ��
	{NEGTIVE,drawNEGTIVE},              //�ڰ׷�ת
	{BOOKSEP,drawBOOKSEP},              //�鱾����
	{STAMP,drawSTAMP},                  //ӡ��
	{PAGEFORMAT,drawPAGEFORMAT},        //ҳ���ʽ
	{SCANNING,drawSCANNING},            //ɨ����
	{REPEAT_SCAN,drawREPEAT_SCANING},		//�ظ�ɨ����
	{PRINTING,drawPRINTING},            //��ӡ��
	{REPEAT_PRINT,drawREPEAT_PRINTING},	//�ظ���ӡ��
	{SING_PING,drawSING_PING},          //��ӡ��
	{REPEAT_SCANPRINT,drawREPEAT_SCANPRINTING},	//�ظ���ӡ��
	{SPZCL, drawPZCL},                      //��ֽ����
	{SHBYG, drawHBYG},                  //�ϲ�ԭ��
	{SYGFX, drawYGFX},                  //ԭ�巽��
	{S4To1, draw4To1},                        //�ĺ�һ
	{SBUSY, drawBUSY},                     //æ
	{WARNING,drawWARNING},               //����
	{NEXTSCAN, drawNEXTSCAN},         // Hold On Next Scan
	{HANDPAPER, drawHANDPAPER} ,   // �ֶ�ֽ��ѡֽ
	{ PASSWDLOGIN, drawPASSWDLOGIN },
	{ FINGERLOGIN, drawFINGERLOGIN },
	{ ADMIN, drawADMIN },
	{ USERMANAGE, drawUSERMANAGE},
	{ CHANGEPWD, drawCHANGEPWD },
	{ ADDUSER, drawADDUSER },
	{ DELUSER, drawDELUSER },
	{ VERIFYALLDEL,drawVERIFYALLDEL},
	{ RUNON, OnRUN},
	{ XIAOYONG, drawSXIAOYONG},
	{ SQREN, drawSQREN},
	{ SMSJY, drawSMSJY },
	{ PANELRESETTIME,  drawPANEL_RESET_TIME_SET},
	{ BKPAPERSET,  drawBKPAPER_SET},
	{ SLEEPTIMESET, drawSLEEP_TIME_SET},
	{ SETWN, drawSETWN},
	{ MODE, drawMODE},
	{ FIX, drawFIX},
	{ GAIN, drawGAIN},
	{ TCRWAIT, drawTCRWAIT},
	{ TCROK, drawTCROK},
	{ MYSQLERROR, drawMYSQLERROR},
	{ TEM, drawTEM},
	{ REPEAT , drawREPEAT}      //�ظ���ӡ��ӡɨ���ѡ��
};

Item TintItem[]  ={{0,"�Զ�"}, {1,"�ֶ�"},0};
Item PaperItem[] ={{0,"�Զ�"}, {1, "ֽ��1��"}, {2, "�ֶ���"}, 0};
Item PaperBoxItem[] = {{0,"ֽ�ųߴ�"},{1,"ֽ������"},0};
Item PaperSZItem[] = { {0,"�Զ�",0x00}, {1,"A6==", 0x10},{2,"B4 ==", 0x1F},  {3,"B5==",0x10}, 0 };
Item PaperTypeItem[] = { {0,"��ֽͨ"},{1,"����ֽ"},{2,"����"},{3,"����ֽ"},0 };
Item ScaleItem[] ={{0,"�Զ�"},{1,"�̶�"},{2,"�ֶ�"},0};
Item ScaleFixItem[] = {{0,"25%", 25},{1,"50%",50},{2,"70%  A3->A4, B4->B5",70},{3,"81%  B4->A4",81},{4,"100%", 100},{5,"115%  B4->A3",115},{6,"141%  B5->B4, A4->A3", 141},{7,"200%",200},{8,"400%",400},0};
Item QualityItem[] ={{0,"�ı�/��Ƭ"},{1,"�ı�"},{2,"��Ƭ"},0};
Item FuncItem[] = {{0,"����",ERASE},{1,"�ļ��߾�",PAGEMARGIN},{2,"X/Y����",XYSCALE},{3,"��ת",NEGTIVE},{4,"�鱾����",BOOKSEP},{5,"ӡ��",STAMP}, {6,"ȷ�Ϸ���",READY},0};
Item EraseItem[] ={{0,"��"},{1,"��"},{2,"��"},{3,"�߿�"},0};
Item ScaleXYItem[] ={{0,"��"},{1,"��"},0};
Item PageMarginItem[] = {{0,"��"},{1,"��"},0};
Item NegtiveItem[] = {{0,"��"},{1,"��"},0};
Item BookSepItem[] = {{0,"��"},{1,"����"},{2,"��չ"},0};
Item StampItem[] = {{0,"��"},{1,"ҳ��"},0};
Item PageFormatItem[] = {{0,"P001, P002"},{1,"1, 2, 3"},0};
Item PZCLItem[] = { { 0, "����ҳ" }, {1,"��ҳ"}, 0 };
Item HBYGItem[] = { { 0, "��"}, {1, "2��1"}, {2, "4��1"}, 0 };
Item YGFXItem[] = { {0, "��"}, {1,"��"}, {2,"��"}, {3,"�ײ�"}, 0};
Item _4To1Item[] = { {0,"��ʽ1"}, {1,"��ʽ2"}, 0};
Item HandPaperItem[] = { {0,"sA3",0x37}, {1,"sA4 ==", 0x18}, {2,"sA4R ||",0x33},{3,"sB4 ==",0x1F}, {4, "sB5 ==", 0x10}, {5, "sB5R ||", 0x16}, {6, "s8K==", 0x1F},{7, "s16K==", 0x10}, 0 };
Item AdminItem[] = { { 0, "�û�����", USERMANAGE}, {1, "��ӡ����", READY} ,{2, "ģʽѡ��", MODE}};
Item ModeItem[] = {{0,"��ͨ"},{1,"��ȫ"},0};
Item UserManageItem[] = { {0, "����Ա������", CHANGEPWD}, {1,"ע���û�", ADDUSER}, {2,"ɾ���û�", DELUSER}, {3,"��������û�", VERIFYALLDEL},{4 , "��ӡ����������",REPEAT} };
Item VerifyAllDelItem[] = { {0, "ȡ��"}, {1,"ȷ�����"} };
Item DeleteUserItem[] = { {0, "���û�ID,OKɾ��"}, {1,"ɨ��ָ��OK��ɾ��"} };
Item SleepTimeSetItem[]={{0, "1����",  60}, {1, "3����",  180}, {2, "5����",  300}, {3, "30����",  1800}, {4, "4Сʱ",  14400},  0 };
Item PanelResetItem[] = {{0, "��",  0}, {1, "30��",  30}, {2, "1����",  60}, {3, "3����",  180}, {4, "5����", 300},  0 };
Item BKPaperSetItem[] ={{0, "K��ֽ"}, {1, "B��ֽ"}, 0 };
Item XYongItem[] ={ {0,"�Զ���帴λ", PANELRESETTIME}, {1, "����ģʽ", SLEEPTIMESET}, {2,"BKֽ����", BKPAPERSET}, {3,"ά��ģʽ", FIX}, 0};
Item MsjyItem[] = {{0, "����1"}, {1,"����"}, {2, "����"}, {3, "����"}, 0};
Item FIXItem[] ={ {0,"TCR����", TCRWAIT}, {1, "��������", GAIN}, {2, "�¶�Ũ��", SETWN}, {3, "�¶ȵ���", TEM},0};
Item TEMItem[] = {{0, "-3",-150}, {1,"-2",-100}, {2, "-1",-50}, {3, "0",0}, {4, "1",50}, {5, "2",100},{6, "3",150}, 0};//�¶�7��
Item REPEATItem[] = {{0, "��һ�δ�ӡ�ļ�"},{1 , "��һ��ɨ���ļ�"}, {2 , "��һ�θ�ӡ�ļ�"} , 0};
//�������б�ѡ��Ľ��棬��ItemList��¼�������б���Ϣ�� // WhichStage, nItemCnts, curSelidx, WhoListIs
ItemList  WndList[] =
{
	{TINT,  2, 0, TintItem},
	{PAPER, 3, 0, PaperItem},
	{PAPERBOX, 2, 0, PaperBoxItem},
	{PAPERSZ, 4, 0,  PaperSZItem},
	{PAPERTYPE, 4, 0, PaperTypeItem},
	{SCALE, 3, 0, ScaleItem},
	{SCALEFIX,9, 0, ScaleFixItem},
	{QUALITY, 3, 0, QualityItem},
	{FUNC, 7, 0, FuncItem},
	{ERASE,4,0,EraseItem},
	{XYSCALE,2,0,ScaleXYItem},
	{PAGEMARGIN,2,0,PageMarginItem},
	{NEGTIVE,2,0,NegtiveItem},
	{BOOKSEP,3,0,BookSepItem},
	{STAMP,2,0, StampItem},
	{PAGEFORMAT,2,0, PageFormatItem},
	{SPZCL,2,0,PZCLItem},
	{SHBYG,3,0,HBYGItem},
	{SYGFX,4, 0, YGFXItem},
    {S4To1, 3, 0, _4To1Item},
    {HANDPAPER, 8, 0, HandPaperItem},
    { ADMIN, 3, 0, AdminItem},
	{ USERMANAGE, 5, 0, UserManageItem},
	{ VERIFYALLDEL, 2, 0, VerifyAllDelItem},
	{ DELUSER, 2, 0, DeleteUserItem},
	{SLEEPTIMESET, 5, 0, SleepTimeSetItem},
	{PANELRESETTIME, 5, 0, PanelResetItem},
	{BKPAPERSET, 2, 0, BKPaperSetItem},
	{XIAOYONG, 4, 0, XYongItem},
	{SMSJY, 4, 0, MsjyItem},
	{MODE, 2, 0, ModeItem},
	{FIX, 4, 0, FIXItem},
	{TEM, 7, 0, TEMItem},
	{REPEAT , 3 , 0 , REPEATItem}
};

enum { TINTIDX = 0,   PAPERIDX= 1,  PAPERBOXIDX = 2,   PAPERSZIDX = 3,   PAPERTYPEIDX = 4,   SCALEIDX = 5,
             SCALEFIXIDX = 6,      QUALITYIDX = 7,      FUNCIDX = 8,     ERASEIDX = 9,    XYSCALEIDX = 10,
             PAGEMARGINIDX = 11,   NEGTIVEIDX = 12,     BOOKSEPIDX = 13,   STAMPIDX = 14,    PAGEFORMATIDX = 15,
             PZCLIDX = 16,    HBYGIDX = 17,     YGFXIDX = 18,     S4To1IDX = 19,      HANDPAPERIDX = 20,
             ADMINIDX = 21, USERMANAGEIDX = 22, VERIFYALLDELIDX = 23, DELUSERIDX = 24, SLEEPTIMSETIDX = 25,
             PANELRSTIDX = 26,  BKPAPERIDX = 27,  XYONGIDX = 28, MSJYIDX = 29, MODEIDX = 30, FIXIDX = 31, TEMIDX = 32,REPEATIDX = 33
              };

Edit  Eready = {READY, "1", 0, 3, 1, 999};    //copy
Edit  Escale = {SCALEMANUAL, "95", 0, 3,  25, 400};
Edit  Exyscalesetx = {XYSCALESET,"100", 0, 3, 50,100};
Edit  Exyscalesety = {XYSCALESET,"100", 0, 3, 5, 200};
Edit  Emarginset  = {MARGINSETMM,"10", 0, 2, 0, 20};
Edit  Eeraseset   = {ERASESETMM, "10", 0, 2, 4, 20};

Edit  EPsswd = { PASSWDLOGIN, "", 0, 6, 0, 0};    //copy
Edit  EcurRealPsswd = { PASSWDLOGIN, "",  0, 6, 0, 0 };    //copy

Edit  EOldPsswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  EOldRealPsswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  ENew1Psswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  ERealNew1Psswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  ENew2Psswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  ERealNew2Psswd = { CHANGEPWD, "", 0, 6, 0, 0 };    //copy
Edit  EAddUserID = { ADDUSER, "1", 0, 4, 1, 4095 };    //copy,�û���������idΪ0
Edit  EDelUserID = { DELUSER, "0", 0, 4, 1, 4095 };    //copy
Edit  ETem1 = { SETWN, "0", 0, 4, 0, 4095 };    //�¶�1
Edit  ETem2 = { SETWN, "0", 0, 4, 0, 4095 };    //�¶�1
Edit  ETint = { SETWN, "0", 0, 4, 0, 4095 };    //Ũ��
Edit  EGain = { GAIN, "0", 0, 4, 0, 4095 };		//���� 90-190

volatile int  UpdateUI = 1;//ֻ��Ϊ1ʱ������Ż���ݵ�ǰ״̬ˢ��
volatile int  g_IsValidUser = 0;
volatile FingerOP  op = {FINGERLOGIN,  0, 0, 0};
volatile int scanfingerOK = 0;//ָ��ɾ��ʱ��ֹƵ������ɨ��ɨ�裬ֻҪɨ��ɹ����ID����1
volatile int isMatching = 0; //����ָ�Ʊȶ��źţ�ָ��ģ���һֱ�ȴ��ȶԴ�ʱ��1��ֻ���յ�����ֵ�Ż���0����ָֹ��ģ��һֱ�ȴ��ȶԶ��û����˷�����ɲ�ƥ��
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void* ScanFinger(void*  In)
{
        while(1)//���ڵ������ǣ��������ȶ�״̬��һֱ�ȴ�ָ��
        {
                FingerOP *OP= (FingerOP*) In;
                //if(debug) printf("ScanFinger  curOPStage:%d, curopStage:%d, ok: %d\n", OP->cmd,op.cmd,OP->pushOK);
                int cur_Usrs = 0;
                u16 uid = 0;
                //sleep(3);//����ʱ������˸
                switch(OP->cmd)
                {
                        case  ADDUSER :
                                    cur_Usrs = GetUsersCnt();   sprintf(szBottomInfo, "�û�������%d", cur_Usrs);
                                    OP->uid = atoi(EAddUserID.text);
                                    if(OP->pushOK)
                                    {
                                             if(AddUser(OP->uid, 1) == ACK_SUCCESS)
                                             {
                                                sprintf(szBottomInfo,  "%s",  "��ӳɹ�");
                                                admin.admin_id = curSPPara.nUserID;
												insert_admin_entry( 0, OP->uid);
												Add_user(OP->uid,'0','0','0');
                                             }
                                             else { ParseInfo(szBottomInfo); }
                                             OP->pushOK = 0;
                                    }
                                    UpdateUI = 1;
                                    break;  // AddUser
                        case  DELUSER:
									if( WndList[DELUSERIDX].cur_sel && (scanfingerOK==0)) // ɨ��ָ�ƻ��id
									{ /* Do FingerScan and Match User , set duID*/;
                                                 if (ACK_SUCCESS == MatchFinger())
                                                  {
                                                       uid =  GetUserId();
                                                       sprintf(EDelUserID.text, "%d", uid);
                                                       sprintf(szBottomInfo, "��ǰ�û�:%d", uid);
                                                       scanfingerOK = 1;
                                                  } else
                                                  {    ParseInfo(szBottomInfo);      }
									}
                                     if(OP->pushOK)
									{
                                              uid = atoi(EDelUserID.text);
                                              if (uid >= 0 && uid < 4096)
                                               {
                                                        if( ACK_SUCCESS == DeleteUser(uid) )  /*if Do Del A User */
                                                        {
                                                               sprintf(szBottomInfo, "%d", uid);
                                                               sprintf(szBottomInfo + strlen(szBottomInfo), ": ��ɾ��", uid);
                                                               sprintf(EDelUserID.text, "%d",  0 );
                                                               admin.admin_id = curSPPara.nUserID;
                                                               insert_admin_entry( 1, uid);
                                                               Delete_user(uid);
                                                        }
                                                       else
                                                       {
                                                             ParseInfo(szBottomInfo);
                                                       }
                                                      OP->pushOK = 0;
                                            }
                                    }
                                   UpdateUI = 1;        break;  // DelUser
                        case  FINGERLOGIN:
								if(debug) printf("fingerin1\n");
                                if (ACK_SUCCESS == MatchFinger())
                                {
									if(debug) printf("fingerin2\n");
                                     curSPPara.nUserID = GetUserId();
                                     OP->ret = ACK_SUCCESS;
                                     usleep(100000);//����ʱ�Ƿ�ֹ�����������ɹ���һ���ɹ��������
                                    // printf("Fingerlogin cur=%d last=%d\n",curStage,lastStage);
                                     //OP->cmd = curStage  = READY;
								     drawReadyToSP();  // ��ӡ�����ʼ��.
								     RD_user_permission(curSPPara.nUserID);
								     if(debug) printf("user:%d permission:%d,%d,%d\n",curSPPara.nUserID,per.print_per,per.scan_per,per.copy_per);
									 OP->cmd = curStage  = READY;//һ��Ҫ�����⣬����ͼ�����
                                }
                                if(debug) printf("fingerin3\n");
                               UpdateUI = 1;           break;
                        case VERIFYALLDEL:
								if(debug) printf("deletalluser,cur_sel = %d\n",WndList[VERIFYALLDELIDX].cur_sel);
                                sprintf(szBottomInfo,  "%s",  "OK��ȷ�ϲ���");
                                if(OP->pushOK && (WndList[VERIFYALLDELIDX].cur_sel==1))
                                {
                                       if (DeleteAllUser(0) == ACK_SUCCESS)
                                       {
                                         sprintf(szBottomInfo,  "%s",  "��¼��ȫ�����");
                                         admin.admin_id = curSPPara.nUserID;
										 insert_admin_entry(2, 0);
										 Delete_All_user();
                                       }
                                       else      {      ParseInfo(szBottomInfo);    curStage = VERIFYALLDEL;   }
                                       OP->pushOK = 0;
                                }
                               UpdateUI = 1;         break; // DllAll
                }
           //     printf("curScanHeadPos :%d\n",  curYPos);
               sleep(1);
       }
   return 0;
}

// ��������
void* KeysListen(void *)
{
      if(debug) printf("KeyListen In\n");
      int  up  =  1;                   //  keydown-up judge
      time_t  s = 0,  t = 0;             //  idle  time count (not accurate of clock use)
      int  keydownloop = 0;  //  first keydown  update.
      time(&s);
      while(1)
       {
             KEYS curKey = (KEYS)get_reg(KeyPadReg);   // ���ް���������
             if((curKey & 0x100)  &&  up)  // keys  press down
             {
                   curKeyPress = curKey;      up  =  0;     keydownloop ++;      if(debug) printf("Listen Key  %x  Down,  keydownloop(1 will updateUI) %d,   curStage: %d\n", curKey, keydownloop,  curStage);
             }
             if ( !(curKey & 0x100)  && (!up ))   // keys  up
             {
                    up = 1;   keydownloop = 0;   time(&s);  if(debug) printf("no key pressed now, up = 1, keydownloop = 0\n");
             }

            // 180 ���ް������޹����ͽ���.
             if ( up && (curStage == READY || curStage ==  FINGERLOGIN ))
             {
                  time(&t);
                  if (t - s  > SysData.IdleSecsToSleep )
                  {
						 if(debug) printf("t=%d,s=%d,t-s=%d\n",t,s,t-s);
                         OnESAVE(); time(&s);
                  }
                  if ((t - s  > SysData.SecsPanelReset) && (SysData.SecsPanelReset!=0))
                  {
                         OnRESET(); time(&s);
                  }
            }else {  time(&s);  }

            if(keydownloop == 1 )
            {
                        for(int i = 0; i < size_message_map; i++)
                        {
                                  if(curKeyPress == message_map[i].key)
                                  {
                                            message_map[i].func();
                                  }
                        }
                        keydownloop ++;
            }
     //     if(debug) printf("Listen Key  %x ,  keydownloop(1 will updateUI) %d,  curStage: %d\n", curKey, keydownloop,  curStage);
           usleep(US_TIME);
       }
       return 0;
}

int ComWriteCmd(char *cmd)
{
	char end[3] = { 0xff, 0xff, 0xff };
	write(seri_fd4.fd,  cmd,  strlen(cmd));
	return write(seri_fd4.fd,  end,  3);
}

void ResetLCDUI()
{
	ComWriteCmd("function0.va0.val=7");
	ComWriteCmd("function0.va1.val=12");
	ComWriteCmd("function0.va2.val=6");
	ComWriteCmd("function0.va3.val=38");
	ComWriteCmd("function1.va0.val=40");
	ComWriteCmd("function1.va1.val=35");
	ComWriteCmd("hbyg.va0.val=29");
}

unsigned short  G_LED_Val  = 0x0F00;    // ���ƶ�Ӧ�ļĴ��� ���� b11: TFSM74,  b10: Power5V   b8: PadWorkEnable

#define  ESaveLED   (0x01 << 5)
#define  ErrorLED   (0x01 << 4)
#define  HHYGLED  (0x01 << 3)
#define  PRINTLED (0x01 << 2)
#define  StartDisableLED  (0x01 << 1)
#define  StartEnable5VLED     (0x01)

void DisplayLED( )
{
     //G_LED_Val  |= 0x0D00;
     if(curStage!=PASSWDLOGIN) G_LED_Val  |= 0x0D00;//��ֹ�����½ʱ��ָ��ģ���ϵ�
     //if(curStage!=PASSWDLOGIN) G_LED_Val  |= 0x0D00;
     switch(curStage)
     {

          case   WARNING :
          G_LED_Val |= ErrorLED + StartDisableLED ;
          break;
          case  PRINTING:
          G_LED_Val  |= PRINTLED;
          G_LED_Val  &= ~ErrorLED;
          G_LED_Val  &= ~StartEnable5VLED;
          G_LED_Val  |=  StartDisableLED;
          break;
          case  SLEEP:
          G_LED_Val  = 0x0300;
          G_LED_Val |= ESaveLED;
          break;
          case HHYG:
          G_LED_Val |= HHYGLED;
          break;
          case PASSWDLOGIN:
          case  READY:
          G_LED_Val &= ~PRINTLED;
          G_LED_Val &= ~StartDisableLED;
          G_LED_Val |= StartEnable5VLED;
          G_LED_Val &= ~ESaveLED;
          G_LED_Val &= ~ErrorLED;
          break;
          case  SCANNING:
          case  SING_PING:
          G_LED_Val   &= ~StartEnable5VLED;
          G_LED_Val |= StartDisableLED;
          G_LED_Val  &= ~ErrorLED;
          break;
     }
     set_reg(PadLEDReg,  G_LED_Val );
 //    if(debug) printf(" inDisPlayLED(),   curStage: %d,  LEDRegValue :%2x,  curKeyPress %2x, UpdateUI %d,  tid_sp %d\n", curStage,  G_LED_Val, curKeyPress, UpdateUI, tid_sp);
}

void* Lcd_display(void *)
{
	while(1)
	{
	       if( UpdateUI )
           {
                for(int i = 0; i < size_stage_map; i++)
                {
                        if(curStage == stage_map[i].stage)
                        {
                                stage_map[i].draw();
                        }
                }
               UpdateUI = 0;
          }

         DisplayLED();

         if (curStage != lastStage) { lastStage = curStage;   }

         usleep(US_TIME* 10);
	}
	return 0;
}

static void DrawSelectItem()
{
	int sz = sizeof(WndList) / sizeof(ItemList);

	for (int i = 0; i < sz; i++)   // i��ʾListѡ��
	{
		int LstItemsPerScr = LstItemsPerScreen;
		if (curStage == WndList[i].stage)
		{
		    if(curStage == SLEEPTIMESET || curStage == PANELRESETTIME)
		    {  LstItemsPerScr = 5; }
			int idx = WndList[i].last_sel % LstItemsPerScr;
			int id = WndList[i].cur_sel%LstItemsPerScr;
			if(idx != id)
			{
                char str[32] = { 0 }; 	snprintf(str, 32, "ref b%d", idx);
                ComWriteCmd(str);
			}
			char sel_id[32] = { 0 }; snprintf(sel_id, 32, "click b%d,1", id);
			ComWriteCmd(sel_id);
			break;
		}
	}
}

// ALWAYS JUMP TO READY
void OnvKeyREADY()
{
    drawREADY(); UpdateUI = 1;
}

void OnvKeyFINGER()
{
    curStage = FINGERLOGIN;    hand_ack[4] = -2;     UpdateUI = 1;
}
//���ԭ��
void OnHHYG()
{
    bTScanMov = !bTScanMov;
}
//����˫��
void OnDMSM()
{
    UpdateUI = 1;
}
///��ֽ����
void OnPZCL()
{
	if (curStage == READY)
	{
		 curStage = SPZCL;
		 UpdateUI = 1;
	}
}
//�ϲ�ԭ��
void OnHBYG()
{
	if (curStage == READY)
	{
		curStage = SHBYG;
		UpdateUI = 1;
	}
}
//���֤��ӡ
void OnSFZFY()
{
	if (curStage == READY)
	{
		curSPPara.bIsSFZ = !curSPPara.bIsSFZ;
		if (curSPPara.bIsSFZ)
		{
			curSPPara.nCombineMode = 0;
			curSPPara.n4To1 =  0;
			curSPPara.nBookSep = 0;
			curSPPara.nEraseBorder = 0;
			curSPPara.nPageMargin = 0;
			curSPPara.nScaleX = curSPPara.nScaleY = 100;
			curSPPara.nQuality = 0;
			snprintf(curSPPara.szQuality, 16, "�ı�/��Ƭ");
		}
		UpdateUI = 1;
	}
}
//Ч��
void OnXYONG()
{
    if(curStage == READY)
    {
       curStage = XIAOYONG;    UpdateUI = 1;
    }
}
//ȷ��
void OnQREN()
{
    if(curStage == READY)
    {
       curStage = SQREN;      UpdateUI = 1;
    }
}
//ģʽ����
void OnMSJY()
{
    if(curStage == READY)
    {
       curStage = SMSJY;  UpdateUI = 1;
    }
}
//ֽ��
void OnPAPER()
{
	if(curStage == READY)
	{
	     if(curSPPara.bAutoPaper) {curSPPara.nPaperSize = nBoxPaperSz = checkPaperBox1Paper();}
		 curStage = PAPER;
		 UpdateUI = 1;
	}
}
//����
void OnSCALE()
{
	if(curStage == READY)
	{
		 curStage = SCALE;
		 UpdateUI = 1;
	}
}
//����
void OnQUALITY()
{
	if(curStage == READY)
	{
		 curStage = QUALITY;
		 UpdateUI = 1;
	}
}
//Ũ��
void OnTINT()
{
	if(curStage == READY)
	{
		 curStage = TINT;
		 UpdateUI = 1;
	}
}
//����
void OnFUNC()
{
	if(curStage == READY)
	{
		curStage = FUNC;
		UpdateUI = 1;
	}
}

void OnUP()
{
	int sz = sizeof(WndList)/sizeof(ItemList);
	for(int i = 0; i < sz; i++)
	{
		if(curStage == WndList[i].stage)
		{
			WndList[i].cur_sel--;
			if(WndList[i].cur_sel < 0)
			{
				WndList[i].cur_sel = 0;
			}
			WndList[i].last_sel = WndList[i].cur_sel + 1;
			break;
		}
	}
	switch (curStage)
	{
	case CHANGEPWD:
	case SETWN:
	case XYSCALESET:
		var_x_y--;	if(var_x_y < 0)  { var_x_y =  0;  }     break;
	default:
		break;
	}
	 UpdateUI = 1;
}

void OnDOWN()
{
	int sz = sizeof(WndList)/sizeof(ItemList);
	for(int i = 0; i < sz; i++)
	{
		if(curStage == WndList[i].stage)
		{
			WndList[i].cur_sel++;
			if(WndList[i].cur_sel >= WndList[i].cnt)
			{
				WndList[i].cur_sel = WndList[i].cnt -1;
			}
			WndList[i].last_sel = WndList[i].cur_sel - 1;
			break;
		}
	}

	switch(curStage)
	{
	case XYSCALESET:
	case SETWN:
	case CHANGEPWD:
		var_x_y++; break;
	default:
		break;
	}
	 UpdateUI = 1;
}
void OnLEFT()
{
	switch(curStage)
	{
	case TINT:

		if(WndList[TINTIDX].cur_sel > 0)  // �ֶ�( sel == 0���Զ�Tint )��Enable
		{
			curLRIdx--;
			if(curLRIdx < nTintMin) curLRIdx = nTintMin;
		}
		break;
   case READY:
      if(bTScanMov) StepTo(curYPos - 600);      break;

	default:break;
	}
	 UpdateUI = 1;
}

void OnRIGHT()
{
	int scale_percent = 100;
	int mm = 0;
	switch(curStage)
	{
	case TINT:
		if(WndList[TINTIDX].cur_sel > 0)  //  �ֶ�( curIdx == 0���Զ�Tint )��Enable
		{
			curLRIdx++;
			if(curLRIdx > nTintMax) curLRIdx = nTintMax;
		}
		break;
	case PAPER:
		if(WndList[PAPERIDX].cur_sel == 1) //  ��ֽ��1ѡ�С�
		{
			curStage = PAPERBOX;
		}else if(WndList[PAPERIDX].cur_sel == 2) //�ֶ�
		{
            curStage = HANDPAPER;
		}
		break;
	case ERASE:
		if (WndList[ERASEIDX].cur_sel)
		{
			curStage = ERASESETMM;
			curSPPara.nEraseBorder = WndList[ERASEIDX].cur_sel;  //0 �أ�1��2�ϣ�3�߿�
		}
		break;
	case XYSCALE:
		if (WndList[XYSCALEIDX].cur_sel)
		{
			curStage = XYSCALESET;
			var_x_y = 0;   // ��ʼ��ѡ�� x ���롣
		}
		break;
	case PAGEMARGIN:
		if (WndList[PAGEMARGINIDX].cur_sel)
		{
			curStage = MARGINSETMM;
		}
		break;
	case STAMP:
		if (WndList[STAMPIDX].cur_sel)
		{
			curStage = PAGEFORMAT;
		}
		break;
	case SHBYG:
		if (WndList[HBYGIDX].cur_sel == 1) curStage = SYGFX;
		else if (WndList[HBYGIDX].cur_sel == 2)curStage = S4To1;
		break;
	case S4To1:
		curStage = SYGFX;
		break;
    case READY:
        if(bTScanMov) StepTo(curYPos + 450);
       break;
	default:break;
	}
	 UpdateUI = 1;
}

void PWD_RD(char *szPwd)
{
	FILE *fp = fopen("/usr/bin/PWD", "r");
	if (fp)
	{
		fgets(szPwd, 7, fp);fclose(fp);
	}

}

void PWD_WR(char * szPwd)
{
	FILE *fp = fopen("/usr/bin/PWD", "w");
	if (fp)
	{
		fputs(szPwd, fp);fclose(fp);
	}

}

void RD_SysProfile(SysProfile &sp)
{
    FILE *fp = fopen("/usr/bin/SYSPROFILE", "rb");
	if (fp)
	{
		fread(&sp.B_KPaper,  1,  sizeof(char), fp);  fread(&sp.IdleSecsToSleep,  1, sizeof(int), fp);  fread(&sp.SecsPanelReset, 1, sizeof(int), fp);  fclose(fp);
		if(debug) printf("read %c,%d,%d\n",sp.B_KPaper,sp.IdleSecsToSleep,sp.SecsPanelReset);
	}
}
void WR_SysProfile(SysProfile &sp)
{
    FILE *fp = fopen("/usr/bin/SYSPROFILE", "wb");
	if (fp)
	{
		fwrite(&sp.B_KPaper,  1, sizeof(char), fp);  fwrite(&sp.IdleSecsToSleep,  1, sizeof(int), fp);  fwrite(&sp.SecsPanelReset, 1, sizeof(int), fp);  fclose(fp);
		if(debug) printf("write %c,%d,%d\n",sp.B_KPaper,sp.IdleSecsToSleep,sp.SecsPanelReset);
	}
}

void RD_MOSHI(TwoSP &spp)
{
    FILE *fp = fopen("/usr/bin/MOSHI", "rb");
	if(fp)
	{
		fread(&spp, 1,  sizeof(TwoSP), fp);		fclose(fp);
	}
}
void WR_MOSHI(TwoSP &spp)
{
    FILE *fp = fopen("/usr/bin/MOSHI", "wb");
	if(fp)
	{
		fwrite(&spp, 1,  sizeof(TwoSP), fp);   fclose(fp);
	}
}
TwoSP twosp = {0};
// ����ѡ�� �� ������ת
void OnOK()
{
	memset(szBottomInfo, 0, sizeof(szBottomInfo));
	int scale_percent = 100;
	int mm = 0;
	char szPwd[10] = { 0 };
	int curUsrCnt = 0, aUid = 0, dUid = 0;
	Item item = {0};
	int idx = 0;
    int pH, pW;
    int tem = 0;
	switch(curStage)
	{
	case TINT:
		curSPPara.bAutoTint = !WndList[TINTIDX].cur_sel;   // �Զ������ֶ�Tint
		if(!curSPPara.bAutoTint) { curSPPara.nTint = curLRIdx; }  // �ֶ��Ļ���nTint ����ȷ��
		usleep(500000);                      // �Ե�Ƭ��
		curStage = READY;	             // ����������
		break;
	case PAPER:
		curStage = READY;
		curSPPara.nPaperBox = WndList[PAPERIDX].cur_sel;       // 0 �Զ�������ֽ�У������ֶ�
		curSPPara.bAutoPaper = ! WndList[PAPERIDX].cur_sel ;   //��ֽ�к��ֶ�Ϊ�Զ�
		if(curSPPara.bAutoPaper || curSPPara.nPaperBox == 1 )  // auto paper; ���ֽ��,�Զ�ֽ��Ĭ��ѡ��ֽ��ѡֽ
		{
             curSPPara.nPaperSize = nBoxPaperSz = checkPaperBox1Paper();
		     switch (nBoxPaperSz)
             {
                case 0x33: sprintf(szBoxPaper, "A4||"); break;
                case 0x38:
                case 0x18:  sprintf(szBoxPaper, "A4=="); break;
                case 0x3F:
                case 0x37:  sprintf(szBoxPaper, "A3");   break;
                case 0x10: sprintf(szBoxPaper,"B5_16K==");  break;
                case 0x1F: sprintf(szBoxPaper, "B4_8K==");  break;
                case 0x16: sprintf(szBoxPaper, "B5_16K||"); break;
             }
             sprintf(curSPPara.szPaper, "%s", szBoxPaper);
		}
		else if(curSPPara.nPaperBox == 2)//�ֶ�ֽ��
		{
		      sprintf(curSPPara.szPaper, "%s", szHandPaper);   curSPPara.nPaperSize = nHandPaperSz; printf("HandPaper %s, nHandPaperSz %2x\n", curSPPara.szPaper, nHandPaperSz);
		}
		if(curSPPara.bAutoScale){ CalScale_Page_to_Scan(); }  //
		break;
	case PAPERBOX:
		curStage = WndList[PAPERBOXIDX].cur_sel ? PAPERTYPE : PAPERSZ;
		break;
	case PAPERTYPE:
		curStage = PAPER;
		sprintf(curSPPara.szPaperType ," %s",WndList[PAPERTYPEIDX].lst[WndList[PAPERTYPEIDX].cur_sel].str);  // ֽ�����ͣ�
		break;
	case PAPERSZ:
		curStage = PAPER;
		item = WndList[PAPERSZIDX].lst[WndList[PAPERSZIDX].cur_sel];
		curSPPara.nPaperSize = item.theValue;
		if(WndList[PAPERSZIDX].cur_sel == 0)  // �Զ�
		{
		     curSPPara.bAutoPaper = 1;
		     curSPPara.nPaperSize  = nBoxPaperSz = checkPaperBox1Paper();
		     switch (nBoxPaperSz)
             {
                case 0x33: sprintf(szBoxPaper, "A4||"); break;
                case 0x38:
                case 0x18: sprintf(szBoxPaper, "A4=="); break;
                case 0x37: sprintf(szBoxPaper, "A3");   break;
                case 0x10:
                case 0x30: sprintf(szBoxPaper,"B5_16K==");  break;
                case 0x1F: sprintf(szBoxPaper, "B4_8K==");  break;
                case 0x16:
                case 0x36: sprintf(szBoxPaper, "B5_16K||"); break;
             }
		}else
		{
		   curSPPara.nPaperSize  = nBoxPaperSz = item.theValue;	   sprintf(szBoxPaper ," %s",  item.str);     // ֽ�����ͣ�
		}
		sprintf(curSPPara.szPaper, "%s",  szBoxPaper);
		if(curSPPara.bAutoScale){ CalScale_Page_to_Scan(); }  //
		break;
    case HANDPAPER:
        curStage = PAPER;
        curSPPara.bAutoPaper = 0;
        curSPPara.nPaperBox =  2;  // �ֶ���ֽ
		item = WndList[HANDPAPERIDX].lst[WndList[HANDPAPERIDX].cur_sel];
		curSPPara.nPaperSize  =  nHandPaperSz =  item.theValue;
		sprintf(szHandPaper ," %s", item.str);    if(debug) printf("HandPaper ��s\n",  szHandPaper); // ֽ�����ͣ�
		sprintf(curSPPara.szPaper, "%s", szHandPaper);
		if(curSPPara.bAutoScale){ CalScale_Page_to_Scan(); }
        break;
	case QUALITY:
		curStage = READY;
		curSPPara.nQuality = WndList[QUALITYIDX].cur_sel;
		sprintf(curSPPara.szQuality,"%s", WndList[QUALITYIDX].lst[WndList[QUALITYIDX].cur_sel].str);
		if(WndList[QUALITYIDX].cur_sel==0) curSPPara.nTint = 4;//�ı�/��Ƭ
		if(WndList[QUALITYIDX].cur_sel==1) curSPPara.nTint = 8;//�ı�
		if(WndList[QUALITYIDX].cur_sel==2) curSPPara.nTint = 4;//��Ƭ
		break;
	case SCALE://����
	    drawReadyToSP();
		if(WndList[SCALEIDX].cur_sel == 0) { curStage = READY; }//�Զ�
		else if(WndList[SCALEIDX].cur_sel == 1) { curStage = SCALEFIX; }//�̶�
		else { curStage = SCALEMANUAL; }//�ֶ�
		curSPPara.bAutoScale = !WndList[SCALEIDX].cur_sel;
		if(curSPPara.bAutoScale) {  CalScale_Page_to_Scan();  }//������Զ�����������������ű���
		else
        {
             GetPageSize(pH, pW);
             if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
             {
                 ScanImageNeedRotate  = 0;  // �̶�����100%100��ͼ��Ҫ��ת.
				 if(curScanRegion.W  > pW || curScanRegion.H > pH)
				 {
					  drawScanPaperNotMatch();
				 }
             }
        }
		break;
	case TEM:
		//drawReadyToSP();
		//curStage = READY;
		tem = WndList[TEMIDX].lst[WndList[TEMIDX].cur_sel].theValue;
		if(debug) printf("SetofWN.tem1 = %d\n",tem);
		if(tem < 0){ tem = abs(tem); tem|= 0x1000;}
		SetofWN.tem1 = tem;
		if(debug) printf("SetofWN.tem1 = %x,%d\n",SetofWN.tem1);
		WR_TEM(SetofWN);
		set_reg(0x2d,tem);
		//sprintf(szBottomInfo, "%s", "���óɹ�");
		curStage = READY;
		break;
	case SCALEFIX:
	    drawReadyToSP();
		curStage = READY;
		curSPPara.nScaleX = curSPPara.nScaleY = WndList[SCALEFIXIDX].lst[WndList[SCALEFIXIDX].cur_sel].theValue;
		GetPageSize(pH, pW);
        if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
        {
            ScanImageNeedRotate  = 0;  // �̶�����100%100��ͼ��Ҫ��ת.
			if(curScanRegion.W  > pW || curScanRegion.H > pH)
			{
				drawScanPaperNotMatch();
			}
        }
		break;
	case SCALEMANUAL:
	    drawReadyToSP();
		scale_percent= atoi(Escale.text);
		if(scale_percent <= Escale.maxV && scale_percent >= Escale.minV)
		{
			curSPPara.nScaleX = curSPPara.nScaleY = scale_percent;
			curStage = READY;
		}
		GetPageSize(pH, pW);
        if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
        {
            ScanImageNeedRotate  = 0;  // �̶�����100%100��ͼ��Ҫ��ת.
			if(curScanRegion.W  > pW || curScanRegion.H > pH)
			{
				 drawScanPaperNotMatch();
			}
         }
		break;
	case ERASE:
		curStage = WndList[ERASEIDX].cur_sel ?  ERASE : FUNC;
		curSPPara.nEraseBorder = WndList[ERASEIDX].cur_sel;  //0 �أ�1��2�ϣ�3�߿�
		break;
	case ERASESETMM:
		mm = atoi(Eeraseset.text);
		if(mm <= Eeraseset.maxV && mm >= Eeraseset.minV)
		{
			curSPPara.nEraseBorder = WndList[ERASEIDX].cur_sel;
			curSPPara.nErasemm = mm;
		    curStage = FUNC;
		}
		break;
	case XYSCALE:
	    drawReadyToSP();
		curStage = WndList[XYSCALEIDX].cur_sel ? XYSCALE : FUNC;
		if (!WndList[XYSCALEIDX].cur_sel)
		{
		    curSPPara.bIsXYScaleSet = 0;
			curSPPara.nScaleX = curSPPara.nScaleY = 100;
		}else
		{
		    curSPPara.bIsXYScaleSet = 1;
		}
		GetPageSize(pH, pW);
        if(curSPPara.nScaleX == 100 && curSPPara.nScaleY == 100)
        {
             ScanImageNeedRotate  = 0;  // �̶�����100%100��ͼ��Ҫ��ת.
			 if(curScanRegion.W  > pW || curScanRegion.H > pH)
			 {
				  drawScanPaperNotMatch();
			 }
        }
		var_x_y = 0;   // ��ʼ��ѡ�� x ���롣
		break;
	case XYSCALESET:
	    curSPPara.bIsXYScaleSet = 1;
		scale_percent = atoi(Exyscalesetx.text);
		if(scale_percent <= Exyscalesetx.maxV && scale_percent >= Exyscalesetx.minV)
		{
			curSPPara.nScaleX = scale_percent;
			scale_percent = atoi(Exyscalesety.text);
			if(scale_percent <= Exyscalesety.maxV && scale_percent >= Exyscalesety.minV)
			{
				curSPPara.nScaleY = scale_percent;
				curStage = FUNC;
			}
		}
		break;
	case PAGEMARGIN:
		if (WndList[PAGEMARGINIDX].cur_sel)
		{
			curSPPara.nPageMargin = 1;
		}
		else
		{
		    curSPPara.nPageMargin = 0;
			curStage = FUNC;
		}
		break;
	case MARGINSETMM:
		mm = atoi(Emarginset.text);
		if(mm <= Emarginset.maxV && mm >= Emarginset.minV)
		{
			curSPPara.nPageMargin = 1;
			curSPPara.nMaginmm = mm;
		    curStage = FUNC;
		}
		break;
	case NEGTIVE:
		curSPPara.nNegtive = WndList[NEGTIVEIDX].cur_sel; curStage = FUNC;
		break;
	case MODE:
		curSPPara.nMode = WndList[MODEIDX].cur_sel;
		//printf("mode = %d,%d\n",curSPPara.nMode,WndList[MODEIDX].cur_sel);
		if(curSPPara.nMode)  WR_Mode(1);//дΪ�ǰ�ȫģʽ;
		else WR_Mode(0);//дΪ��ȫģʽ;
		curStage = ADMIN;
		break;
	case FIX:
		curStage = (STAGE)(WndList[FIXIDX].lst[WndList[FIXIDX].cur_sel].theValue);
		if(curStage == TCRWAIT)
		{
			if(debug) printf("curStatus = %d\n",curStage);
			drawTCRWAIT();
			//sleep(5);
			TCR_ADJUST();
			curStage = TCROK;
		}
		break;
	case GAIN:
		SetofWN.gain = atoi(EGain.text);
		if(debug) printf("SetofWN.gain = %d\n",SetofWN.gain);
		set_reg(0x25,SetofWN.gain);
		WR_GAIN(SetofWN);
		sprintf(szBottomInfo, "%s", "���óɹ�");
		break;
	case BOOKSEP:
		curSPPara.nBookSep = WndList[BOOKSEPIDX].cur_sel;
		if(curSPPara.nBookSep) // �鱾����
		{
		       curSPPara.nCombineMode = 0;
			   curSPPara.n4To1 =  0;
			   curSPPara.bIsSFZ = 0;
		}
		curStage = FUNC;
		break;
	case STAMP:
		curSPPara.nStamp = WndList[STAMPIDX].cur_sel;
		curStage = WndList[STAMPIDX].cur_sel ? STAMP : FUNC;
		break;
	case PAGEFORMAT:  // ӡ�Ǹ�ʽ
		curSPPara.nStampMode = WndList[PAGEFORMATIDX].cur_sel; curSPPara.nStamp = 1; curStage = FUNC;
		break;
	case FUNC:
		curStage = (STAGE)(WndList[FUNCIDX].lst[WndList[FUNCIDX].cur_sel].theValue);
		break;
	case SPZCL:// ��ֽ����
		curStage = READY;
		curSPPara.groups_pages = WndList[PZCLIDX].cur_sel;
		break;
	case SHBYG:  // �ϲ�ԭ��
		curStage = WndList[HBYGIDX].cur_sel ? SHBYG : READY;
		curSPPara.nCombineMode = WndList[HBYGIDX].cur_sel;
		if (curSPPara.nCombineMode)  // ���һģʽ�������֤��ӡ���鱾���뻥��
		{
			curSPPara.nBookSep = 0;
			curSPPara.bIsSFZ = 0;
			if (curSPPara.nCombineMode == 1) curSPPara.nScaleX = curSPPara.nScaleY = 70;
			else curSPPara.nScaleX = curSPPara.nScaleY = 50;
		}
		break;
	case S4To1:
		curStage = SHBYG;
		curSPPara.n4To1 = WndList[S4To1IDX].cur_sel;
		curSPPara.nCombineMode = 2;
		curSPPara.nScaleX = curSPPara.nScaleY = 50;
		curSPPara.nBookSep = 0;
		curSPPara.bIsSFZ = 0;
		break;
	case SYGFX:
		curStage = SHBYG;
		break;
    case NEXTSCAN:
        curStage = SCANNING;
        g_OKContinueScanPressed = 1;
        break;
//	case PASSWDLOGIN:
//		PWD_RD(szPwd);
//
//		if (0 == strcmp(szPwd, EcurRealPsswd.text))	{ curStage = ADMIN; }
//		else
//		{
//			sprintf(szBottomInfo, "%s", "�������");
//		}
//		break;
	case PASSWDLOGIN://�������������½5����������Ӱ�ȫ����Ա����
		PWD_RD(szPwd);
		if(RD_ErrorPW()<5)
		{
			if (0 == strcmp(szPwd, EcurRealPsswd.text))
			{
				G_LED_Val |= 0x0800;  set_reg(PadLEDReg, G_LED_Val); serial5_Finger_open(); //���뷽ʽ��½�ɹ������¸�ָ��ģ���ϵ㣬�Է����û������ܵ�ʹ��
				curStage = ADMIN;	Unlock_ErrorPW();
			}
			else
			{
				sprintf(szBottomInfo, "%s", "�������");
				WR_ErrorPW();
			}
		}else
			sprintf(szBottomInfo, "%s", "����Ա����");
		break;
	case ADMIN:
		curStage = (STAGE)(WndList[ADMINIDX].lst[WndList[ADMINIDX].cur_sel].theValue);
		memset(EPsswd.text, 0, sizeof(EPsswd.text));					EPsswd.cur_idx = 0;
		memset(EcurRealPsswd.text, 0, sizeof(EcurRealPsswd.text));		EcurRealPsswd.cur_idx = 0;
		break;
	case USERMANAGE:
		curStage = (STAGE)(WndList[USERMANAGEIDX].lst[WndList[USERMANAGEIDX].cur_sel].theValue);
		var_x_y = 0;
		memset(EPsswd.text, 0, sizeof(EPsswd.text));					                EPsswd.cur_idx = 0;
		memset(EcurRealPsswd.text, 0, sizeof(EcurRealPsswd.text));		EcurRealPsswd.cur_idx = 0;
		memset(EOldPsswd.text, 0, sizeof(EOldPsswd.text));				        EOldPsswd.cur_idx = 0;
		memset(EOldRealPsswd.text, 0, sizeof(EOldRealPsswd.text));		EOldRealPsswd.cur_idx = 0;
		memset(ENew1Psswd.text, 0, sizeof(ENew1Psswd.text));			    ENew1Psswd.cur_idx = 0;
		memset(ERealNew1Psswd.text, 0, sizeof(ERealNew1Psswd.text));	ERealNew1Psswd.cur_idx = 0;
		memset(ENew2Psswd.text, 0, sizeof(ENew2Psswd.text));			        ENew2Psswd.cur_idx = 0;
		memset(ERealNew2Psswd.text, 0, sizeof(ERealNew2Psswd.text));	ERealNew2Psswd.cur_idx = 0;
		sprintf(EDelUserID.text, "%s", "0    ");    EDelUserID.cur_idx = 0;
        op.pushOK =  0;
        op.cmd = curStage;
        if(debug) printf("op.cmd = %d\n",curStage);
		break;
	case VERIFYALLDEL:
		op.pushOK = 1;
		break;
	case CHANGEPWD:
		PWD_RD(szPwd);
		if (0 == strcmp(szPwd, EOldRealPsswd.text))
		{
			if (strcmp(ERealNew1Psswd.text, ERealNew2Psswd.text) == 0)
			{
				if (strlen(ERealNew1Psswd.text) > 5){ PWD_WR(ERealNew1Psswd.text); sprintf(szBottomInfo, "%s", "�����޸ĳɹ�"); }
				else{ sprintf(szBottomInfo, "%s", "���볤������6λ"); }
			}
			else{ sprintf(szBottomInfo, "%s", "���������β�һ��"); }
		}
		else
		{
			sprintf(szBottomInfo, "%s", "ԭ�������");
		}
		break;
	case SETWN:
		//�����õ���ֵд��Ĵ����У��������ݿ��б���
		SetofWN.tem1 = atoi(ETem1.text);
		//set_reg(0x00,SetofWN.tem1);
		usleep(10000);
		SetofWN.tem2 = atoi(ETem2.text);
		//set_reg(0x01,SetofWN.tem2);
		usleep(10000);
		SetofWN.tint = atoi(ETint.text);
		//set_reg(0x02,SetofWN.tint);
		WR_WN(SetofWN);
		sprintf(szBottomInfo,"%s","���óɹ�");
		break;
    case DELUSER: scanfingerOK = 0; op.pushOK = 1; break;
	case ADDUSER:	op.pushOK = 1;     break;
    case READY:   if(bTScanMov) curYPos = Y_ORI;    break;
    case FINGERLOGIN:  break;
    case XIAOYONG:  	curStage = (STAGE)(WndList[XYONGIDX].lst[WndList[XYONGIDX].cur_sel].theValue);		break;
    case SLEEPTIMESET:
      SysData.IdleSecsToSleep = WndList[SLEEPTIMSETIDX].lst[WndList[SLEEPTIMSETIDX].cur_sel].theValue;      WR_SysProfile(SysData);   curStage = XIAOYONG;       break;
    case PANELRESETTIME:
      SysData.SecsPanelReset = WndList[PANELRSTIDX].lst[WndList[PANELRSTIDX].cur_sel].theValue;      WR_SysProfile(SysData);   curStage = XIAOYONG;      break;
   case BKPAPERSET:
      SysData.B_KPaper = WndList[BKPAPERIDX].cur_sel;      WR_SysProfile(SysData);         curStage = XIAOYONG;      break;
   case SLEEP:  break;
   case REPEAT:

		if(WndList[REPEATIDX].cur_sel == 0)
		{
			curStage = REPEAT_PRINT;           //���OK�����л����ظ���ӡ
											   //��Ӧ���ڴ˴����ô�ӡ����Ϣ����ֽ�Ŵ�С����Ϣ
		}
		else if(WndList[REPEATIDX].cur_sel == 1)
		{
			curStage = REPEAT_SCAN;			//���OK�л����ظ�ɨ��
											//��Ӧ���ڴ˴�����ɨ�����Ϣ����ֽ�Ŵ�С����Ϣ
		}
		else
		{
			curStage = REPEAT_SCANPRINT;	//���OK�л����ظ���ӡ
											//��Ӧ���ڴ˴����ø�ӡ����Ϣ����ֽ�ŵĴ�С��Ϣ
		}
		if(debug) printf("curStage is %d\n",curStage);
   case SMSJY:  idx = WndList[MSJYIDX].cur_sel;  if(idx < 2) {RD_MOSHI(twosp); curSPPara = twosp.sp[idx];} else {twosp.sp[idx%2] = curSPPara; WR_MOSHI(twosp); }break;
   default: curStage = READY;  break;
	}
	 UpdateUI = 1;
}

void OnRETURN()
{
	int scale_percent = 100;
	int mm = 0;
	switch(curStage)
	{
	case TCROK:
		curStage = FIX;
		break;
	case SETWN:
		curStage = FIX;
		break;
	case FIX :
		curStage = XIAOYONG;
		break;
	case GAIN:
		curStage = FIX;
		break;
	case TEM:
		curStage = FIX;
		break;
	case TINT:
		curStage = READY;
		break;
	case MODE:
		curStage = READY;
		break;
	case PAPER:
		curStage = READY;
		break;
	case PAPERBOX:
		curStage = PAPER;
		break;
	case PAPERTYPE:
		curStage = PAPER;
		break;
	case PAPERSZ:
		curStage = PAPER;
		break;
	case SCALEMANUAL:
		scale_percent = atoi(Escale.text);
		if(scale_percent <= Escale.maxV && scale_percent >= Escale.minV)
		{
			curStage = SCALE;
		}
		break;
	case SCALEFIX:
		curStage = SCALE;
		break;
	case ERASE:
	case PAGEMARGIN:
	case XYSCALE:
	case NEGTIVE:
	case BOOKSEP:
	case STAMP:
		curStage = FUNC;
		break;
	case PAGEFORMAT:
		curStage = STAMP;
		break;
	case ERASESETMM:
		mm = atoi(Eeraseset.text);
		if(mm <= Eeraseset.maxV && mm >= Eeraseset.minV)
		{
		    curStage = ERASE;
		}
		break;
	case XYSCALESET:
		scale_percent = atoi(Exyscalesetx.text);
		if(scale_percent <= Exyscalesetx.maxV && scale_percent >= Exyscalesetx.minV)
		{
			scale_percent = atoi(Exyscalesety.text);
			if(scale_percent <= Exyscalesety.maxV && scale_percent >= Exyscalesety.minV)
			{
				curStage = XYSCALE;
			}
		}
		break;
	case MARGINSETMM:
		mm = atoi(Emarginset.text);
		if(mm <= Emarginset.maxV && mm >= Emarginset.minV)
		{
		    curStage = PAGEMARGIN;
		}
		break;
	case VERIFYALLDEL:
	case CHANGEPWD:
	case DELUSER:
		if(isMatching==0)
		{
			curStage = USERMANAGE; op.cmd = USERMANAGE ;  break;
		}else
			break;
	case USERMANAGE: 	curStage = ADMIN; 	break;
	case ADDUSER:
		curStage = USERMANAGE; op.cmd = USERMANAGE ;  break;
    case FINGERLOGIN:
    case PASSWDLOGIN:    if(curSPPara.nMode==0) curStage = FINGERLOGIN; if(curSPPara.nMode==1) curStage = READY;  break;
	default:curStage = READY; break;
	}
	 UpdateUI = 1;
}

//��ӡ
void OnPRINT()
{
	  //curStage = REPEAT;  //�ظ���ӡ״̬
      //UpdateUI = 1;
}
//����
void OnACCESS()
{
	close(seri_fd5.fd);   seri_fd5.fd = -1;
	G_LED_Val &= 0xf7ff ;  set_reg(PadLEDReg, G_LED_Val); //ѡ���������¼����Ҫ�ȹص�ָ��ģ�飬��½�ɹ��ٴ򿪣���ָֹ�ƿ���fingerlogin��
    op.cmd = curStage =  PASSWDLOGIN;    memset(szBottomInfo, 0, sizeof(szBottomInfo));   UpdateUI = 1;
}
//���ܺͻ���
void OnESAVE()
{
	 memset(EPsswd.text, 0, sizeof(EPsswd.text));					EPsswd.cur_idx = 0;
     if(curStage != SLEEP) // turn to sleep.
     {
            op.cmd =  curStage = SLEEP;    G_LED_Val = 0x0320;  set_reg(PadLEDReg, G_LED_Val);   ComWriteCmd("sleep=1");  close(seri_fd5.fd);   seri_fd5.fd = -1;   MoveCtrlVal  &=  ~CIS_CLOCK_SET;
			set_reg(MOVE_CTRL_REG,  MoveCtrlVal);
			usleep(10000);
			PrnCmdRegVal |= 0x8000;//bit15��1 ��������״̬
			set_reg(PRN_CMD_REG,PrnCmdRegVal);
     }
     else     // ˯�����а�������
     {
		if(curSPPara.nMode==0){//��ȫģʽ
			   if(debug) printf("safe nMode=%d\n",curSPPara.nMode);
			   curKeyPress = vKeyFINGER ;       op.cmd =  curStage = FINGERLOGIN ;
			   set_reg(PadLEDReg,  0x0F00);
			   serial5_Finger_open();
			   ComWriteCmd("sleep=0");//HMI����
			   MoveCtrlVal  |=  CIS_CLOCK_SET;     set_reg(MOVE_CTRL_REG,  MoveCtrlVal);
           }else{
			   if(debug) printf("notsafe nMode=%d\n",curSPPara.nMode);
			   curSPPara.nUserID = 0; 	per.copy_per = 1;	per.scan_per = 1;	per.print_per = 1;//��ͨģʽ�£����ָܻ���ID��Ϊ0��Ȩ�޾���
			   drawReadyToSP();
			   curKeyPress = vKeyFINGER ;
			   op.cmd =  curStage = READY ;
			   set_reg(PadLEDReg,  0x0F00);
			   serial5_Finger_open();
			   ComWriteCmd("sleep=0");
			   MoveCtrlVal  |=  CIS_CLOCK_SET;     set_reg(MOVE_CTRL_REG,  MoveCtrlVal);
           }
           usleep(10000);
		   PrnCmdRegVal &= ~0x8000;//���߻���
		   set_reg(PRN_CMD_REG,PrnCmdRegVal);
     }
     UpdateUI = 1;
}
///����
void OnRESET()
{
	int mode = curSPPara.nMode;
    int uid = curSPPara.nUserID;
    curSPPara = DefSP;   //��Ĭ�ϲ���ֵ������ǰ
    curSPPara.nUserID = uid;  //  mode,uid���ܸ�λ.
	curSPPara.nMode = mode;
	sprintf(Eready.text,"%s","1  "); Eready.cur_idx = 0;
	sprintf(Escale.text,"%s"," 95"); Escale.cur_idx = 0;
	memset(EPsswd.text, 0, sizeof(EPsswd.text));					EPsswd.cur_idx = 0;
	memset(EcurRealPsswd.text, 0, sizeof(EcurRealPsswd.text));		EcurRealPsswd.cur_idx = 0;
	memset(EOldPsswd.text, 0, sizeof(EOldPsswd.text));				EOldPsswd.cur_idx = 0;
	memset(EOldRealPsswd.text, 0, sizeof(EOldRealPsswd.text));		EOldRealPsswd.cur_idx = 0;
	memset(ENew1Psswd.text, 0, sizeof(ENew1Psswd.text));			ENew1Psswd.cur_idx = 0;
	memset(ERealNew1Psswd.text, 0, sizeof(ERealNew1Psswd.text));	ERealNew1Psswd.cur_idx = 0;
	memset(ENew2Psswd.text, 0, sizeof(ENew2Psswd.text));			ENew2Psswd.cur_idx = 0;
	memset(ERealNew2Psswd.text, 0, sizeof(ERealNew2Psswd.text));	ERealNew2Psswd.cur_idx = 0;

	memset(EGain.text,0,sizeof(EGain.text)); EGain.cur_idx = 0;
	memset(ETem1.text,0,sizeof(ETem1.text)); ETem1.cur_idx = 0;
	memset(ETem2.text,0,sizeof(ETem2.text)); ETem2.cur_idx = 0;
	memset(ETint.text,0,sizeof(ETint.text)); ETint.cur_idx = 0;

	sprintf(EAddUserID.text, "%s", "0    ");    EAddUserID.cur_idx = 0;
	sprintf(EDelUserID.text, "%s", "0    ");    EDelUserID.cur_idx = 0;
    ResetLCDUI();
	UpdateUI = 1;
}

//��ʼ
void OnRUN()
{
       g_RunPressed = 1;
       g_exit = 0;
       int nTint =  RD_TINT();//�����ݿ��ж�ȡ̼��Ũ�ȵ�ֵ
	   if(nTint ==9)	//������ˣ���������Ũ��Ϊ9������8���м��㣬����÷���ĵĵĽ����ϵ���Ũ�ȵ���10����λ������������
	   {
			nTint =8;
	   }
	  curSPPara.nTint = nTint;
	if(curStage == READY){
		   if(!tid_sp)
		   {
				if(per.copy_per){
					curSPPara.nCopy = atoi(Eready.text);


					pthread_create(&tid_sp, 0, ScanPrint, 0);
					curStage = SING_PING;      UpdateUI = 1;
				}else{
					char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"�޸�ӡȨ��\"");
					ComWriteCmd(t4);
				}
		  }
      }
       //�ظ���ӡ
	else if(curStage == REPEAT_PRINT)
	{
		if(debug) printf("REPEAT_PRINT IN!\n");

		if(!tid_rep_prn)
		{
			prn_repprint_fd =  open("/media/repeat_print", O_RDWR|O_CREAT);
			lseek(prn_repprint_fd,0L,SEEK_SET);

			repeat_print_sign = 1;

			if(debug) printf("prn_repprint_fd is %d\n" , prn_repprint_fd);

			PRN_SCAN_PARA  psp ={ prn_repprint_fd,  1 , 0};

			pthread_create(&tid_rep_prn, 0, TPrint,  (void*)&psp);              // Print implemented  in PrintJob.cpp ��
			curStage = REPEAT_PRINT;   UpdateUI = 1;
			//pthread_join(tid_rep_prn, 0);   tid_rep_prn = 0;
		}                //  ��ֹ�߳�ʱ���жϸ�ֵ��
	}
	//�ظ���ӡ  ɨ��-��ӡ
	else if(curStage == REPEAT_SCANPRINT)
	{
		if(debug) printf("REPEAT_SCANPRINT IN!\n");
		if(!tid_rep_sp)
		   {
				if(per.copy_per){
					curSPPara.nCopy = atoi(Eready.text);

					//prn_repscanprint_fd =  open("/media/repeat_scanprint", O_RDWR|O_CREAT);

					//lseek(prn_repscanprint_fd,0L,SEEK_SET);

					repeat_scanprint_sign = 1;

					//printf("prn_repprint_fd is %d\n" , prn_repscanprint_fd);
					pthread_create(&tid_rep_sp, 0, RepeatScanPrint, 0);
					curStage = REPEAT_SCANPRINT;      UpdateUI = 1;

					pthread_join(tid_rep_sp, 0);   tid_rep_sp = 0;
					repeat_scanprint_sign = 0;
				}else{
					char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"�޸�ӡȨ��\"");
					ComWriteCmd(t4);
				}
		  }
	}
	//�ظ�ɨ��
	else if(curStage == REPEAT_SCAN)
	{
		if(debug) printf("REPEAT_SCAN IN!\n");
		if(!tid_rep_scan)
		   {
				if(per.copy_per){
					curSPPara.nCopy = atoi(Eready.text);

					repeat_scan_sign = 1;

					pthread_create(&tid_rep_scan, 0, RepeatScanPrint, 0);
					curStage = REPEAT_SCAN;      UpdateUI = 1;

					pthread_join(tid_rep_scan, 0);   tid_rep_scan = 0;
					repeat_scan_sign = 0;
				}else{
					char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"�޸�ӡȨ��\"");
					ComWriteCmd(t4);
				}
		  }
	}

}
//����ת��
void OnFLIP()//���¶�Ũ�����ð�������������,
{
//	if(curStage == READY)
//	{
//		 curStage = SETWN;
//		 UpdateUI = 1;
//	}
}

//���ֹͣ
void OnCLEARSTOP()
{
	switch(curStage)
	{
	case READY:
		//memset(Eready.text, 0, sizeof(Eready.text));
		sprintf(Eready.text, "%s","1  ");
		Eready.cur_idx = 0;
		g_exit = 1;
		break;
	case TCRWAIT:
		curStage = TCROK;
		break;
	case SCALEMANUAL:
		//memset(Escale.text, 0, sizeof(Escale.text));
		sprintf(Escale.text, "%s","   ");
		Escale.cur_idx = 0;
		break;
	case XYSCALESET:
		if(!var_x_y)
		{
			sprintf(Exyscalesetx.text, "%s","   ");
			Exyscalesetx.cur_idx = 0;
		}else
		{
			sprintf(Exyscalesety.text, "%s","   ");
			Exyscalesety.cur_idx = 0;
		}
		break;
	case MARGINSETMM:
		sprintf(Emarginset.text, "%s","  ");
		Emarginset.cur_idx = 0;
		break;
	case ERASESETMM:
		sprintf(Eeraseset.text, "%s","  ");
		Eeraseset.cur_idx = 0;
		break;
	case PASSWDLOGIN:
		memset(EPsswd.text, 0, sizeof(EPsswd.text));                 EPsswd.cur_idx = 0;
		memset(EcurRealPsswd.text, 0, sizeof(EcurRealPsswd.text));   EcurRealPsswd.cur_idx = 0;
		break;
	case CHANGEPWD:
		if (var_x_y % 3 == 0)
		{
			memset(EOldPsswd.text, 0, sizeof(EOldPsswd.text));         EOldPsswd.cur_idx = 0;
			memset(EOldRealPsswd.text, 0, sizeof(EOldRealPsswd.text)); EOldRealPsswd.cur_idx = 0;
		}
		if (var_x_y % 3 == 1)
		{
			memset(ENew1Psswd.text, 0, sizeof(ENew1Psswd.text));          ENew1Psswd.cur_idx = 0;
			memset(ERealNew1Psswd.text, 0, sizeof(ERealNew1Psswd.text));  ERealNew1Psswd.cur_idx = 0;
		}
		if (var_x_y % 3 == 2)
		{
			memset(ENew2Psswd.text, 0, sizeof(ENew2Psswd.text));          ENew2Psswd.cur_idx = 0;
			memset(ERealNew2Psswd.text, 0, sizeof(ERealNew2Psswd.text));  ERealNew2Psswd.cur_idx = 0;
		}
		break;
	case GAIN:
		memset(EGain.text, 0, sizeof(EGain.text)); EGain.cur_idx = 0;
		break;
	case SETWN:
		if (var_x_y % 3 == 0)
		{
			//memset(EOldPsswd.text, 0, sizeof(EOldPsswd.text));         EOldPsswd.cur_idx = 0;
			memset(ETem1.text, 0, sizeof(ETem1.text)); ETem1.cur_idx = 0;
		}
		if (var_x_y % 3 == 1)
		{
			//memset(ENew1Psswd.text, 0, sizeof(ENew1Psswd.text));          ENew1Psswd.cur_idx = 0;
			memset(ETem2.text, 0, sizeof(ETem2.text));  ETem2.cur_idx = 0;
		}
		if (var_x_y % 3 == 2)
		{
			//memset(ENew2Psswd.text, 0, sizeof(ENew2Psswd.text));          ENew2Psswd.cur_idx = 0;
			memset(ETint.text, 0, sizeof(ETint.text));  ETint.cur_idx = 0;
		}
		break;
	case ADDUSER:
		sprintf(EAddUserID.text, "%s", "1    ");     EAddUserID.cur_idx = 0;
		break;
	case DELUSER:
		sprintf(EDelUserID.text, "%s", "1    ");     EDelUserID.cur_idx = 0;
		break;
	case FINGERLOGIN: break;
    case SCANNING:
    case PRINTING:
    case SING_PING:
    case WARNING:
        g_exit = 1;       PrnCmdRegVal  |=   PRINT_TASK_HALT;        set_reg(PRN_CMD_REG, PrnCmdRegVal);  break;    //HwReset() ;   break;
	default:
		curStage = READY;		break;
	}
	memset(szBottomInfo, 0, sizeof(szBottomInfo));
	UpdateUI = 1;
}


void OnNUM()
{
	char bit  = 0;
	int  find  = 0;
	for(int  i  = 0;  i < 10;  i++ )
	{
	         if(curKeyPress == pairs[i].key)        {          bit =  pairs[i].val;        find = 1;     break;	       }
	}
	if(debug) printf("OnNUM : Key  %c  Pressed,   curStage: %d\n",  bit, curStage);
	if(!find ) return;

    switch(curStage)
	{
	case READY:
		if('0' == bit && 0 == Eready.cur_idx){ break; } // ������λ����Ϊ0
		Eready.text[Eready.cur_idx++] =  bit;      if(debug) printf("Eready :%s\n",  Eready.text);
		if(Eready.cur_idx > Eready.maxL -1){ Eready.cur_idx = Eready.maxL - 1; }
		break;
	case SCALEMANUAL:
		Escale.text[Escale.cur_idx++] =  bit;
		if(Escale.cur_idx > Escale.maxL -1){ Escale.cur_idx = Escale.maxL - 1; }
		break;
	case XYSCALESET:
		if(var_x_y)
		{
			Exyscalesety.text[Exyscalesety.cur_idx++] =  bit;
			if(Exyscalesety.cur_idx > Exyscalesety.maxL -1){ Exyscalesety.cur_idx = Exyscalesety.maxL - 1; }
		}else
		{
			Exyscalesetx.text[Exyscalesetx.cur_idx++] =  bit;
			if(Exyscalesetx.cur_idx > Exyscalesetx.maxL -1){ Exyscalesetx.cur_idx = Exyscalesetx.maxL - 1; }
		}
		break;
	case MARGINSETMM:
		Emarginset.text[Emarginset.cur_idx++] =  bit;
		if(Emarginset.cur_idx > Emarginset.maxL -1){ Emarginset.cur_idx = Emarginset.maxL - 1; }
		break;
	case ERASESETMM:
		Eeraseset.text[Eeraseset.cur_idx++] =  bit;
		if(Eeraseset.cur_idx > Eeraseset.maxL -1){ Eeraseset.cur_idx = Eeraseset.maxL - 1; }
		break;
	case PASSWDLOGIN:
		if (EPsswd.cur_idx < EPsswd.maxL)
		{
			EPsswd.text[EPsswd.cur_idx++] = '*';
			EcurRealPsswd.text[EcurRealPsswd.cur_idx++] = bit;
		}
		break;
	case CHANGEPWD:
		if (var_x_y % 3 == 0)
		{
			if (EOldPsswd.cur_idx < EOldPsswd.maxL)
			{
				EOldPsswd.text[EOldPsswd.cur_idx++] = '*';
				EOldRealPsswd.text[EOldRealPsswd.cur_idx++] = bit;
			}
		}
		if (var_x_y % 3 == 1)
		{
			if (ENew1Psswd.cur_idx < ENew1Psswd.maxL)
			{
				ENew1Psswd.text[ENew1Psswd.cur_idx++] = '*';
				ERealNew1Psswd.text[ERealNew1Psswd.cur_idx++] = bit;
			}
		}
		if (var_x_y % 3 == 2)
		{
			if (ENew2Psswd.cur_idx < ENew2Psswd.maxL)
			{
				ENew2Psswd.text[ENew2Psswd.cur_idx++] = '*';
				ERealNew2Psswd.text[ERealNew2Psswd.cur_idx++] = bit;
			}
		}
		break;
	case GAIN:
		if (EGain.cur_idx < EGain.maxL)
			{
				//EOldPsswd.text[EOldPsswd.cur_idx++] = '*';
				EGain.text[EGain.cur_idx++] = bit;
			}
		break;
	case SETWN:
		if (var_x_y % 3 == 0)
		{
			if (ETem1.cur_idx < ETem1.maxL)
			{
				//EOldPsswd.text[EOldPsswd.cur_idx++] = '*';
				ETem1.text[ETem1.cur_idx++] = bit;
			}
		}
		if (var_x_y % 3 == 1)
		{
			if (ETem2.cur_idx < ETem2.maxL)
			{
				//ENew1Psswd.text[ENew1Psswd.cur_idx++] = '*';
				ETem2.text[ETem2.cur_idx++] = bit;
			}
		}
		if (var_x_y % 3 == 2)
		{
			if (ETint.cur_idx < ETint.maxL)
			{
				//ENew2Psswd.text[ENew2Psswd.cur_idx++] = '*';
				ETint.text[ETint.cur_idx++] = bit;
			}
		}
		break;
	case ADDUSER:
		EAddUserID.text[EAddUserID.cur_idx++] = bit;
		if (EAddUserID.cur_idx > EAddUserID.maxL - 1){ EAddUserID.cur_idx = EAddUserID.maxL - 1; }
		break;
	case DELUSER:
		EDelUserID.text[EDelUserID.cur_idx++] = bit;
		if (EDelUserID.cur_idx > EDelUserID.maxL - 1){ EDelUserID.cur_idx = EDelUserID.maxL - 1; }
		break;
	default: break;
	}
	 UpdateUI = 1;
}

void OnSTAR()
{
     UpdateUI = 1;
}
void OnCROSS()
{

     UpdateUI = 1;
}


void drawLOAD()
{

}
void drawWAIT()
{

}
void drawHEAT()
{
}

void drawFINGERLOGIN()    // ָ�Ƶ�½
{
    if(debug) printf("DrawFingerLogin UpdateUI is %d\n", UpdateUI);

	if (curStage != lastStage)
	{
	    ComWriteCmd("page loading");
        sleep(1);
		ComWriteCmd("page fingerlogin");
	}
     memset(szBottomInfo, 0, sizeof(szBottomInfo));
	 ParseInfo(szBottomInfo);
	 char t0[256] = {0};  sprintf(t0, "t0.txt=\"%s\"", szBottomInfo);	     ComWriteCmd(t0);
	 if(debug) printf("Leave Draw Finger,  curStage %d\n", curStage);
}

void drawPASSWDLOGIN()   // ����Ա�������½
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pwdlogin");
	}
	char pwd[32] = { 0 }; sprintf(pwd,  "t1.txt=\"%s\"", EPsswd.text);
	ComWriteCmd(pwd);

	char info[64] = { 0 }; sprintf(info, "t3.txt=\"%s\"", szBottomInfo);
	ComWriteCmd(info);
}

void drawADMIN()                 // ����Ա����, ѡ��ӡor����
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page admin");
	}
	DrawSelectItem();
}
void drawCHANGEPWD()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pwd_change");
	}
	if (var_x_y % 3 == 0)
	{   ComWriteCmd("t3.txt=\"<\""); ComWriteCmd("t4.txt=\"\"");   ComWriteCmd("t5.txt=\"\"");    }
	else if (var_x_y % 3 == 1)
	{   ComWriteCmd("t3.txt=\"\"");  ComWriteCmd("t4.txt=\"<\"");  ComWriteCmd("t5.txt=\"\""); }
	else
	{   ComWriteCmd("t3.txt=\"\"");  ComWriteCmd("t4.txt=\"\"");   ComWriteCmd("t5.txt=\"<\""); }

	char pwd[32] = { 0 };
	sprintf(pwd,  "t0.txt=\"%s\"", EOldPsswd.text);     ComWriteCmd(pwd);
	sprintf(pwd,  "t1.txt=\"%s\"", ENew1Psswd.text);	ComWriteCmd(pwd);
	sprintf(pwd,  "t2.txt=\"%s\"", ENew2Psswd.text);	ComWriteCmd(pwd);
	char info[64] = { 0 }; sprintf(info, "t6.txt=\"%s\"", szBottomInfo);
	ComWriteCmd(info);
}
void drawADDUSER()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page add_user");
	}
	char UsrID[32] = { 0 };
	sprintf(UsrID, "n0.val=%d", atoi(EAddUserID.text));	  ComWriteCmd(UsrID);
	char info[64] = { 0 };  sprintf(info, "t4.txt=\"%s\"", szBottomInfo);	ComWriteCmd(info);
}

void drawDELUSER()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page delete_user");
	}
	char UsrID[32] = { 0 };
	sprintf(UsrID, "n0.val=%d", atoi(EDelUserID.text));	  ComWriteCmd(UsrID);
	DrawSelectItem();
	char info[64] = { 0 };  sprintf(info, "t4.txt=\"%s\"", szBottomInfo);	ComWriteCmd(info);
}

void drawUSERMANAGE()
{
	int id_screen = WndList[USERMANAGEIDX].cur_sel / LstItemsPerScreen;
	int id_lastscr = WndList[USERMANAGEIDX].last_sel / LstItemsPerScreen;
	if(id_screen != id_lastscr || curStage != lastStage)
	{
		char page[32] = { 0 }; sprintf(page,  "page user_manage%d", id_screen);		ComWriteCmd(page);
		if(debug) printf("page user_manage%d\n", id_screen);
	}
 	DrawSelectItem();
}

void drawVERIFYALLDEL()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page verify_alldel");
	}
	DrawSelectItem();
	char info[64] = { 0 };  sprintf(info, "t4.txt=\"%s\"", szBottomInfo);	ComWriteCmd(info);
}

void DrawUIStatusPics()
{
	UI_LCD_Status_PicId p0id = P_NULL, p1id = P_NULL, p2id = P_NULL, p3id = P_NULL, p4id = P_NULL, p5id = P_NULL;
	if (curSPPara.groups_pages) p0id = P_FENYE;
	if (curSPPara.nEraseBorder) p2id = P_ERASEBORDER;
	if (curSPPara.nPageMargin)  p3id = P_PAGEMARGIN;
	if (curSPPara.nNegtive)		p4id = P_NEGTIVE;
	if (curSPPara.nStamp)		p5id = P_STAMP;
	if (curSPPara.bIsSFZ)
	{
		p1id =  P_SFZ;               // p1 ���֤
		p2id = p3id = P_NULL;
	}
	else if (curSPPara.nBookSep)
	{
		p1id = P_BOOKSEP;            // p1 �鱾����
		p2id = P_NULL;
	}
	else if (curSPPara.nCombineMode)
	{
		if (curSPPara.nCombineMode == 1)       p1id = P_2TO1;     // P1: ���һģʽ
		else if (curSPPara.nCombineMode == 2)  p1id = P_4TO1;
		p2id = p3id = P_NULL;
	}
	char str[32] = { 0 };
	snprintf(str, 32, "p0.pic=%d", p0id);		ComWriteCmd(str);    //P0 ��ҳ
	snprintf(str, 32, "p1.pic=%d", p1id);		ComWriteCmd(str);    //P1 ���֤/���һ/�鱾����
	snprintf(str, 32, "p2.pic=%d", p2id);		ComWriteCmd(str);    //P2 ����
	snprintf(str, 32, "p3.pic=%d", p3id);		ComWriteCmd(str);    //P3 װ��
	snprintf(str, 32, "p4.pic=%d", p4id);		ComWriteCmd(str);    //P4 ��ת
	snprintf(str, 32, "p5.pic=%d", p5id);		ComWriteCmd(str);    //P5 ӡ��
}

// ��黥�����
void DoUIOperationCheck()
{
	if (curSPPara.bIsSFZ)
	{
		curSPPara.nEraseBorder = 0;
		curSPPara.nPageMargin  = 0;
	}
	else if (curSPPara.nBookSep)
	{
		curSPPara.nEraseBorder = 0;
	}
	else if (curSPPara.nCombineMode)
	{
		curSPPara.nEraseBorder = 0;
		curSPPara.nPageMargin  = 0;
	}
	else if (curSPPara.groups_pages)
	{
		curSPPara.nEraseBorder = 0;
		curSPPara.nPageMargin  = 0;
	}
}
void drawREADY()
{
	//printf("drawREADY cur=%d last=%d\n",curStage,lastStage);
  	if (curStage != lastStage)
	{
		ComWriteCmd("page main");   if(debug) printf("page main\n");
	}
	char t0[32] = { 0 }; sprintf(t0,  "t0.txt=\"%s\"", curSPPara.szPaper);

    //������ȷ��ֽ����Դ������ǰ��֪�ײ�
	PModeRegVal &= ~(1 << 9);//ֽ����Դλ��0,Ĭ��ֽ��
	if(curSPPara.nPaperBox == 2)  PModeRegVal |= ( 1 <<  9 );   set_reg(PMODE_REG, PModeRegVal);               // ��1��ֽ����Դ bit 9 ��1��ʾ�ֶ�ֽ����Ч(��ʱnPaperBox==2), 0��ʾֽ����Ч

	ComWriteCmd(t0);
	char n0[32] = { 0 }; sprintf(n0,  "n0.val=%d", atoi(Eready.text));
	ComWriteCmd(n0);
	char t1[32] = { 0 };
	if(curSPPara.bAutoScale)
	{
	   sprintf(t1,  "t1.txt=\"%d%% �Զ�\"", curSPPara.nScaleX);
	}
	else
	{
	   sprintf(t1,  "t1.txt=\"%d%% �̶�\"", curSPPara.nScaleX);
	}
	if(curSPPara.bIsXYScaleSet) {sprintf(t1,  "t1.txt=\"%d%% %d%%\"", curSPPara.nScaleX, curSPPara.nScaleY);}
	ComWriteCmd(t1);
	char t2[32] = { 0 }; sprintf(t2, "t2.txt=\"%s\"", curSPPara.szQuality);
	ComWriteCmd(t2);
	char t3[32] = { 0 };
	if (curSPPara.bAutoTint)
	{
		ComWriteCmd("vis h0,0");
		sprintf(t3, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
	}
	else
	{
	    sprintf(t3, "h0.val=%d", curSPPara.nTint);
		ComWriteCmd(t3);
		ComWriteCmd("vis h0,1");
	}
	DrawUIStatusPics();
	ComWriteCmd(szReadyT4);
}

void drawTINT()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page density");
	}
	//ѡ��
	DrawSelectItem();
	// �ֶ�
	if (WndList[TINTIDX].cur_sel > 0) // 3 ���ֶ������������ֶ�Ũ����.
	{
		if(debug) printf("%d\n",curLRIdx);
		WR_TINT(curLRIdx);
		char t3[32] = { 0 }; snprintf(t3, 32, "h0.val=%d", curLRIdx);
		ComWriteCmd(t3);
	}
}
// �ֶ���ֽ��ֽ��׼�����ˣ������棻���򷵻ؼ١�
int handpaperfeeder_avaliable()
{
      if(debug) printf("handpaper_avaliable %02x\n",  _state.status[1] );
      return !(_state.status[1]  & (1 <<  6));
}
// �ֶ�ѡֽʱ�����µ�ǰ���ֶ�ֽ������(0x38, 0x33,0x37��)��
int HandPaperMatch(int nPaperSz)  // �Ƿ�ɺ�����Щ���߶Դ�ӡͷ(||)���������Ϊ��ӡ���������Ƕ̱���Դ�ӡͷ��
{
     int  nhandPaper = 0;  //A4
     switch(nHandPaperSz)
     {
         case 0x33:                                             // A4  ||
         case 0x18:
         case 0x38:  nhandPaper =  9;  break;    // A4 ==
         case 0x37:  nhandPaper =  8;  break;    // A3
         case 0x16:                                             // B5 16K ||
         case 0x10:  nhandPaper = 13;  break;    // B5 == , 16K==
         case 0x1F:  nhandPaper = 12 ; break;   // B4== 8K ==
     }
     return  nhandPaper == nPaperSz;
}

int checkPaperBox1Paper()//�Ĵ������ֽ��ֽ��
{
    //  1 ֽ����Դ��ʾֽ�У� 0  ��ʾ�ֶ�, 1  ֽ��
    unsigned short rval = get_reg(STATUS_PAPERSZ_REG);
    nBoxPaperSz = (rval & 0x3F);                // Status_reg�ĵͣ�λΪbox paper size  // ����ֽ�Ŵ���:  A3: 0x7  A4 Horizontal: 0x8, A4 Vertical: 0x3
	//printf("nBoxPaperSz= %x\n",nBoxPaperSz);
    switch (nBoxPaperSz)
    {
                case 0x33: sprintf(szBoxPaper, "A4||"); break;
                case 0x18:
                case 0x38: sprintf(szBoxPaper, "A4=="); break;
                case 0x3F:
                case 0x37: sprintf(szBoxPaper, "A3");   break;
                case 0x10:
                case 0x30: sprintf(szBoxPaper, "B5_16K==");  break;
                case 0x1F: sprintf(szBoxPaper, "B4_8K==");  break;
                case 0x16:
                case 0x36: sprintf(szBoxPaper, "B5_16K||"); break;
                default:   sprintf(szBoxPaper,"");break;
     }
    return nBoxPaperSz;
}

void checkszBoxPaper()
{
		nBoxPaperSz = checkPaperBox1Paper();
		switch (nBoxPaperSz)
		{
			case 0x33: sprintf(szBoxPaper, "A4||"); break;
			case 0x38:
			case 0x18:  sprintf(szBoxPaper, "A4=="); break;
			case 0x3F:
			case 0x37:  sprintf(szBoxPaper, "A3");   break;
			case 0x10: sprintf(szBoxPaper,"B5_16K==");  break;
			case 0x1F: sprintf(szBoxPaper, "B4_8K==");  break;
			case 0x16: sprintf(szBoxPaper, "B5_16K||"); break;
		}
		sprintf(curSPPara.szPaper, "%s", szBoxPaper);
		//return szBoxPaper;

}

void drawPAPER()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page paper");
	}
	char t1[32] = {0};       sprintf(t1, "b1.txt=\" ֽ��1: %s\"", szBoxPaper);       ComWriteCmd(t1);
	char t2[32] = {0};       sprintf(t2, "b2.txt=\" �ֶ� : %s\"",  szHandPaper);     ComWriteCmd(t2);
	DrawSelectItem();
}

void drawPAPERBOX()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page paperbox1");
	}
	DrawSelectItem();
}

void drawPAPERSZ()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page papersize");
	}
	DrawSelectItem();
}
void drawPAPERTYPE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page papertype");
	}
	DrawSelectItem();
}
void drawSCALE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page zoom");
	}
	DrawSelectItem();
}
void drawSCALEAUTO()
{
}
void drawSCALEFIX()
{
	int id_screen = WndList[SCALEFIXIDX].cur_sel / LstItemsPerScreen;
	int id_lastscr = WndList[SCALEFIXIDX].last_sel / LstItemsPerScreen;
	if(id_screen != id_lastscr || curStage != lastStage)
	{
		char page[32] = { 0 }; snprintf(page, 32, "page zoomf%d", id_screen);		ComWriteCmd(page);
	}
	DrawSelectItem();
}
void drawSCALEMANUAL()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page zoomhand");
	}
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Escale.text));
	ComWriteCmd(n0);
}
void drawQUALITY()
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page quality");
	}
	DrawSelectItem();
}
void drawFUNC()
{
	int id_screen = WndList[FUNCIDX].cur_sel / LstItemsPerScreen;
	int id_lastscr = WndList[FUNCIDX].last_sel / LstItemsPerScreen;
	if(id_screen != id_lastscr || curStage != lastStage)
	{
		char page[32] = { 0 }; snprintf(page, 32, "page function%d", id_screen);		ComWriteCmd(page);
	}
	DrawSelectItem();
}
void drawERASE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page eraseborder");
	}
	DrawSelectItem();
}
void drawERASESETMM()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page erasesetmm");
	}
	char* Title[3] = { "t0.txt=\"�������ã���\"", "t0.txt=\"�������ã��ϣ�\"", "t0.txt=\"�������ã��߿�\"" };
	ComWriteCmd(Title[WndList[ERASEIDX].cur_sel - 1]);
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Eeraseset.text));
	ComWriteCmd(n0);
}
void drawPAGEMARGIN()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pagemargin");
	}
	DrawSelectItem();
}
void drawMARGINSETMM()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pmsetmm");
	}
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Emarginset.text));
	ComWriteCmd(n0);
}
void drawXYSCALE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page xyscale");
	}
	DrawSelectItem();
}
void drawXYSCALESET()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page xyscaleset");
	}
	if (var_x_y % 2 == 0)
	{   ComWriteCmd("t0.txt=\"<\""); ComWriteCmd("t1.txt=\"\"");   }
	else if (var_x_y % 2 == 1)
	{   ComWriteCmd("t0.txt=\"\"");  ComWriteCmd("t1.txt=\"<\"");  }

	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Exyscalesetx.text));
	ComWriteCmd(n0);
	char n1[32] = { 0 }; snprintf(n1, 32, "n1.val=%d", atoi(Exyscalesety.text));
	ComWriteCmd(n1);
}

void drawNEGTIVE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page negtive");
	}
	DrawSelectItem();
}
void drawBOOKSEP()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page booksep");
	}
	DrawSelectItem();
}

void drawSTAMP()
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page stamp");
	}
	DrawSelectItem();
}

void drawPAGEFORMAT()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pageformat");
	}
	DrawSelectItem();
}

void drawPZCL()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page pzcl");
	}
	DrawSelectItem();
}

void drawHBYG()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page hbyg");
	}
	DrawSelectItem();
}

void draw4To1()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page _4to1");
	}
	DrawSelectItem();
}

void drawYGFX()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page ygfx");
	}
	DrawSelectItem();
}

void drawSCANNING()
{
    if(debug) printf("DrawScanning\n");
	if (curStage != lastStage)
	{
		ComWriteCmd("page main");
	}
	char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);
	ComWriteCmd(t0);
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Eready.text));
	ComWriteCmd(n0);
	char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);
	ComWriteCmd(t1);
	char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"", curSPPara.szQuality);
	ComWriteCmd(t2);
	char t3[32] = { 0 };
	if (curSPPara.bAutoTint)
	{
		ComWriteCmd("vis h0,0");
		snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
	}
	else
	{
	    snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
		ComWriteCmd(t3);
		ComWriteCmd("vis h0,1");
	}
	DrawUIStatusPics();
	ComWriteCmd("t4.txt=\"\"");
}
//�ظ�ɨ��ʱ�Ľ���
void drawREPEAT_SCANING()
{
    if(debug) printf("DrawRepeat_Scaning\n");
	if (curStage != lastStage)
	{
		ComWriteCmd("page main");
	}
	checkszBoxPaper();
	curSPPara.nScaleX  = 100;
	sprintf(curSPPara.szQuality, "�ı�/��Ƭ");
	char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);//�жϵ���ֽ���е�ֽ�Ŵ�С
	ComWriteCmd(t0);
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Eready.text));//��ӡ����
	ComWriteCmd(n0);
	char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);//�����ݿ����ֽ�Ŵ�С�����бȶԱ���
	ComWriteCmd(t1);
	char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"", curSPPara.szQuality);//Ĭ��Ϊ0 �ı�/��Ƭ
	ComWriteCmd(t2);
	char t3[32] = { 0 };
	if (curSPPara.bAutoTint)
	{
		ComWriteCmd("vis h0,0");
		snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
	}
	else
	{
	    snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
		ComWriteCmd(t3);
		ComWriteCmd("vis h0,1");
	}
	DrawUIStatusPics();
	ComWriteCmd("t4.txt=\"\"");
}

void drawNEXTSCAN()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page scannext");
	}
	char t0[32] = { 0 };  snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);
	ComWriteCmd(t0);
	char n0[32] = { 0 };  snprintf(n0, 32, "n0.val=%d", atoi(Eready.text));
	ComWriteCmd(n0);
	char t1[32] = { 0 };  snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);
	ComWriteCmd(t1);
	char t3[32] = { 0 };  snprintf(t3, 32, "t3.txt=\"%d\"", avaliableFrames);
	ComWriteCmd(t3);
	char t4[32] = { 0 };  snprintf(t4, 32, "t4.txt=\"ɨ��OK/��ӡ��ʼ %d%\"", GetMemLost());
	ComWriteCmd(t4);
	DrawUIStatusPics();
}

void drawHANDPAPER()
{
	int id_screen = WndList[HANDPAPERIDX].cur_sel / LstItemsPerScreen;
	int id_lastscr = WndList[HANDPAPERIDX].last_sel / LstItemsPerScreen;
	if(id_screen != id_lastscr || curStage != lastStage)
	{
		char page[32] = { 0 }; sprintf(page,  "page handpaper%d", id_screen);		ComWriteCmd(page);
	}
 	DrawSelectItem();
}

void drawPRINTING()
{
        if (curStage != lastStage)
        {
            ComWriteCmd("page main");
        }
        char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);
        ComWriteCmd(t0);
        if(debug) printf("---------curSPPara.szPaper : %s-----\n",curSPPara.szPaper);
        char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d:%d", _state.printed_count, _state.nPageCount);
        ComWriteCmd(n0);
        if(debug) printf("-----------_state.printed_count : %d , _state.nPageCount : %d\n----------",_state.printed_count , _state.nPageCount);
        char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);
        ComWriteCmd(t1);
        if(debug) printf("-------------curSPPara.nScaleX :%d \n--------",curSPPara.nScaleX);
        char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"", curSPPara.szQuality);
        ComWriteCmd(t2);
        if(debug) printf("-------------curSPPara.szQuality :%s \n--------",curSPPara.szQuality);
        char t3[32] = { 0 };
        //Ũ��
        char t5[32] = { 0 }; snprintf(t5, 32, "t5.txt=\"%s\"","Ũ��");
		ComWriteCmd(t5);


        if(debug) printf("-------------Concentration :%0.4f \n--------",Concentration/4096.0*3.3);
        char t6[32] = { 0 }; snprintf(t6, 32, "t6.txt=\"%0.4f\"", Concentration/4096.0*3.3);
		ComWriteCmd(t6);

        if (curSPPara.bAutoTint)
        {
            ComWriteCmd("vis h0,0");
            snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
        }
        else
        {
            snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
            ComWriteCmd(t3);
            ComWriteCmd("vis h0,1");
        }

        ComWriteCmd("t4.txt=\">���ڴ�ӡ\"");
}
//�����ظ���ӡ��
void drawREPEAT_PRINTING()
{
		if (curStage != lastStage)
        {
            ComWriteCmd("page main");
        }

		checkszBoxPaper();
        curSPPara.nScaleX  = 100;
		sprintf(curSPPara.szQuality, "�ı�/��Ƭ");
        char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);//ֽ������
        ComWriteCmd(t0);
        char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d:%d", 1,1);//ֽ������Ӧ��Ĭ��Ϊ1
        ComWriteCmd(n0);
        char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);//ֽ�����ű���
        ComWriteCmd(t1);
        char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"",  curSPPara.szQuality);//Ĭ��Ϊ0 �ı�/��Ƭ
        ComWriteCmd(t2);
        char t3[32] = { 0 };
        if (curSPPara.bAutoTint)
        {
            ComWriteCmd("vis h0,0");
            snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
        }
        else
        {
            snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
            ComWriteCmd(t3);
            ComWriteCmd("vis h0,1");
        }

        ComWriteCmd("t4.txt=\">�����ظ���ӡ\"");
}


void drawSING_PING()
{
   if(debug) printf("DrawFingerLogin UpdateUI is %d\n", UpdateUI);
   if (curStage != lastStage)
	{
		ComWriteCmd("page main");
	}
	char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);
	ComWriteCmd(t0);
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", atoi(Eready.text));
	ComWriteCmd(n0);
	char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);
	if(curSPPara.bIsXYScaleSet) {sprintf(t1,  "t1.txt=\"%d%% %d%%\"", curSPPara.nScaleX, curSPPara.nScaleY);}
	ComWriteCmd(t1);
	char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"", curSPPara.szQuality);
	ComWriteCmd(t2);
	char t3[32] = { 0 };
	if (curSPPara.bAutoTint)
	{
		ComWriteCmd("vis h0,0");
		snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
	}
	else
	{
	    snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
		ComWriteCmd(t3);
		ComWriteCmd("vis h0,1");
	}
	DrawUIStatusPics();
	//char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"���ڸ�ӡ: %d\"",  _state.printed_count+1);
	char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"���ڸ�ӡ\"");
	ComWriteCmd(t4);
}


//�����ظ���ӡ��
void drawREPEAT_SCANPRINTING()
{

	if (curStage != lastStage)
	{
		ComWriteCmd("page main");
	}

	checkszBoxPaper();
	curSPPara.nScaleX  = 100;
	sprintf(curSPPara.szQuality, "�ı�/��Ƭ");

	char t0[32] = { 0 }; snprintf(t0, 32, "t0.txt=\"%s\"", curSPPara.szPaper);	//ֽ���е�ֽ�Ŵ�С
	ComWriteCmd(t0);
	char n0[32] = { 0 }; snprintf(n0, 32, "n0.val=%d", 1);	//��ӡ����
	ComWriteCmd(n0);
	char t1[32] = { 0 }; snprintf(t1, 32, "t1.txt=\"%d%%\"", curSPPara.nScaleX);	//���ű���
	if(curSPPara.bIsXYScaleSet) {sprintf(t1,  "t1.txt=\"%d%% %d%%\"", curSPPara.nScaleX, curSPPara.nScaleY);}
	ComWriteCmd(t1);
	char t2[32] = { 0 }; snprintf(t2, 32, "t2.txt=\"%s\"", curSPPara.szQuality); //Ĭ��Ϊ0 �ı�/��Ƭ
	ComWriteCmd(t2);
	char t3[32] = { 0 };
	if (curSPPara.bAutoTint)
	{
		ComWriteCmd("vis h0,0");
		snprintf(t3, 32, "t3.txt=\"%s\"", "�Զ�"); ComWriteCmd(t3);
	}
	else
	{
	    snprintf(t3, 32, "h0.val=%d", curSPPara.nTint);
		ComWriteCmd(t3);
		ComWriteCmd("vis h0,1");
	}
	DrawUIStatusPics();
	//char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"���ڸ�ӡ: %d\"",  _state.printed_count+1);
	char t4[32] = { 0 }; snprintf(t4, 32, "t4.txt=\"�����ظ���ӡ\"");
	ComWriteCmd(t4);
}

void drawBUSY()
{
}

void drawSQREN()  // ȷ��
{
     if (curStage != lastStage)
	{
		ComWriteCmd("page queren");
	}
	char t0[32] = { 0 }; sprintf(t0,   "t0.txt=\"����: %d\"", AllKindPages.total);
	ComWriteCmd(t0);
	char t1[32] = { 0 }; sprintf(t1,   "t1.txt=\"��ӡ: %d\"", AllKindPages.printed);
	ComWriteCmd(t1);
	char t2[32] = { 0 }; sprintf(t2,   "t2.txt=\"��ӡ: %d\"", AllKindPages.duplicated);
	ComWriteCmd(t2);
	char t3[32] = { 0 }; sprintf(t3,   "t3.txt=\"ɨ��: %d\"", AllKindPages.scaned);
	ComWriteCmd(t3);
	DrawSelectItem();
}
//�ظ���ӡ��ѡ��
void drawREPEAT()
{
    if (curStage != lastStage)
    {
        ComWriteCmd("page repeat_print");
    }
    DrawSelectItem();
}

void drawSXIAOYONG() // Ч��
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page xiaoyong");
	}
	DrawSelectItem();
}
void drawSMSJY()  // ģʽ����
{
   if (curStage != lastStage)
	{
		ComWriteCmd("page msjy");
	}
	DrawSelectItem();
}

void drawPANEL_RESET_TIME_SET()
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page panelreset");
	}
	DrawSelectItem();
}
void drawSLEEP_TIME_SET()
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page sleeptime");
	}
	DrawSelectItem();
}
void drawBKPAPER_SET()
{
    if (curStage != lastStage)
	{
		ComWriteCmd("page bkpaperset");
	}
	DrawSelectItem();
}
#include "warning.h"

void drawWARNING()
{
        if (curStage != lastStage)
        {
            ComWriteCmd("page warning");
        }

        char str[1024] = {0},  t1[1024] = {0};
        for(int i = 0;  i < W1;  i++)
        {
             int id =  _state.status[0]  & (0x0001 <<  i);
             if(id)
             {
                   sprintf(str + strlen(str), "%s ",  warning_1[W1 -1 - i].warning_str);  goto TAG;
             }
        }
        for(int  i = 0;  i  < 8 ; i++)//����û����W2����Ϊ�����̼�ۼ����þ����۵���ʾ,������warring״̬
        {
             int id =  _state.status[1]  & (1 <<  i);
             if(id)
             {
                   sprintf(str + strlen(str), "%s ",  warning_2[W2- 1 - i].warning_str);  goto TAG;
             }
        }

      TAG:     snprintf(t1, 1024, "t1.txt=\"%s\"",  str);

      ComWriteCmd(t1);
}

void drawReadyToSP()
{
     sprintf(szReadyT4,"t4.txt=\">׼����ӡ   UID:%d\"",  curSPPara.nUserID);
}

void drawIDnotMatch()
{
     ComWriteCmd("t4.txt=\">�û�ID��ƥ��\"");
    // sprintf(szReadyT4,"%s","t4.txt=\">�û�ID��ƥ��\"");
}

void drawTonerWillEmpty()
{
     sprintf(szReadyT4,"t4.txt=\">̼�ۼ����ľ�\"");
}
void drawMainFlash()
{
     sprintf(szReadyT4,"t4.txt=\">׼����ӡ\"");
}
void drawTonerRecover()
{
     ComWriteCmd("t4.txt=\">׼����ӡ\"");
}
void drawAAddToner()
{
     ComWriteCmd("t4.txt=\">����A̼����\"");
}
void drawBAddToner()
{
     ComWriteCmd("t4.txt=\">����B̼����\"");
}

void drawIDnotHavePrintPermission()
{
     ComWriteCmd("t4.txt=\">�޴�ӡȨ��\"");
}

void drawIDnotHaveScanPermission()
{
     ComWriteCmd("t4.txt=\">��ɨ��Ȩ��\"");
}

void drawComAndPageRun()
{
     ComWriteCmd("t4.txt=\">���ڸ�ӡ\"");
}

void drawMemUsed()
{
	int mem = GetMemLost();
	char t0[32] = { 0 }; sprintf(t0,  "t4.txt=\"ʣ��ռ�: %d %\"",  mem);
	ComWriteCmd(t0);
}

void drawScanPaperNotMatch()
{
     if(curSPPara.nPaperBox == 1)
        sprintf(szReadyT4,"%s","t4.txt=\">ֽ��ֽ�Ų�ƥ��\"");
	 else if(curSPPara.nPaperBox == 2)
	    sprintf(szReadyT4,"%s","t4.txt=\">�ֶ�ֽ�Ų�ƥ��\"");
}
void drawSETWN()//�¶Ⱥ�Ũ������
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page set_wn");
	}
	if (var_x_y % 3 == 0)
	{   ComWriteCmd("t3.txt=\"<\""); ComWriteCmd("t4.txt=\"\"");   ComWriteCmd("t5.txt=\"\""); }//�¶�1
	else if (var_x_y % 3 == 1)
	{   ComWriteCmd("t3.txt=\"\"");  ComWriteCmd("t4.txt=\"<\"");  ComWriteCmd("t5.txt=\"\""); }//�¶�2
	else
	{   ComWriteCmd("t3.txt=\"\"");  ComWriteCmd("t4.txt=\"\"");   ComWriteCmd("t5.txt=\"<\"");}//Ũ��

	char tem[32] = { 0 };
	sprintf(tem,  "t0.txt=\"%s\"", ETem1.text); ComWriteCmd(tem);
	sprintf(tem,  "t1.txt=\"%s\"", ETem2.text);	ComWriteCmd(tem);
	sprintf(tem,  "t2.txt=\"%s\"", ETint.text);	ComWriteCmd(tem);
	sprintf(szBottomInfo,"%d,%d,%d",SetofWN.tem1,SetofWN.tem2,SetofWN.gain);//��ʾ��ǰ���ݿ��б��������
	char info[64] = { 0 }; sprintf(info, "t6.txt=\"%s\"", szBottomInfo);
	ComWriteCmd(info);
}

void drawMODE()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page set_mode");
	}
	DrawSelectItem();
}

void drawTCRWAIT()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page tcrwait");
	}
}

void drawTCROK()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page tcrok");
	}
	//unsigned short tcr = get_reg(0x25);
	char n0[32] = { 0 }; sprintf(n0, "n0.val=%d", SetofWN.gain);
	ComWriteCmd(n0);

}

void drawFIX()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page fix");
	}
	DrawSelectItem();
}

void drawGAIN()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page gain");
	}
	char tem[32] = { 0 };
	sprintf(tem,  "n0.val=%d", atoi(EGain.text)); ComWriteCmd(tem);
	//sprintf(szBottomInfo,"%d,%d,%d",SetofWN.tem1,SetofWN.tem2,SetofWN.tint);//��ʾ��ǰ���ݿ��б��������
	sprintf(szBottomInfo,"%d",SetofWN.gain);//��ʾ��ǰ���ݿ��б��������
	char info[32] = { 0 }; sprintf(info, "t0.txt=\"��ǰ����ֵ%s\"", szBottomInfo);
	ComWriteCmd(info);
}
void drawTEM()
{
	int id_screen = WndList[TEMIDX].cur_sel / LstItemsPerScreen;
	int id_lastscr = WndList[TEMIDX].last_sel / LstItemsPerScreen;
	if(id_screen != id_lastscr || curStage != lastStage)
	{
		char tem[32] = { 0 }; snprintf(tem, 32, "page tem%d", id_screen);		ComWriteCmd(tem);
	}
	DrawSelectItem();
}

void TCR_ADJUST()
{
	unsigned short CmdTCRVal = 0;
	double x1, x2, x3 = 0;
	unsigned short TCR_Avg = 0;
	CmdTCRVal |= 0x01;
	set_reg(0x2c,CmdTCRVal);//��ʼ�ź�
	CmdTCRVal = get_reg(0x2c);
	while(!(CmdTCRVal&0x0001)) //bit 0 Ϊ1  ��ʾtcr�������
	{
		sleep(1);
		CmdTCRVal = get_reg(0x2c);
		if(debug) printf("CmdTCLval = %x\n",CmdTCRVal);
	}
	TCR_Avg = (CmdTCRVal>>4) & 0xfff; //ȡbit15-bit4
	if(debug) printf("TCR_AVG = %x\n",TCR_Avg);
	x1 = ((double)TCR_Avg / 4095) * 3.3;
	if(debug) printf("x1 = %f\n",x1);
	x2 = (1.58 - x1) * 90 + 140;
	if(debug) printf("x2 = %f\n",x2);
	x3 = (x2 * 4095) / 256;
	if(debug) printf("x3 = %f,%d\n",x3,(unsigned short)x3);
	set_reg(0x25,(unsigned short)x3);
	set_reg(0x2c,0x03);
	SetofWN.gain = x3;
	WR_GAIN(SetofWN);
	sleep(3);
	if(debug) printf("tcr done CmdTCLval = %x\n",CmdTCRVal);
	set_reg(0x2c,0x00);  //�Ĵ������
}
void drawPageChange()
{
	char t4[32] = { 0 };
	//����ʾ̼�ۼ����þ�����ʾ���ڶ�����Ϣ
	 if(curStage == SING_PING &&!(_state.status[1]&0x100)) snprintf(t4, 32, "t4.txt=\"���ڸ�ӡ %d\"",  _state.printed_count+1);
	 //if(curStage == PRINTING  &&!(_state.status[1]&0x100)) snprintf(t4, 32, "t4.txt=\"���ڴ�ӡ %d\"",  _state.printed_count+1);
	 //if(curStage == SING_PING){ snprintf(t4, 32, "t4.txt=\"���ڸ�ӡ: %d\"",  _state.printed_count+1);}
	 //if(curStage == PRINTING){  snprintf(t4, 32, "t4.txt=\"���ڴ�ӡ: %d\"",  _state.printed_count+1);}
	 ComWriteCmd(t4);
}

void drawMYSQLERROR()
{
	if (curStage != lastStage)
	{
		ComWriteCmd("page mysqlerror");
	}
}
