#ifndef _SCANPRINTTYPES_
#define _SCANPRINTTYPES_

typedef struct PrnPara
{
	unsigned short nPaperSrc;			   //  1 - by hand�� 0 paper box
    unsigned short nPaperSize;             // ֽ�ųߴ�
    unsigned short nLines;				       // lines of one page data
    unsigned short nColumns;                // cols of one page data
    unsigned short nRlc;                       //  Դ�����Ƿ�ѹ������ 0 δѹ�������ݣ� 1 ѹ����������
    unsigned short nSingle_Double;      //  ����˫��
    unsigned short nPaperType;            //  ֽ�����ͣ�
    unsigned short nColorMode;           //  ��ɫģʽ 1���ڰף� -4 ��ɫ
    unsigned short nCopies;                  // ��ӡ����
    unsigned short nResolution;            //  �ֱ���
    unsigned short nPrnCombineMode; // ��;4��1
    unsigned short nCopyTrue;             //  �Ƿ���ݴ�ӡ
    volatile unsigned short nUserId;                 // ��ӡ��ID.
}PrnPara;

typedef struct ScanPrnPara
{
  	int nWidth;                              //ͼ��������ص���(�����֤һ��Ϊż��)
	int nHeight;                              //ͼ��߶�
	int nPaperBox;                       // ֽ�����;  0 �Զ�����ֽ�У����ֶ�
	int nPaperType;                      // ֽ������
	char szPaperType[32];            // ֽ������
	int nPaperSize;                        // ֽ�Ŵ�С  �����룿����ֽ�л����ֶ�ֽ��
	char szPaper[32];                    // ֽ������
	int nCopy;                                //��ӡ����
	int nScaleX;                            //X����ֵ%
	int nScaleY;                             //Y����ֵ%;
    unsigned char bAutoScale;      //�Ƿ��Զ�����
    int nQuality;                                //������ [0�ı�/��Ƭ] [1�ı�] [2��Ƭ],
	char szQuality[16];                      //�����ı�;
//	unsigned char bAutoQuality;          //�Ƿ��Զ�����
	int nTint;                                       //Ũ��  0~9 10���� 5 Ϊ �м� 50% ��
	unsigned char bAutoTint;              //�Ƿ��Զ�Ũ��
	unsigned char nEraseBorder;         //���ߣ�[0����]��[1 ��]�� [2 ��]�� [3 �߿�]
	unsigned char nErasemm;             //����������  mm����
	unsigned char nPageMargin;          //װ����λ��, 0��ʾ�رգ������ʾ��
	unsigned char nMaginmm;             //���õ�mm��ֵ
	unsigned char nNegtive;                  //[0 ����ת]��[1 ��ת]
	unsigned char nStamp;                   //[0 NoPageStamp], 1 ��ӡ��
	unsigned char nStampMode;           //[0 "P001,P002"],  [1 "1,2,3"]
	unsigned char bIsSFZ;                   //���֤��ӡ��
	unsigned char groups_pages;          //��ֽ����ҳ���Ƿ���(1��ʾ��ҳ(1����ҵ��3ҳ��A0B0C0 -> A1B1C1->AnBnCn)�� 0��ʾ����(A0A1...An->B0B1...Bn->C0C1Cn�� Ĭ�Ϸ���)
	unsigned char nCombineMode;         //�ϲ�ģʽ��[0�� ��]�� [1�� 2��һ]�� [2�� 4��һ]
	unsigned char n4To1;                      //4��һ��ʽ��0,[12;34], 1[13;24]
	unsigned char nBookSep;                 //�鱾����[0, ��] [1 ����] [2 ��չ]
	unsigned char nSingle_Double;        // ����˫��
	unsigned char bAutoPaper;              // �Զ�ֽ�š����ǣ���������ҪӲ�����ġ�
	volatile unsigned short  nUserID;                 //��ǰ�û�Id.
	unsigned char nColormode;              // 0 ��ɫ, 1�Ҷ�, 2 ��ֵ
	unsigned char bIsXYScaleSet;           // �Ƿ�XYSet
	unsigned short nMode; 				  //[0 ��ȫ]��[1 �ǰ�ȫ]
}ScanPrnPara;

typedef  struct SysProfile
{
      char    B_KPaper;  // K 0,  B, 1
      int     IdleSecsToSleep;   // ����ʱ��
      int     SecsPanelReset;   // ��Ļ��λʱ��
}SysProfile;

typedef struct PageSize
{
      int  hight;          // ͼ��ĸ�
      int  weight;      // ͼ��Ŀ�
      int  totalsize; // ͼ���ȫ����С
      int  ncolormode;  //0-������ʣ�1-�Ҷ�  2-��ֵ
      int  cate	;		//1����ɨ�����ݣ�2����ӡ����
}PageSize;


typedef struct AllKindsofPages
{
      int  total;          // ��ҳ��, exclude scaned
      int  printed;      // ��ӡ��
      int  duplicated; // ��ӡ��
      int  scaned;      // ɨ��ҳ����
}AllKindsofPages;

typedef struct SetDateofWN
{
      unsigned short  tem1;      // �¶�1
      unsigned short  tem2; 	  // �¶�2
      unsigned short  tint;      // Ũ�ȡ�
      unsigned short  gain;      // ����
} SetDateofWN;

// Wanbo Driver StartDataCmd
typedef struct
{
	int dwSignature;  //0x58444847
	int dwRequest;    //0-PRINT,1-SCAN
	int dwUserId;
	int Reserved[5];
}REQUEST_PACKET;

typedef struct
{
     int fd;
     int uid;
     int isUsbfd;
}PRN_SCAN_PARA;

typedef struct TwoSP
{
    ScanPrnPara sp[2];
}TwoSP;
#endif
