
#include "image.h"
#include "memory.h"
#include "ScanPrintTypes.h"
#include <stdlib.h>
#include "globals.h"
#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"

void _resize(GImage* src, GImage* dst)
{
	double rW = 1.0 * dst->W / src->W;
	double rH = 1.0 * dst->H / src->H;
	for (int dr = 0; dr < dst->H; dr++)
	{
		for (int dc = 0; dc < dst->W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			int sc0 = dc / rW,  sc1 = Min(sc0 + 1, src->W-1), sr0 = dr / rH, sr1 = Min(sr0 + 1,src->H-1);
			float a = dc / rW - sc0, b = dr/rH - sr0;
			dst->bits[dr*dst->W + dc] = (1-a)*(1-b)* src->bits[sr0*src->W + sc0] + a*(1-b)*src->bits[sr0*src->W + sc1] + (1-a)*b*src->bits[sr1*src->W + sc0] + (a*b)*src->bits[sr1*src->W + sc1];
		}
	}
}

void __resize(GImage* src, GImage* dst)
{
	double rW = 1.0 * dst->W / src->W;
	double rH = 1.0 * dst->H / src->H;
	float *a_arr=(float *)malloc(sizeof(float)*dst->W);
	int *sc0_arr=(int *)malloc(sizeof(int)*dst->W);
	for (int dr = 0; dr < dst->H; dr++)
	{
        int sr0=dr / rH,sr1=Min(sr0 + 1,src->H-1);;
        float b=dr/rH - sr0;

		for (int dc = 0; dc < dst->W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			if(dr==0)
           {
                sc0_arr[dc]=dc / rW;
                a_arr[dc]=dc / rW - sc0_arr[dc];
           }

			int sc0 = sc0_arr[dc],  sc1 = Min(sc0 + 1, src->W-1);
			float a = a_arr[dc];
			dst->bits[dr*dst->W + dc] = (1-a)*(1-b)* src->bits[sr0*src->W + sc0] + a*(1-b)*src->bits[sr0*src->W + sc1] + (1-a)*b*src->bits[sr1*src->W + sc0] + (a*b)*src->bits[sr1*src->W + sc1];
		}
	}
	free(a_arr);
	free(sc0_arr);
}

void resize_C(uchar* psrc, int sW, int sH, uchar *pdst,  int dW, int dH)
{
	double rW = 1.0 * dW / sW;
	double rH = 1.0 * dH / sH;
	for (int dr = 0; dr < dH; dr++)
	{
		for (int dc = 0; dc < dW; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			int sc0 = dc / rW, sc1 = Min(sc0 + 1, sW - 1), sr0 = dr / rH, sr1 = Min(sr0 + 1, sH - 1);
			float a = dc / rW - sc0, b = dr / rH - sr0;
			float sa = (1 - a)*(1 - b), sb = a*(1 - b), sc = (1 - a)*b, sd = a*b;
			int A = sr0*sW * 3 + sc0 * 3, B = sr0*sW * 3 + sc1 * 3, C = sr1*sW * 3 + sc0 * 3, D = sr1*sW * 3 + sc1 * 3;
			pdst[dr*dW * 3 + dc*3]     = sa* psrc[A]     + sb*psrc[B]     + sc*psrc[C]     + sd*psrc[D];
			pdst[dr*dW * 3 + dc*3 + 1] = sa* psrc[A + 1] + sb*psrc[B + 1] + sc*psrc[C + 1] + sd*psrc[D + 1];
			pdst[dr*dW * 3 + dc*3 + 2] = sa* psrc[A + 2] + sb*psrc[B + 2] + sc*psrc[C + 2] + sd*psrc[D + 2];
		}
	}
}

// 顺时针选旋转
void _Rotate90(GImage *pOriImg, GImage* pDst)
{
	if (pDst && pOriImg && pDst->bits && pOriImg->bits)
	{
		for (int r = 0; r < pOriImg->H; r++)  // 原图高度->目标图的宽度cols
		{
			for (int c = 0; c < pOriImg->W; c++) // 原图宽度 -> 目标图高度rows
			{
				pDst->bits[c * pOriImg->H + (pOriImg->H - 1 - r)] = pOriImg->bits[r * pOriImg->W + c];
			}
		}
	}
}


