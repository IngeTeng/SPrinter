#ifndef _SCANPRINTTYPES_
#define _SCANPRINTTYPES_

typedef struct PrnPara
{
	unsigned short nPaperSrc;			   //  1 - by hand， 0 paper box
    unsigned short nPaperSize;             // 纸张尺寸
    unsigned short nLines;				       // lines of one page data
    unsigned short nColumns;                // cols of one page data
    unsigned short nRlc;                       //  源数据是否压缩过： 0 未压缩的数据， 1 压缩过的数据
    unsigned short nSingle_Double;      //  单面双面
    unsigned short nPaperType;            //  纸张类型：
    unsigned short nColorMode;           //  颜色模式 1：黑白， -4 彩色
    unsigned short nCopies;                  // 打印份数
    unsigned short nResolution;            //  分辨率
    unsigned short nPrnCombineMode; // 如;4合1
    unsigned short nCopyTrue;             //  是否逐份打印
    volatile unsigned short nUserId;                 // 打印者ID.
}PrnPara;

typedef struct ScanPrnPara
{
  	int nWidth;                              //图像横向像素点数(请程序保证一定为偶数)
	int nHeight;                              //图像高度
	int nPaperBox;                       // 纸盒序号;  0 自动，１纸盒，２手动
	int nPaperType;                      // 纸张类型
	char szPaperType[32];            // 纸张类型
	int nPaperSize;                        // 纸张大小  机器码？包括纸盒或者手动纸张
	char szPaper[32];                    // 纸张名称
	int nCopy;                                //复印分数
	int nScaleX;                            //X缩放值%
	int nScaleY;                             //Y缩放值%;
    unsigned char bAutoScale;      //是否自动缩放
    int nQuality;                                //质量： [0文本/照片] [1文本] [2照片],
	char szQuality[16];                      //质量文本;
//	unsigned char bAutoQuality;          //是否自动质量
	int nTint;                                       //浓度  0~9 10级， 5 为 中间 50% ？
	unsigned char bAutoTint;              //是否自动浓度
	unsigned char nEraseBorder;         //消边：[0不削]，[1 左]， [2 上]， [3 边框]
	unsigned char nErasemm;             //消边数量，  mm计算
	unsigned char nPageMargin;          //装订线位置, 0表示关闭，非零表示打开
	unsigned char nMaginmm;             //设置的mm数值
	unsigned char nNegtive;                  //[0 不反转]，[1 反转]
	unsigned char nStamp;                   //[0 NoPageStamp], 1 有印记
	unsigned char nStampMode;           //[0 "P001,P002"],  [1 "1,2,3"]
	unsigned char bIsSFZ;                   //身份证复印？
	unsigned char groups_pages;          //排纸：分页还是分组(1表示分页(1份作业有3页，A0B0C0 -> A1B1C1->AnBnCn)， 0表示分组(A0A1...An->B0B1...Bn->C0C1Cn， 默认分组)
	unsigned char nCombineMode;         //合并模式：[0： 关]， [1： 2合一]， [2： 4合一]
	unsigned char n4To1;                      //4合一样式：0,[12;34], 1[13;24]
	unsigned char nBookSep;                 //书本分离[0, 关] [1 分离] [2 伸展]
	unsigned char nSingle_Double;        // 单面双面
	unsigned char bAutoPaper;              // 自动纸张。不是Ａ４，是需要硬件检测的。
	volatile unsigned short  nUserID;                 //当前用户Id.
	unsigned char nColormode;              // 0 彩色, 1灰度, 2 二值
	unsigned char bIsXYScaleSet;           // 是否XYSet
	unsigned short nMode; 				  //[0 安全]，[1 非安全]
}ScanPrnPara;

typedef  struct SysProfile
{
      char    B_KPaper;  // K 0,  B, 1
      int     IdleSecsToSleep;   // 休眠时间
      int     SecsPanelReset;   // 屏幕复位时间
}SysProfile;

typedef struct PageSize
{
      int  hight;          // 图像的高
      int  weight;      // 图像的宽
      int  totalsize; // 图像的全部大小
      int  ncolormode;  //0-代表真彩，1-灰度  2-二值
      int  cate	;		//1代表扫描数据，2代表复印数据
}PageSize;


typedef struct AllKindsofPages
{
      int  total;          // 总页数, exclude scaned
      int  printed;      // 打印数
      int  duplicated; // 复印数
      int  scaned;      // 扫描页数　
}AllKindsofPages;

typedef struct SetDateofWN
{
      unsigned short  tem1;      // 温度1
      unsigned short  tem2; 	  // 温度2
      unsigned short  tint;      // 浓度　
      unsigned short  gain;      // 增益
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
