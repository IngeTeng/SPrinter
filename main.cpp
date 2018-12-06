#include <iostream>
#include "pthread.h"
#include "memory.h"
#include "unistd.h"
#include "BufUsb.h"
#include "GPMC.h"
#include "Serial.h"
#include "PrintJob.h"
#include "ScanPrintTypes.h"
#include "LcdUI.h"
#include "fcntl.h"
#include "poll.h"
#include <netinet/in.h>
#include "Scan.h"
#include "stdlib.h"
#include "globals.h"
#include "ScanPrint.h"


using namespace std;
int debug = 1;
// global var
pthread_t   tid_lcd = 0,  tid_keys = 0,  tid_vfinger = 0,   tid_prn = 0, tid_rep_prn = 0, tid_scan = 0, tid_rep_scan = 0 , tid_sp = 0, tid_rep_sp = 0,  tid_net = 0, tid_status = 0, tid_scancheck = 0,	tid_loginlock = 0;
BUF_USB                  _bufUsb;
RUNNING_STATE   _state = {0};
volatile   int    g_exit = 0;                           // 退出线程标志
volatile   int    isUsbDump = 0;                        // USB中断标志
int    prn_scan_fd , prn_repprint_fd , prn_repscanprint_fd , prn_repscan_fd;
int 	repeat_print_sign , repeat_scanprint_sign , repeat_scan_sign;
int   _gpmc;
short  prn_scan_Port = 9001;
STAGE   oldStage ;
short scanPort = 9002,  prnPort = 9001;
int    scanfd,  prnfd;
int number = 0;
unsigned short Concentration;			//碳粉浓度--实施更新预测
// 设备初始化
void InitScanPrinter()
{
//    open_mysql();			// 获得mysql 连接.
    serial4_LCD_open();     // _seral4
    _gpmc = open(FPGA_DEV, O_RDWR);
    RD_WN(SetofWN);						//读取数据库中的温度浓度增益参数
    usleep(10000);
    RD_GAIN(SetofWN);
    set_reg(0x25,SetofWN.gain);			//写入保存的增益数值
    if(debug) printf("init gain = %d\n",SetofWN.gain);
    if((SetofWN.tem1 & 0x0FFF) >150) SetofWN.tem1 = 0;//保护机制，防止从数据库中读取到的温度数值出错
    if(debug) printf("init tem = %d\n",SetofWN.tem1);
    set_reg(0x2d,SetofWN.tem1);         //写入保存的温度数值
	HwReset();                     // 消除异常初始化
    InitScanner(600, 1, 128);   //  1 Gray Image
    InitScanPos();  // 扫描就位
    ScannerCalibrationAfterPosZeroFind();
    set_reg(PadLEDReg,  G_LED_Val | 0x0F00);   // 给指纹加电.
    serial5_Finger_open();   // _seral5
    usleep(200000);
    ComWriteCmd("sleep=0"); // 唤醒片内意外睡眠
    RD_SysProfile(SysData);             // 机器当前配置参数读取
    RD_AllKindsofPages(AllKindPages);   // 机器当前计数读取
    usleep(10000);
	curSPPara.nMode = RD_Mode();        //读取数据库中的模式参数
	if(debug) printf("%d",curSPPara.nMode);
    curSPPara.nPaperSize = nBoxPaperSz = checkPaperBox1Paper();
    sprintf(curSPPara.szPaper, "%s", szBoxPaper);
	//测试加入机器初始化的时候进行确定缩放比例
	Concentration = get_reg(0x0041);//设置初始值浓度预测
	printf("Concentration %0.4f\n", Concentration/4096.0*3.3);
    if(debug) printf("\n\n**  InitScanPrinter OK **\n\nserial4.fd  %x,   serial5.fd %x, gpmc.fd %x,  SysData (KB:%d  ST:%d  RT:%d), curSPPara.nPaperSize %d\n",
    seri_fd4.fd,  seri_fd5.fd ,  _gpmc,  SysData.B_KPaper, SysData.IdleSecsToSleep, SysData.SecsPanelReset, curSPPara.nPaperSize);
    //MoveTo(curYPos + 1500);
    usleep(500000);
	PS7PressUP();
    number--;
	PS7PressDown();
	CalScale_Page_to_Scan();
}

void CloseScanPrinter()
{
     close(_gpmc);
     close(seri_fd4.fd);
     close(seri_fd5.fd);
     //close_mysql();
}

