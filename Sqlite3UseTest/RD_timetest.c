#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sqlite3.h>
#include <errno.h>

//#include "LcdUI.h"
//#include "SQLlite3.h"
//#include "Serial.h"
//#include "globals.h"

#define DATABASE "printer.db"	//数据库文件存放路径
//#define RTC "/dev/rtc"
#define  SQL_LEN 256
sqlite3 *db = NULL;
char *zErrMsg = 0;
int rc = 0;

typedef struct  User_info//用户信息
{
	int user_id;
	char handle_time[256];
	char handle_type[32];
	char paper_type[32];
	int paper_nums;
	int copys;
}User_info;

typedef struct  Admin_info//管理员信息
{
	int admin_id;
	char handle_time[256];
	char handle_type[32];
}Admin_info;

typedef struct  Permission_info//权限信息 1有 0无
{
	unsigned short print_per;
	unsigned short scan_per;
	unsigned short copy_per;
}Permission_info;


typedef struct AllKindsofPages
{
      int  total;          // 总页数, exclude scaned
      int  printed;      // 打印数
      int  duplicated; // 复印数
      int  scaned;      // 扫描页数　
}AllKindsofPages;

typedef struct PageSize
{
      int  hight;          // 图像的高
      int  weight;      // 图像的宽
      int  totalsize; // 图像的全部大小
      int  ncolormode;  //0-代表真彩，1-灰度  2-二值
      int  cate	;		//1代表扫描数据，2代表复印数据
}PageSize;

typedef struct SetDateofWN
{
      unsigned short  tem1;      // 温度1
      unsigned short  tem2; 	  // 温度2
      unsigned short  tint;      // 浓度　
      unsigned short  gain;      // 增益
} SetDateofWN;


User_info user = {0};
Admin_info admin = {0};
Permission_info


 per = {1};//默认普通模式全权限
char s[10] ="sql";



void open_sqlite3()
{
	rc = 0;
	rc = sqlite3_open(DATABASE , &db);
	if(rc)
	{
		fprintf(stderr , "Can't open database:%s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}
	else
	{
		printf("You have opened a sqlite3 database printer.db successfully!\n");
	}
}

//用于登陆成功后的置位
void WR_time(char op)
{
	open_sqlite3();
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "update lock_time set time='%c' where id=1",op);
	printf("更新状态 [%s]\n", sql);

	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
	
}
//用于boa页面登陆许可检测
char RD_time()
{
	open_sqlite3();
	char *time;
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from lock_time where id = 1");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return 0;
	}
	int i;
	for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	time = azResult[3];
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
	if(time[0] == '1') return '1';
	else return '0';
}
//循环检测高位，到时见后拉低
void boa_lock()
{
	while(1)
	{
		if(RD_time()=='1')
		{
			printf("login\n");
			sleep(60);
			printf("set low\n");
			WR_time('0');
		}
		printf("not login\n");
		sleep(5);
	}
}

//循环检测高位，到时间后拉低
void* LoginLock(void* p)
{
	while(1)
	{
		if(RD_time()=='1')//捕获登陆成功，开始计时
		{
			printf("login\n");
			sleep(60);
			printf("set low\n");
			WR_time('0');//一分钟，置地，锁定页面只有重新登陆才可以继续访问
		}
		sleep(5);
	
	}
}


//开机模式设置
void WR_Mode(int op)
{
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql, "update mode set mode=%d where id=1",op);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}


int main()
{
	AllKindsofPages akps;
	PageSize sizedata;
	SetDateofWN wn;
	char sign;
	//insert_user_entry();
	//insert_admin_entry(0, 1);
	//update_user_entry(5);
	//RD_user_permission(2);
	//int res = RD_TINT();
	//printf("res :%d",res);
	//WR_user_permission(1,'1','0','0');
	//Add_user(1,'1','1','0');
	//Delete_All_user();
	sign = RD_time();
	printf("sign = %c",sign);
	//RD_AllKindsofPages(akps);
	
	return 0;
}