// 顺时针选旋转
void Rotate90(GImage *pOriImg, GImage* pDst)
{

	unsigned char *ps = pOriImg->bits;
	unsigned char *pd = 0;

	for (int r = 0; r < pOriImg->H; r++, pd++)      // 原图高度->目标图的宽度cols
	{
		int db = pOriImg->H - 1 - r;
		pd = pDst->bits;
		for(int c = 0; c < pOriImg->W; c++, pd += pDst->W) // 原图宽度 -> 目标图高度rows
		{
			*(pd + db) = *ps++;
		}
	}
}

// 对半调图旋转９０度 (7017 620)->(4958, 878)
void  RotateHalfTone90(GImage &src, GImage *rotated)
{
		unsigned char mask_array[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
		unsigned char *ps = src.bits;
		for(int r = 0; r < src.H; r++) // 7017
		{
			int db = (src.H-1-r)/8;
			int db_bit = (src.H-1-r)%8;
			unsigned char mask = mask_array[db_bit];
			unsigned char *pd   = rotated->bits;
			for(int b = 0, c = 0;   b < src.W;   b++, c += 8)  // 620
			{
				if(*ps)
				{
					unsigned char *pp = pd;
					for(int cb = 0;  cb < 8;  cb++)
					{
						if(*ps & mask_array[cb])
						{      //unsigned char *pd=rotated->bits+rotated->W*(c+cb);
								*(pp+db) |= mask;
						}
						pp += rotated->W;
					}
				}
				ps++;
				pd+=rotated->W * 8;
			}
		}
}

// 逆时针选旋转
void Rotate90CCW(GImage *pOriImg, GImage* pDst)
{
	if (pDst && pOriImg && pDst->bits && pOriImg->bits)
	{
		for (int r = 0; r < pOriImg->H; r++)  // 原图高度->目标图的宽度cols
		{
			for (int c = 0; c < pOriImg->W; c++) // 原图宽度 -> 目标图高度rows
			{
				pDst->bits[(pOriImg->W -1 - c) * pOriImg->H + r] = pOriImg->bits[r * pOriImg->W + c];
			}
		}
	}
}
// 逆时针旋转
void Rotate90_ColorCCW(uchar *pOriImg, uchar* pDst, int sW, int sH)
{
	if (pDst && pOriImg)
	{
		for (int r = 0; r < sH; r++)  // 原图高度->目标图的宽度cols
		{
			for (int c = 0; c < sW; c++) // 原图宽度 -> 目标图高度rows
			{
				pDst[(sW - 1 - c)* sH * 3 + r * 3]       = pOriImg[r * sW * 3 + c * 3];
				pDst[(sW - 1 - c)* sH * 3 + r * 3 + 1] = pOriImg[r * sW * 3 + c * 3 + 1];
				pDst[(sW - 1 - c)* sH * 3 + r * 3 + 2] = pOriImg[r * sW * 3 + c * 3 + 2];
			}
		}
	}
}

// 顺时针旋转
void Rotate90_Color(uchar *pOriImg, uchar* pDst, int sW, int sH)
{
	if (pDst && pOriImg)
	{
		for (int r = 0; r < sH; r++)  // 原图高度->目标图的宽度cols
		{
			for (int c = 0; c < sW; c++) // 原图宽度 -> 目标图高度rows
			{
				pDst[c* sH * 3 + (sH - 1 - r) * 3]      = pOriImg[r * sW * 3 + c * 3];
				pDst[c* sH * 3 + (sH - 1 - r) * 3 + 1] = pOriImg[r * sW * 3 + c * 3 + 1];
				pDst[c* sH * 3 + (sH - 1 - r) * 3 + 2] = pOriImg[r * sW * 3 + c * 3 + 2];
			}
		}
	}
}

uchar BayerT[64] = { 0, 32, 8,  40, 2,  34, 10, 42,
                     48,16, 56, 24, 50, 18, 58, 26,
                     12,14, 4,  36, 14, 46, 6,  38,
					 60,28, 52, 20, 60, 30, 54, 22,
					 3, 35, 11, 43, 1,  33, 9,  41,
					 51,19, 59, 27, 49, 17, 57, 25,
					 15,47, 7,  39, 13, 45, 5,  37,
					 63,31, 55, 23, 61, 29, 53, 21
                    };

void HalfToneB(GImage *pSrc, GImage *pDst)
{
	uchar *pt = pDst->bits, *pS = pSrc->bits;

	for (int r = 0; r < pSrc->H ; r++)
    {
                register uchar rid = r % 8;
                for (int c = 0; c < pSrc->W; c++)
                {
                    register uchar cid = c % 8;
                    uchar psc = curSPPara.nNegtive ? 255 - pS[c] : pS[c];
                    if ((psc >> 2) < BayerT[(rid << 3) + cid])
                    {
                        pt[c / 8] |= (0x80  >> cid );
                    }
                }
                pS += pSrc->W;     pt += pDst->W;
    }
}
#define  HT 127
void _HalfToneB(GImage *pSrc, GImage *pDst)
{
        uchar *pt = pDst->bits , *pS = pSrc->bits;
		float* pTmp = new float[pSrc->W * 2], *pse = pTmp, *pjump = pse;  // pse = srcdata + error
		memset(pTmp, 0, pSrc->W * 2 * sizeof(float));

        if(curSPPara.nNegtive)
        {
                for (int r = 0;  r < pSrc->H -1;   r++)
                {
                        for (int c = 1;  c < pSrc->W-1;  c++)
                        {
                            pse[c] = pjump[c] +  pS[c] ;	 pjump[c] = 0;
                            uchar pt_c = 0;
                            if (pse[c] >HT)
                            {
                                pt[c / 8] |= (0x80 >> (c % 8));	 pt_c = 255;
                                float err = pse[c] - pt_c;
                                pse[c + 1] += err * 0.4375;  pse[c + pSrc->W - 1] += err * 0.1875;	pse[c + pSrc->W] += err * 0.3125;  pse[c + pSrc->W + 1] += err * 0.0625; // 为了加速，只处理黑色信号
                           }
                        }
                    pjump = pse + pSrc->W;		pS += pSrc->W;   pt += pDst->W;
                }
		}else
		{
		    for (int r = 0;  r < pSrc->H -1;   r++)
                {
                        for (int c = 1;  c < pSrc->W-1;  c++)
                        {
                            pse[c] = pjump[c] +   ( pS[c] ^ 0xff );	 pjump[c] = 0;
                            uchar pt_c = 0;
                            if (pse[c] > HT)
                            {
                                pt[c / 8] |= (0x80 >> (c % 8));	 pt_c = 255;
                                float err = pse[c] - pt_c;
                                pse[c + 1] += err * 0.4375;  pse[c + pSrc->W - 1] += err * 0.1875;	pse[c + pSrc->W] += err * 0.3125;  pse[c + pSrc->W + 1] += err * 0.0625;   // 为了加速，只处理黑色信号
                           }
                        }
                    pjump = pse + pSrc->W;		pS += pSrc->W;   pt += pDst->W;
                }
		}
		delete[] pTmp;
}

void HalfTone(GImage *pSrc, GImage *pDst)
{
    uchar uch_inv = curSPPara.nNegtive ? 0x00 : 0xff;     // 图像反转
	if (pDst && pSrc && pSrc->bits && pDst->bits)
	{
		uchar *pt = 0, *pS;
		float* pTmp = new float[pSrc->W * 2], *pse = pTmp, *pjump = pse;  // pse = srcdata + error
		memset(pTmp, 0, (pSrc->W * 2) * sizeof(float));
		for (int r = 0; r < pSrc->H - 1; r++)
		{
			pS = pSrc->bits + r * pSrc->W;   pt = pDst->bits + r *pSrc->W;
			for (int c = 0; c < pSrc->W; c++)
			{
				pse[c] = pjump[c] + ( pS[c] ^ uch_inv );	 pjump[c] = 0;
			}
			for (int c = 1; c < pSrc->W -1; c++)
			{
				pt[c] = pse[c] > 127 ? 255 : 0;
				float err = pse[c] - pt[c];
				pse[c + 1] += err * 0.4375;  pse[c + pSrc->W - 1] += err * 0.1875;	pse[c + pSrc->W] += err * 0.3125;  pse[c + pSrc->W + 1] += err * 0.0625;
			}
			pjump = pse + pSrc->W;
		}

		delete[] pTmp;
	}
}

// 获得源图像中的一块子区域位置，不分配新的内存
void  SubImage(GImage *src,   myRect  &rt,   ImgHeader  &img_header)
{
       if(rt.x < 0 ) rt.x = 0;
       if(rt.x >= src->W)  rt.x = src->W - 1;
       if(rt.y < 0)  rt.y = 0;
       if(rt.y >= src->H)   rt.y  = src->H -  1;
       if(rt.x + rt.W > src->W)  rt.W = src->W - rt.x;
       if(rt.y + rt.H  > src->H)   rt.H  = src->H - rt.y;

       img_header.H    = rt.H;
       img_header.W   = rt.W;
       img_header.step = src->W;
       img_header.bits  = src->bits + rt.y * src->W + rt.x;
}


void resize(GImage* dst, GImage* src)
{
	double rW = 1.0 * dst->W / src->W;
	double rH = 1.0 * dst->H / src->H;
	unsigned char * pd = dst->bits, *ps = src->bits;
	int *sc_array = (int *)malloc(dst->W*sizeof(int));
	for (int dc = 0; dc < dst->W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
	{
		sc_array[dc] = dc / rW;
	}
	for (int dr = 0; dr < dst->H; dr++)
	{
		int sr0 = dr / rH;
		for (int dc = 0; dc < dst->W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			int sc0 = sc_array[dc];    // dc / rW;
			*(pd + dc) = *(ps + sc0);
		}
		pd += dst->W;
		ps = src->bits + sr0*src->W;
	}
	free(sc_array);
}

// 把图像头描述的一块内存图像A,　贴到目标图B的某指定的矩形位置Rt (支持缩放)　　（不产生新的内存分配操作）
void Paste(GImage *dst, myRect rt, ImgHeader &img_header, int ortho)
{
	double rW = 1.0 * rt.W / img_header.W;
	double rH = 1.0 * rt.H / img_header.H;
	if (ortho)
	{
		rW = rH = Min(rW, rH);
		rt.H = rH * img_header.H;
		rt.W = rW * img_header.W;
	}
	unsigned char * pd = dst->bits + rt.y * dst->W + rt.x, *ps = img_header.bits;
	int *sc_array = (int *)malloc(rt.W*sizeof(int));
	for (int dc = 0; dc < rt.W; dc++)          // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
	{
		sc_array[dc] = dc / rW;
	}
	int minH = Min(rt.H, dst->H), minW = Min(rt.W, dst->W);
	for (int dr = 0; dr < minH; dr++)
	{
		int sr0 = dr / rH;
		for (int dc = 0; dc < minW; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			*(pd + dc) = *(ps + sc_array[dc]);
		}
		pd += dst->W;
		ps = img_header.bits + sr0*img_header.step;
	}
	free(sc_array);
}

// 把图像头描述的一块内存图像A,　贴到目标图B的某指定的矩形位置Rt (不支持缩放)　　（不产生新的内存分配操作）
void P(GImage *dst, myRect rt, ImgHeader &img_header)
{
	if(debug) printf("GImage dst.H:%d , dst.W:%d\n",dst->H ,dst->W);
	if(debug) printf("myRect rt.H:%d , rt.W:%d\n",rt.H ,rt.W);
	unsigned char * pd = dst->bits + rt.y * dst->W + rt.x, *ps = img_header.bits;

	for (int dr = 0; dr < rt.H; dr++)
	{
		for (int dc = 0; dc < rt.W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			pd[dc] = ps[dc];
		}
		pd += dst->W;    ps += img_header.step;
	}
}

// 把图像头描述的一块内存图像A,　顺时针旋转９０度后贴到目标图B的某指定的矩形位置Rt (不支持缩放)　（不产生新的内存分配操作）
void R90P(GImage *dst, myRect rt, ImgHeader  &img_header)
{
	unsigned char * pd = dst->bits + rt.y * dst->W + rt.x, *ps = img_header.bits + (img_header.H - 1)*img_header.step;

	for (int dr = 0; dr < rt.H; dr++)
	{
		for (int dc = 0; dc < rt.W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后最近临插值
		{
			*(pd +dc) = *(ps - dc*img_header.step + dr);
		}
		pd += dst->W;
	}
}

// 把图像头描述的一块内存图像A,　顺时针旋转９０度后贴到目标图B的某指定的矩形位置Rt (支持缩放)　（不产生新的内存分配操作）
void R90Paste(GImage *dst, myRect rt, ImgHeader  &img_header, int ortho)
{
	double rW = 1.0 * rt.W / img_header.H;
	double rH = 1.0 * rt.H / img_header.W;
	if (ortho)
	{
		rW = rH = Min(rW, rH);
		rt.H = rH * img_header.W;
		rt.W = rW * img_header.H;
	}

	unsigned char * pd = dst->bits + rt.y * dst->W + rt.x, *ps = img_header.bits + (img_header.H - 1)*img_header.step;
	int *sr_array = (int *)malloc(rt.W *sizeof(int));
	for (int dc = 0; dc < rt.W; dc++)          // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
	{
		sr_array[dc] = dc / rW;
	}

	for (int dr = 0; dr < rt.H; dr++)
	{
		int sc0 = dr / rH;
		for (int dc = 0; dc < rt.W; dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后最近临插值
		{	//dst->bits[(dr + rt.y)*dst->W + (rt.x + dc)] = img_header.bits[(img_header.H - 1 - sc0)*img_header.step + sr0];
			*(pd +dc) = *(ps - sr_array[dc]*img_header.step + sc0);
		}
		pd += dst->W;
	}
	free(sr_array);
}


// 把图像头描述的一块内存图像A,　贴到目标图B的某指定的矩形位置Rt (支持缩放)　　（不产生新的内存分配操作）
void _Paste(GImage *dst, myRect rt, ImgHeader &img_header, int ortho)
{
  double rW = 1.0 * rt.W / img_header.W;
	double rH  = 1.0 * rt.H  / img_header.H;
	if (ortho)
	{
		rW = rH = Min(rW, rH);
		rt.H = rH * img_header.H;
		rt.W = rW * img_header.W;
	}
	for (int dr = 0;  dr < rt.H;  dr++)
	{
		for (int dc = 0;  dc < rt.W;  dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			int sc0 = dc / rW,  sc1 = Min(sc0 + 1, img_header.W-1), sr0 = dr / rH, sr1 = Min(sr0 + 1, img_header.H-1);
			float a = dc / rW - sc0, b = dr/rH - sr0;
			dst->bits[(dr + rt.y)*dst->W + (rt.x +dc)] = (1-a)*(1-b) * img_header.bits[sr0*img_header.step + sc0]
                                                                            +  a*(1-b)   * img_header.bits[sr0*img_header.step + sc1]
                                                                            +  (1-a)*b   * img_header.bits[sr1*img_header.step + sc0]
                                                                            +  (a * b )   * img_header.bits[sr1*img_header.step + sc1];
		}
	}
}

// 把图像头描述的一块内存图像A,　顺时针旋转９０度后贴到目标图B的某指定的矩形位置Rt (支持缩放)　（不产生新的内存分配操作）
void _R90Paste(GImage *dst, myRect rt, ImgHeader  &img_header, int ortho)
{
    double rW = 1.0 * rt.W / img_header.H;
	double rH  = 1.0 * rt.H  / img_header.W;
	if (ortho)
	{
		rW = rH = Min(rW, rH);
		rt.H  = rH * img_header.W;
		rt.W = rW * img_header.H;
	}
	for (int dr = 0;  dr < rt.W;  dr++)
	{
		for (int dc = 0;  dc < rt.H;  dc++)   // 对于目标图中的每个点，找到器对应的原始图像四点，然后双线性插值
		{
			//for point(dc, dr), find (sc0, sr0), (sc1, sr0), (sc0, sr1), (sc1,sr1);
			int sc0 = dc / rW,  sc1 = Min(sc0 + 1, img_header.H-1), sr0 = dr / rH, sr1 = Min(sr0 + 1, img_header.W-1);
			float a = dc / rW - sc0, b = dr/rH - sr0;
			dst->bits[(dr + rt.y)*dst->W + (rt.x +dc)] = (1-a)*(1-b) * img_header.bits[(img_header.H - 1 - sc0)*img_header.step + sr0]
                                                                            +  a*(1-b)   * img_header.bits[(img_header.H - 1 - sc1)*img_header.step + sr0]
                                                                            +  (1-a)*b   * img_header.bits[(img_header.H - 1 - sc0)*img_header.step + sr1]
                                                                            +  (a * b )   * img_header.bits[(img_header.H - 1 - sc1)*img_header.step + sr1];
		}
	}
}

// 扫描图像头尾对调。在书本分离的时候好像需要这样（书本翻开面朝下，前后页面对调，后页先扫了，复印的时候要变换回来）。
void UpdownScanFlip(GImage *img)
{
       for(int r = 0;  r < img->H /2 ;  r++)
       {
                uchar* ps = img->bits + r * img->W;
                uchar *pe = img->bits + ( img->H - 1 -r) * img->W;
                for(int c = 0;  c < img->W;  c++)
                {
                     uchar  t = ps[c];   ps[c] = pe[c] ;  pe[c] = t;
                }
       }
}

// 多合1:  如果一页满，则返回1,  否则返回0. Asub 未事先变换，需要内部变换
int  N21_Tx(GImage *TheOne, GImage *ASub, CmbPara& CP)
{
	int sx_c = 0, sy_r = 0, w = 0, h = 0;  // TheOne(起点x(col), 起点y(row), 宽，高) is a Rect, Copy Sub there.
	int iPatchRows = 1, iPatchCols = 1, iPR = 0, iPC = 0;
	switch (CP.N)
	{
	case 2:;
		iPatchRows = 2; iPatchCols = 1;
		break;
	case 4:
		iPatchRows = 2; iPatchCols = 2;
		break;
	case 6:
		iPatchRows = 3; iPatchCols = 2;
		break;
	case 9:
		iPatchRows = 3; iPatchCols = 3;
		break;
	case 16:
		iPatchRows = 4; iPatchCols = 4;
		break;
	}

	if (CP.Style_R0_C1)  // 沿列行走
	{
		iPR = CP.ithPatchNow % iPatchRows;
		iPC = CP.ithPatchNow / iPatchRows;
	}
	else                // 沿行向行进
	{
		iPR = CP.ithPatchNow / iPatchCols;
		iPC = CP.ithPatchNow % iPatchCols;
	}
	int margin = Min(TheOne->H, TheOne->W) / 60;
	sx_c = TheOne->W / iPatchCols * iPC + margin;
	sy_r = TheOne->H / iPatchRows * iPR + margin;
	w =  TheOne->W / iPatchCols - 2*margin;
	h =  TheOne->H / iPatchRows - 2*margin;

	// rotate and paste and resize.
	if (CP.N == 2 || CP.N == 6)
	{
		R90Paste(TheOne, myRect(sx_c, sy_r, w, h), ASub->header, 1);
	}
	else  // direct paste
	{
		Paste(TheOne, myRect(sx_c, sy_r, w, h), ASub->header, 1);
	}

	CP.ithPatchNow++;

	return CP.ithPatchNow == CP.N;
}

void SFZ_SP(GImage* TheOne, ImgHeader &imgheader, int   ithPatchNow)
{
	int margin = Min(TheOne->W, TheOne->H) / 60;
	int sx_c = (TheOne->W / 2 - imgheader.W - margin) / 2 + ithPatchNow * TheOne->W / 2;
	int sy_r = (TheOne->H - imgheader.H - margin) / 2;
	if(debug) printf("TheOneW %d, TheOneH %d, myRect(%d, %d,%d,%d) , imgheader.step %d \n",  TheOne->W, TheOne->H,  sx_c,  sy_r, imgheader.H, imgheader.W , imgheader.step);
	P(TheOne, myRect(sx_c, sy_r, imgheader.W, imgheader.H), imgheader);
}

void Two_to_One(GImage *TheOne, ImgHeader &imgheader, int same_orientation, int ithPatchNow)
{
	int sx_c = 0, sy_r = 0, margin = Min(TheOne->W, TheOne->H) / 60;
	if (same_orientation)
	{
		int dstW = imgheader.H * curSPPara.nScaleY / 100.;  // 70%缩放的宽(左右测量为宽)
		int dstH = imgheader.W * curSPPara.nScaleX / 100.;
		if (TheOne->H > TheOne->W)        // Vertical Paper:   上下放置2幅图像
		{
			if (dstW <= TheOne->W && dstH <= TheOne->H / 2 - 2 * margin)   //按要求70%缩放后可以放下
			{
				sx_c = (TheOne->W - dstW) / 2;
				sy_r = (TheOne->H / 2 - dstH) / 2 + ithPatchNow * TheOne->H / 2;
			}
			else                                                          // 满版等比例缩放填充吧
			{
				double rW = 1.0 * (TheOne->W - 2 * margin) / imgheader.H;   // 因为会旋转，所以行宽对调了
				double rH = 1.0 * (TheOne->H / 2 - 2 * margin) / imgheader.W;
				double rmin = Min(rW, rH);
				dstH = rmin * imgheader.W;
				dstW = rmin * imgheader.H;
				sx_c = (TheOne->W - dstW) / 2;  sy_r = (TheOne->H / 2 - dstH) / 2 + ithPatchNow* TheOne->H / 2;
			}
		}
		else                              // Horizontal Paper  左右放置2幅图像
		{
			if (dstW <= TheOne->W / 2 - 2 * margin && dstH <= TheOne->H)   //按要求70%缩放后可以放下
			{
				sx_c = (TheOne->W / 2 - dstW) / 2 + ithPatchNow * TheOne->W / 2;
				sy_r = (TheOne->H - dstH) / 2;
			}
			else                                                          // 满版等比例缩放填充吧
			{
				double rW = 1.0 * (TheOne->W / 2 - 2 * margin) / imgheader.H;
				double rH = 1.0 * (TheOne->H - 2 * margin) / imgheader.W;
				double rmin = Min(rW, rH);
				dstH = rmin * imgheader.W;
				dstW = rmin * imgheader.H;
				sx_c = (TheOne->W / 2 - dstW) / 2 + ithPatchNow * TheOne->W / 2;  sy_r = (TheOne->H - dstH) / 2;
			}
		}
		R90Paste(TheOne, myRect(sx_c, sy_r, dstW, dstH), imgheader, 1);
	}
	else
	{
		int dstW = imgheader.W * curSPPara.nScaleX / 100.;  // 70%缩放的宽(左右测量为宽)
		int dstH = imgheader.H * curSPPara.nScaleY / 100.;
		if (TheOne->H > TheOne->W)        // Vertical Paper:   上下放置2幅图像
		{
			if (dstW <= TheOne->W && dstH <= TheOne->H / 2 - 2 * margin)   //按要求70%缩放后可以放下
			{
				sx_c = (TheOne->W - dstW) / 2;
				sy_r = (TheOne->H / 2 - dstH) / 2 + ithPatchNow * TheOne->H / 2;
			}
			else                                                          // 满版等比例缩放填充吧
			{
				double rW = 1.0 * (TheOne->W - 2 * margin) / imgheader.W;
				double rH = 1.0 * (TheOne->H / 2 - 2 * margin) / imgheader.H;
				double rmin = Min(rW, rH);
				dstH = rmin * imgheader.H;
				dstW = rmin * imgheader.W;
				sx_c = (TheOne->W - dstW) / 2;  sy_r = (TheOne->H / 2 - dstH) / 2 + ithPatchNow* TheOne->H / 2;
			}
		}
		else                              // Horizontal Paper  左右放置2幅图像
		{
			if (dstW <= TheOne->W / 2 - 2 * margin && dstH <= TheOne->H)   //按要求70%缩放后可以放下
			{
				sx_c = (TheOne->W / 2 - dstW) / 2 + ithPatchNow * TheOne->W / 2;
				sy_r = (TheOne->H - dstH) / 2;
			}
			else                                                          // 满版等比例缩放填充吧
			{
				double rW = 1.0 * (TheOne->W / 2 - 2 * margin) / imgheader.W;
				double rH = 1.0 * (TheOne->H - 2 * margin) / imgheader.H;
				double rmin = Min(rW, rH);
				dstH = rmin * imgheader.H;
				dstW = rmin * imgheader.W;
				sx_c = (TheOne->W / 2 - dstW) / 2 + ithPatchNow * TheOne->W / 2;  sy_r = (TheOne->H - dstH) / 2;
			}
		}
		Paste(TheOne, myRect(sx_c, sy_r, dstW, dstH), imgheader, 1);
	}
}

void Four_to_One(GImage *TheOne, ImgHeader &imgheader, int same_orientation, int ithPatchNow)
{
	int sx_c = 0, sy_r = 0, margin = Min(TheOne->W, TheOne->H) / 60;
	int iPR = 0, iPC = 0;
	if (curSPPara.n4To1)   // 沿列行走
	{
		iPR = ithPatchNow % 2;	iPC = ithPatchNow / 2;
	}
	else                // 沿行向行进
	{
		iPR = ithPatchNow / 2;	iPC = ithPatchNow % 2;
	}

	if (!same_orientation)
	{
		int dstW = imgheader.H * curSPPara.nScaleY / 100.;  // 50%缩放的宽(左右测量为宽)
		int dstH = imgheader.W * curSPPara.nScaleX / 100.;
		if (dstW <= TheOne->W / 2 - 2 * margin && dstH <= TheOne->H / 2 - 2 * margin)   //按要求50%缩放后可以放下
		{
			sx_c = (TheOne->W / 2 - dstW) / 2 + iPC * TheOne->W / 2;
			sy_r = (TheOne->H / 2 - dstH) / 2 + iPR * TheOne->H / 2;
		}
		else                                                          // 满版等比例缩放填充吧
		{
			double rW = 1.0 * (TheOne->W / 2 - 2 * margin) / imgheader.H;
			double rH = 1.0 * (TheOne->H / 2 - 2 * margin) / imgheader.W;
			double rmin = Min(rW, rH);
			dstH = rmin * imgheader.W;
			dstW = rmin * imgheader.H;
			sx_c = (TheOne->W / 2 - dstW) / 2 + iPC * TheOne->W / 2;  sy_r = (TheOne->H / 2 - dstH) / 2 + iPR* TheOne->H / 2;
		}
		R90Paste(TheOne, myRect(sx_c, sy_r, dstW, dstH), imgheader, 1);
	}
	else
	{
		int dstW = imgheader.W * curSPPara.nScaleX / 100.;  // 50%缩放的宽(左右测量为宽)
		int dstH = imgheader.H * curSPPara.nScaleY / 100.;

		if (dstW <= TheOne->W / 2 - 2 * margin && dstH <= TheOne->H / 2 - 2 * margin)   //按要求50%缩放后可以放下
		{
			sx_c = (TheOne->W / 2 - dstW) / 2 + iPC * TheOne->W / 2;
			sy_r = (TheOne->H / 2 - dstH) / 2 + iPR * TheOne->H / 2;
		}
		else                                                          // 满版等比例缩放填充吧
		{
			double rW = 1.0 * (TheOne->W / 2 - 2 * margin) / imgheader.W;
			double rH = 1.0 * (TheOne->H / 2 - 2 * margin) / imgheader.H;
			double rmin = Min(rW, rH);
			dstH = rmin * imgheader.H;
			dstW = rmin * imgheader.W;
			sx_c = (TheOne->W / 2 - dstW) / 2 + iPC * TheOne->W / 2;  sy_r = (TheOne->H / 2 - dstH) / 2 + iPR* TheOne->H / 2;
		}
		Paste(TheOne, myRect(sx_c, sy_r, dstW, dstH), imgheader, 1);
	}
}

#include "stdio.h"
void WritePrn(char* fname, uchar *pData, int aLineBytes, int lines)
{
	char cmdend[256] = "\x1b%-12345X\x1b\x45";
	char cmdOnceHeader[256] = { 0 };	sprintf(cmdOnceHeader,  "\x1b&l1H\x1b&l1X\x1b&l0S\x1b&l%dA\x1b*t%dR\x1b&a0G\x1b&l4T",  9, 600);
	char szPageHeader[256] = { 0 };      sprintf(szPageHeader,   "\x1b*r%dS\x1b*r%dT\x1b*r0A", aLineBytes*8, lines);
	char ansi_cmd[256] = { 0 }; 	            sprintf(ansi_cmd,  "\x1b*b%dW", aLineBytes);
	char szPageTailer[] = "\x1b*rC\x1b\x45";
	FILE *fp =  fopen(fname, "wb");
	if ( fp )
	{
            fwrite(cmdend, 1, strlen(cmdend),  fp);
            fwrite(cmdOnceHeader, 1, strlen(cmdOnceHeader),  fp);
            fwrite(szPageHeader,  1, strlen(szPageHeader),  fp);
            for (int i = 0; i < lines; i++)
            {
                fwrite(ansi_cmd, 1, strlen(ansi_cmd),  fp);
                fwrite(pData + i*aLineBytes, 1, aLineBytes,  fp);
            }
            fwrite(szPageTailer, 1, strlen(szPageTailer),  fp);
            fwrite(cmdend,   1,      strlen(cmdend),  fp);
            fclose(fp);
	}
}

