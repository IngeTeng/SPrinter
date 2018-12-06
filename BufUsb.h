#ifndef BUFUSB_H_INCLUDED
#define BUFUSB_H_INCLUDED

#include "os_def.h"
//#include "Usb.h"

void IniUsbBuffer(BUF_USB *buf);
int UsbBufRearrange(BUF_USB *buf);
//int Read2UsbBuf(BUF_USB *buf,int size,int maxMs);
int ReadOnePacket2UsbBuf(BUF_USB *buf);
int GetMemLost();
#endif
