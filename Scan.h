#ifndef  _SCANNER_
#define _SCANNER_
void* Scan(void* data);

typedef struct ScanReq
{
    double ScanRgnLeft;
    double ScanRgnTop;
    double ScanRgnWidth;
    double ScanRgnHeight;      // cm
    int       ColorMode;             // 0 color ,  1 gray , 2 black_white  by Wanbo
    int       Resolution;
    int       Reserve[6];
}ScanReq;   // 64 bytes

typedef struct ScanRes
{
      int  status;                              // 0 sucess,  else error code;
      int ScanImageWidth;
      int ScanImageHeight;
      int BytesPerLine;
      int  nInterl_Plane;
      int  nRGB_BGR;
      int Reserve[2];
}ScanRes;   // 32 bytes

// colormode : 0 gray, 1 color by HW.
void InitScanner(unsigned short dpi,  unsigned short  colormode, int Tint);
void GetScanResponse(ScanReq &sreq,  ScanRes & sres);


#endif // _SCANNER_
