#ifndef  _SCANPRINT_
#define  _SCANPRINT_
#include "image.h"

void* ScanPrint(void*);
void* RepeatScanPrint(void*);
void* ScanPageSizeCheck(void* pData);

void  InitScanPos();
void StepTo(int YPos);
//　扫描到某处
GImage* DoAScan(int nColorMode);
void DoScanAndSent(int nColorMode,  int  sockfd);
void UpdateScanRegion(myRect  &rt);
void ScannerCalibrationAfterPosZeroFind();

enum { B5W=4300,  K16W = 4606,  A4W=4960,  B4W = 6070,    B5H = 6070,   K8W= 6378,   K16H = 6378,  A4H=7016,   A3W=7016 , B4H=8598,  K8H = 9212,  A3H=9920};
#endif // _SCANPRINT_
