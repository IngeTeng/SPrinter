#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include "memory.h"

#define Min(a,b)  (((a)<(b))?(a):(b))
#define Max(a,b)  (((a)>(b))?(a):(b))

typedef unsigned char uchar;

typedef struct ImgHeader
{
      int H;
      int W;
      int step;   // 数据的真正行宽
      uchar *bits;
}ImgHeader;
// ImgHeader *p = (ImgHeader*)&GImage;    // suppose this is OK

class myRect//坐标系维护
{
     public:
      int  x;   // left
      int  y;   // top
      int  W;  // width
      int  H;  // height
      myRect(int l, int t, int w, int h):x(l), y(t), W(w), H(h){}
};

// 灰度图像定义
class GImage
{
public:
	int    H;    // rows;
	int    W;    // cols;
	uchar* bits;
	ImgHeader  header;
	char  alloc_mem;
	GImage(int h, int w, uchar bset = 0, uchar *data = 0): H(h), W(w), alloc_mem(0)
	{
         if(!data)
         {
              bits = new uchar[H * W];   memset(bits, bset, H*W);
              alloc_mem = 1;
         }else
         {
             bits = data;
         }
         header.H  = h;
         header.W = header.step = w;
         header.bits = bits;
	 }
	~GImage()
	{
         if(bits && alloc_mem)
         {
             delete[] bits;
             header.bits = bits =  0;
         }
	 }
};

typedef struct CmbPara
{
	uchar N;            // N to 1
	uchar Style_R0_C1;  // C2DArray 沿着Row水平行走 0, 沿着Col竖直行走 1
	uchar ithPatchNow;  // 当前要合并的这块子图像的序号(0~N-1)
}CmbPara;

void HalfToneB(GImage *pSrc, GImage *pDst);
void _HalfToneB(GImage *pSrc, GImage *pDst);
// 对半调图旋转９０度
void  RotateHalfTone90(GImage &img_src_half, GImage *img_rotated_half);
// 把图像头描述的一块内存图像A,　贴到目标图B的某指定的矩形位置Rt (支持缩放)　　（不产生新的内存分配操作）
void Paste(GImage *dst, myRect rt, ImgHeader &img_header, int ortho = 0);
// 把图像头描述的一块内存图像A,　贴到目标图B的某指定的矩形位置Rt (支持缩放)　　（不产生新的内存分配操作）
void P(GImage *dst, myRect rt, ImgHeader &img_header);
// 把图像头描述的一块内存图像A,　顺时针旋转９０度后贴到目标图B的某指定的矩形位置Rt (支持缩放)　（不产生新的内存分配操作）
void R90Paste(GImage *dst, myRect rt, ImgHeader  &img_header, int ortho = 0);
// 获得源图像中的一块子区域位置，不分配新的内存
void  SubImage(GImage *src,   myRect  &rt,   ImgHeader  &img_header);
// 扫描图像首尾对调
void UpdownScanFlip(GImage *img);

void Four_to_One(GImage *TheOne, ImgHeader &imgheader, int same_orientation, int ithPatchNow);
void Two_to_One(GImage *TheOne, ImgHeader &imgheader, int same_orientation, int ithPatchNow);
void SFZ_SP(GImage* TheOne, ImgHeader &imgheader, int ithPatchNow);
// 顺时针选旋转
void Rotate90(GImage *pOriImg, GImage* pDst);
void _Rotate90(GImage *pOriImg, GImage* pDst);
void resize(GImage* src, GImage *dst);
void resize1(GImage* src, GImage *dst);
#endif // IMAGE_H_INCLUDED