// 监听状态
void* StatusListen(void *)
{
	if(curSPPara.nMode==0){//安全模式登陆
		curStage = FINGERLOGIN;
		if(debug) printf("Safe mode\n");
	}
	else{//普通模式登陆，默认权限全有
		curStage = READY;
		per.copy_per = per.print_per = per.scan_per = 1;
		if(debug) printf("unSafe mode\n");
	}
       oldStage = curStage;
       unsigned short  wVal = get_reg(HW_SENSOR_REG);   wVal &= 0x3E;   set_reg(HW_SENSOR_REG, wVal);
       while( 1 )
	   {

	           int SeriousErrorHappen = 0,  warningHappern = 0;
               _state.status[0]   = get_reg(HW_STATUS_REG);
               _state.status[1]   = get_reg(HW_SENSOR_REG);
               oldStage = curStage;
			  if(curStage == FINGERLOGIN || curStage == PASSWDLOGIN || curStage == SLEEP) continue;

			  if((_state.status[1] & 0x100)&&(curStage == READY))//碳粉即将用尽,因为只是显示一行字符所以需要另作一个状态，而不是和warning共用
			   {
					drawTonerWillEmpty();
					sleep(1);
					if(debug) printf("toner: curStage  %d,  status: %02x \n",  curStage, _state.status[1]);
			   }
			//if(debug) printf("loop: oldStage %d,	curStage  %d,  status:  %x  %02x  \n",  oldStage, curStage, _state.status[0], _state.status[1]);
               while(( _state.status[0]  &  0xFFF )|| (_state.status[1]  & 0xFF) || (curSPPara.nPaperBox == 2 && (_state.status[1]  & 0x40)) )  // 非 0 就是异常(缺纸，错纸等提示，等待)
			   {
                     curStage = WARNING;  UpdateUI = 1;  warningHappern = 1;
                     if((_state.status[1] & 0x04))  // 卡纸
                     {
						 PrnCmdRegVal  |=   PRINT_TASK_HALT;        set_reg(PRN_CMD_REG, PrnCmdRegVal);      HwReset() ;
                     }
                     if( _state.status[0] & 0xFFF) //对严重的错误，结束
                     {
                         SeriousErrorHappen = 1;
                     }
                     sleep(1);
                     _state.status[0]   = get_reg(HW_STATUS_REG);
                     _state.status[1]   = get_reg(HW_SENSOR_REG);
                     if(debug) printf("waring: oldStage %d,	curStage  %d,  status:  %x  %02x  \n", oldStage,curStage, _state.status[0], _state.status[1]);
               }

               //printf("warningHappern = %d \t SeriousErrorHappen = %d\n",warningHappern,SeriousErrorHappen);
               if(SeriousErrorHappen)//这个可能有问题，因为严重错误时就会一直置高，出不了上个循环，也就不会恢复到之前状态
               {
                    curStage = READY;
               }else if(warningHappern)
               {
                    curStage = oldStage;  UpdateUI = 1;	//printf("warningHappern\n");
               }
//			   if(!SeriousErrorHappen&&!warningHappern&&curStage==READY)
//			   {
//                    drawMainFlash();
//			   }
//               if(curStage == SING_PING||curStage == PRINTING||curStage==READY)
//               {
//					unsigned short x1 = get_reg(0x00);
//					unsigned short x2 = get_reg(0x00);
//					unsigned short x3 = get_reg(0x00);
//					unsigned short x4 = get_reg(0x00);
//					char tt[32] = { 0 }; snprintf(tt, 32, "t4.txt=\"%x,%x,%x,%x\"",x1,x2,x3,x4);
//					ComWriteCmd(tt);
//					printf("tem1 = %x,tem2 = %x,tine = %x\n",SetofWN.tem1,SetofWN.tem2,SetofWN.tint);
//					sleep(1);
//                }
			  // if(debug) printf("curStage  %d,  status:  %02x  %02x\n",  curStage, _state.status[0], _state.status[1]);
               usleep(100000);
       }
       return 0;
}

