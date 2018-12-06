#ifndef PRINTJOB_H_INCLUDED
#define PRINTJOB_H_INCLUDED

#include <stdio.h>
#include "Pcl.h"
#include "os_def.h"
#include "ScanPrintTypes.h"

int    ReadJobCfg(PrnPara *setting);
int    ConfirmNextPage(int* existNext);
void  PrintJobSetting(PrnPara setting);
void* TPrint(void*);
void  InitPrinter();
int   GetPaperBoxPaper(int PaperBoxPaperVal);

int   ExceptionHandler();
void  WaitThisPrintJobFinish();
void  HwReset();
int  WaitHwInitOK();
// cmd
#define  HW_RESET                        0x01
#define  PRINT_TASK_HALT          0x08
#define  PRINT_START                   0x10
#define  PRINT_DATA_SENT          0x04

// status
#define  PCL_PARSER_ERR            0x0200

#define  PRINT_END                       0x4000
#define  HW_INIT_READY               0x8000
#define  NO_MEM                           0x0100

#endif