// 监听网络端口，有数据到来时，建立连接线程，目前版本：等待线程结束（串行阻塞）。
void* SocketListen(void*)
{
	int listenfd_ps = socket(AF_INET, SOCK_STREAM, 0);  //监听fd_printer_scanner
  int nready = 0;
	struct sockaddr_in	cliaddr,  servaddr;
    socklen_t	clilen = sizeof(cliaddr);
    bzero(&servaddr, sizeof(servaddr));   servaddr.sin_family      = AF_INET;	   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	   servaddr.sin_port   = htons(prn_scan_Port);
	unsigned value = 1;
    setsockopt(listenfd_ps, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	bind(listenfd_ps, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd_ps, 2);

	int	usbfd = open(USB_PRINTER_FILE,  O_RDWR);
	struct pollfd		client[2];
	client[0].fd = listenfd_ps;        client[0].events = POLLIN | POLLERR;   //第一个pollfd结构体监视网络监听
	client[1].fd = usbfd;              client[1].events = POLLIN | POLLRDNORM;// usb_fd
	while(1)
	 {
			//if(isUsbDump) { usbfd = open(USB_PRINTER_FILE,  O_RDWR); client[1].fd = usbfd; client[1].events = POLLIN | POLLRDNORM; }
            //if(curStage != READY)     { sleep(2);      continue; }
            if(curSPPara.nMode==0){
				if(curStage != READY)     { sleep(2);      continue; }//安全模式睡眠时监听到任务不执行
			}
            nready = poll(client, 2, 1000);
            if(nready > 0)
            {
					if(curStage == SLEEP) OnESAVE();//非安全模式，收到访问信号就自动唤醒
                    g_exit =  0;  if(debug) printf("poll in,  fds0:%d fdusb1:%d\n",client[0].fd, client[1].fd);
                    prn_scan_fd = -1;
                    int bUsb = 0;
                    if (client[0].revents & POLLIN)         // new client connection 每次有新连接都会执行这个if语句，然后将新添加的链接调用accept来接受链接
                    {
                            prn_scan_fd = accept(listenfd_ps, (struct sockaddr *) &cliaddr, &clilen); //accept函数返回了一个print socketfd
                    }
                    else if (client[1].revents & POLLRDNORM)
				    {
                            prn_scan_fd = usbfd;  bUsb = 1;
                    }

                   if(prn_scan_fd < 0) {  continue;  usleep(100000);}
                   REQUEST_PACKET rp = {0};
                   read(prn_scan_fd,  &rp,  sizeof(REQUEST_PACKET));//网络数据包
                   PRN_SCAN_PARA  psp ={ prn_scan_fd,  rp.dwUserId , bUsb};
                   if(!rp.dwRequest)
                   {
            repeat_print_sign = 0;
						pthread_create(&tid_prn, 0, TPrint,  (void*)&psp);              // Print implemented  in PrintJob.cpp 。
						//curStage = PRINTING;   UpdateUI = 1;
						pthread_join(tid_prn, 0);   tid_prn = 0;                     //  终止线程时可判断该值。
                   }else
                   {
						pthread_create(&tid_scan, 0, Scan,  (void*)&psp);
						pthread_join(tid_scan, 0);   tid_scan = 0;                   //  终止线程时可判断该值
                   }
                   curStage = READY;    UpdateUI = 1;
                   usleep(100000);
           }
	}

	close(listenfd_ps);
	close(usbfd);
    return 0;
}

int main( )
{
     // 系统加电初始化
     InitScanPrinter();    if(debug) printf("After InitScanPrinter\n");

     pthread_create(&tid_lcd,              0,   Lcd_display,   		0);                       // LCD 显示
     pthread_create(&tid_keys,             0,   KeysListen,    		0);                       // 按键监控
     pthread_create(&tid_net,              0,   SocketListen,       0);                       // 网络监控
     pthread_create(&tid_scancheck,        0,   ScanPageSizeCheck,  0);                       // 扫描盖板检测
     pthread_create(&tid_status,           0,   StatusListen,  		0);                       // 状态监控
     pthread_create(&tid_vfinger,          0,   ScanFinger,   	(void*)&op);    			  // 指纹响应
	 pthread_create(&tid_loginlock,        0,   LoginLock,   		0);                       // 访问锁定

	 pthread_join(tid_net,       0);
	 pthread_join(tid_lcd,       0);
	 pthread_join(tid_keys,      0);
	 pthread_join(tid_vfinger,   0);
	 pthread_join(tid_scancheck, 0);
	 pthread_join(tid_status,    0);
	 pthread_join(tid_loginlock, 0);
     CloseScanPrinter();
    return 0;
}
