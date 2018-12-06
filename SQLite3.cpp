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
#include <errno.h>
#include "LcdUI.h"
#include "Serial.h"
#include "globals.h"
#include "SQLite3.h"



sqlite3 *db = NULL;
char *zErrMsg = 0;
int rc = 0;

User_info user = {0};
Admin_info admin = {0};
Permission_info per = {1};//默认普通模式全权限

//得到硬件时间
void get_time(char *time)
{
    int fd, retval, irqcount = 0;
    struct rtc_time rtc_tm;
    fd = open(RTC, O_RDONLY);
    if (fd == -1) {
        perror(RTC);
        exit(errno);
    }
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    	if (retval == -1) {
        	perror("RTC_RD_TIME ioctl");
        	exit(errno);
    	}
	sprintf(time, "%d-%d-%d,%02d:%02d:%02d",rtc_tm.tm_year + 1900,rtc_tm.tm_mon + 1,rtc_tm.tm_mday,
	rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
	close(fd);
}


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
		//printf("You have opened a sqlite3 database printer.db successfully!\n");
	}
}

//用户操作信息登记
void insert_user_entry()
{
	open_sqlite3();
	char sql[SQL_LEN];
	if(debug) printf("插入用户条目\n");
	memset(sql, 0, SQL_LEN);

	//sprintf(sql, "insert into printer_copy values(NULL, %d, '%s', '%s', '%s', %d, %d)", 1, s, s,s,5,6);
	sprintf(sql, "insert into printer_copy values(NULL, %d, '%s', '%s', '%s', %d, %d)", user.user_id, user.handle_time, user.handle_type,user.paper_type,user.paper_nums,user.copys);
	if(debug) printf("插入数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}
//管理员操作登记 0-insert,1-delete,2-delete all
void insert_admin_entry(int op,int uid)
{
	open_sqlite3();
	get_time(admin.handle_time);
	switch(op)
	{
		case 0:sprintf(admin.handle_type,"添加用户：%d",uid);break;
		case 1:sprintf(admin.handle_type,"删除用户：%d",uid);break;
		case 2:sprintf(admin.handle_type,"清除所有用户");break;
	}
	//char handle_type[256];
	//switch(op)
	//{
	//
	//	case 0:sprintf(handle_type,"添加用户：%d",uid);break;
	//	case 1:sprintf(handle_type,"删除用户：%d",uid);break;
	//	case 2:sprintf(handle_type,"清除所有用户");break;
	//}
	char sql[SQL_LEN];
	if(debug) printf("插入管理员条目\n");
	memset(sql, 0, SQL_LEN);
	//sprintf(sql, "insert into admin values(NULL, %d, '%s', '%s')", 1, s, s);
        sprintf(sql, "insert into admin values(NULL, %d, '%s', '%s')", admin.admin_id, admin.handle_time, admin.handle_type);
        if(debug) printf("插入数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}

//页数更新
void update_user_entry(int nums)
{
	open_sqlite3();
	char sql[SQL_LEN];
        if(debug) printf("更新用户条目\n");
        memset(sql, 0, SQL_LEN);
	sprintf(sql,"update printer_copy set nums=%d where time='%s' and uid=%d",nums,user.handle_time,user.user_id);
	//sprintf(sql,"update printer_copy set nums=%d where time='%s' and uid=%d",nums,s,2);
        if(debug) printf("更新数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}
//获取用户权限
void RD_user_permission(int id)
{
	open_sqlite3();
	char *permission;
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	//查询数据
	/*
	int sqlite3_get_table(sqlite3* , const char *sql , char **result , int *nrow , int *ncolumn , char **errmsg);
	result中是以数组形式存放所查询的数据，首先是表名，再是数据。
	nrow ,ncolumn分别为查询语句返回的结果集的行数、列数、没有查到结果时返回0
	*/
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from permission where uid = %d",id);
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	//int i = 0;
	//printf("row:%d column=%d\n",nrow ,ncolumn);
	//printf("/nThe result of querying is :\n");
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	permission = azResult[5];
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	per.print_per = permission[0]-48;
	per.scan_per = permission[1]-48;
	per.copy_per = permission[2]-48;
	if(debug) printf("per.print_per:%d ,per.scan_per:%d ,per.copy_per:%d\n",per.print_per,per.scan_per,per.copy_per);
	sqlite3_close(db);

}
//更新浓度增益
void WR_TINT(int tint)
{
	open_sqlite3();
	char sql[SQL_LEN];
        if(debug) printf("写入浓度增益\n");
        memset(sql, 0, SQL_LEN);
	sprintf(sql,"update tint set tint='%d' where id=1",tint);
        if(debug) printf("更新数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}
//读取浓度增益
int RD_TINT()
{
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from tint where id = 1");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return 0;
	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	int tint = atoi(azResult[3]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
	return tint;
}

//设置用户权限,此处不用
void WR_user_permission(int id, char per_print, char per_scan ,char per_copy )
{
	open_sqlite3();
	char sql[SQL_LEN];
	if(debug) printf("设置用户权限，此处不用\n");
	memset(sql, 0, SQL_LEN);
	char permission[4] = "000";
	if(debug) printf("%c , %c , %c \n",per_print,per_scan,per_copy);
	permission[0] = per_print;
	permission[1] = per_scan;
	permission[2] = per_copy;
	if(debug) printf("permission = %s\n",permission);
	sprintf(sql, "update permission set permission='%s' where uid=%d", permission, id);
	if(debug) printf("插入数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);

}
//指纹用户登记，默认权限全0，无权限
void Add_user(int id ,char per_print ,char per_scan,char per_copy)
{
	open_sqlite3();
	char sql[SQL_LEN];
	if(debug) printf("设置用户权限，此处不用\n");
	memset(sql, 0, SQL_LEN);
	char permission[4] = "000";
	if(debug) printf("%c , %c , %c \n",per_print,per_scan,per_copy);
	permission[0] = per_print;
	permission[1] = per_scan;
	permission[2] = per_copy;
	if(debug) printf("permission = %s\n",permission);
	sprintf(sql, "insert into permission values(NULL,%d,'%s')", id, permission);
	if(debug) printf("插入数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}
//删除指定用户
void Delete_user(int id)
{
	open_sqlite3();
	char sql[SQL_LEN];
	if(debug) printf("删除\n");
	memset(sql, 0, SQL_LEN);

	sprintf(sql, "delete from permission where uid = %d", id);
	if(debug) printf("删除数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
	}
	sqlite3_close(db);
}
//清除所有用户
void Delete_All_user()
{
	open_sqlite3();
	char sql[SQL_LEN] , sql1[SQL_LEN];
	if(debug) printf("删除\n");
	memset(sql, 0, SQL_LEN);

	sprintf(sql, "delete from permission");
	sprintf(sql1, "update sqlite_sequence set seq = 0 where name ='permission'");
	if(debug) printf("删除数据 [%s]\n", sql);

	sqlite3_exec(db , sql ,0 , 0,&zErrMsg);
	sqlite3_exec(db , sql1 ,0 , 0,&zErrMsg);

	sqlite3_close(db);
}
//用于登陆成功后的置位
void WR_time(char op)
{
	open_sqlite3();
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "update lock_time set time='%c' where id=1",op);
	if(debug) printf("更新状态 [%s]\n", sql);

	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
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
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return 0;
	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	time = azResult[3];
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
	//printf("time[0] is %c\n" , time[0]);
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
			if(debug) printf("login\n");
			sleep(60);
			if(debug) printf("set low\n");
			WR_time('0');
		}
		if(debug) printf("not login\n");
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
			if(debug) printf("login\n");
			sleep(60);
			if(debug) printf("set low\n");
			WR_time('0');//一分钟，置地，锁定页面只有重新登陆才可以继续访问
		}
		sleep(5);

	}
}

//读取所有打印、复印、扫描次数
void RD_AllKindsofPages(AllKindsofPages &akps)
{
	if(debug) printf("mysql rd_pages\n");
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from all_pages where id = 1");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	akps.total = atoi(azResult[6]);
	akps.printed = atoi(azResult[7]);
	akps.duplicated = atoi(azResult[8]);
	akps.scaned = atoi(azResult[9]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);

}

void WR_AllKindsofPages(AllKindsofPages &akps)
{
	open_sqlite3();
	if(debug) printf("mysql wr_pages\n");
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql,"update all_pages set total=%d,print=%d,copy=%d,scan=%d where id=1",akps.total,akps.printed,akps.duplicated,akps.scaned);
        if(debug) printf("更新数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
//更新数据库中的复印和扫描的数据
void UpdatePageSize(PageSize &sizedata)
{
	open_sqlite3();
	if(debug) printf("mysql wr_pages\n");
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql,"update page_size set hight=%d,weight=%d,ncolormode = %d ,totalsize=%d where id=%d",sizedata.hight,sizedata.weight,sizedata.ncolormode,sizedata.totalsize,sizedata.cate);
	if(debug) printf("更新数据 [%s]\n", sql);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
//把更新的数据从数据库中读取出来
void ReadPageSize(int &pageH ,int &pageW ,int &ncolormode ,int cate)
{
	if(debug) printf("mysql ReadPageSize\n");
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from page_size where id = %d",cate);
	if(debug) printf("更新数据 [%s]\n", sql);
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;

	}
	int i;
	for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		if(debug) printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	pageH = atoi(azResult[6]);
	pageW = atoi(azResult[7]);
	ncolormode = atoi(azResult[8]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);


}

//写温度浓度参数
void WR_WN(SetDateofWN &wn)
{
	if(debug) printf("mysql wr_wn\n");
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql,"update all_pages set print=%d,copy=%d,scan=%d where id=2",wn.tem1,wn.tem2,wn.tint);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
//写温度参数
void WR_TEM(SetDateofWN &wn)
{
	if(debug) printf("mysql wr_tem\n");
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql,"update all_pages set print=%d where id=2",wn.tem1);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}

//读增益温度浓度参数
void RD_WN(SetDateofWN &wn)
{
	if(debug) printf("mysql rd_wn\n");
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from all_pages where id = 2");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;

	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	wn.tem1 = atoi(azResult[7]);
	wn.tem2 = atoi(azResult[8]);
	wn.tint = atoi(azResult[9]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
}

//读增益参数
void RD_GAIN(SetDateofWN &wn)
{
	if(debug) printf("mysql rd_gain\n");
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from gain where id = 1");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;

	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	wn.gain = atoi(azResult[3]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
}

//写增益参数
void WR_GAIN(SetDateofWN &wn)
{
	if(debug) printf("mysql wr_gain\n");
	open_sqlite3();
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql,"update gain set gain=%d where id=1",wn.gain);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}

//CRC校验算法
unsigned short Crcprocess(char *p, unsigned char len)
{
	unsigned char i=0;
	unsigned short Crc = 0;
	while(len--)
	{
		for( i = 0x80;i!=0;i>>=1)
		{
			if((Crc&0x8000) !=0)
			{
				Crc<<=1;
				Crc^=0x1021;
			}
		else {Crc<<1;}
		if((*p&i) !=0) Crc ^= 0x1021;
		}
		p++;
	}
	return Crc;
}

//CRC写温度浓度参数
void WR_crcWN(SetDateofWN &wn)
{
	if(debug) printf("mysql wr_wn\n");
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	//CRC校验
	char a[4] = {0x00,0x00,0x00,0x00};
  	char b[4] = {0x00,0x00,0x00,0x00};
  	char c[4] = {0x00,0x00,0x00,0x00};
  	int aa = 0,bb = 0,cc = 0;
  	unsigned short crc = 0;

	a[1] = wn.tem1&0xff;
	a[0] = wn.tem1>>8;
	crc = Crcprocess(a,2);
	a[2] = crc>>8;
	a[3] = crc;
	aa = (a[0]<<24)+(a[1]<<16)+(a[2]<<8)+a[3];
	if(debug) printf("%x\n",aa);

	b[1] = wn.tem2&0xff;
	b[0] = wn.tem2>>8;
	crc = Crcprocess(a,2);
	b[2] = crc>>8;
	b[3] = crc;
	bb = (b[0]<<24)+(b[1]<<16)+(b[2]<<8)+b[3];
	if(debug) printf("%x\n",bb);

	c[1] = wn.tint&0xff;
	c[0] = wn.tint>>8;
	crc = Crcprocess(c,2);
	c[2] = crc>>8;
	c[3] = crc;
	cc = (c[0]<<24)+(c[1]<<16)+(c[2]<<8)+c[3];
	if(debug) printf("%x\n",cc);

	sprintf(sql,"update all_pages set print=%d,copy=%d,scan=%d where id=2",aa,bb,cc);
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
//crc读温度浓度参数
void RD_crcWN(SetDateofWN &wn)
{

	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	char a[4] = {0x00,0x00,0x00,0x00};
	char b[4] = {0x00,0x00,0x00,0x00};
       	char c[4] = {0x00,0x00,0x00,0x00};
	int aa = 0,bb = 0,cc = 0;
	unsigned short crc = 0;
	sprintf(sql, "select * from all_pages where id = 2");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;

	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	aa = atoi(azResult[3]);
	a[0] = aa>>24;
	a[1] = aa>>16;
	a[2] = aa>>8;
	a[3] = aa&0xff;
	crc = Crcprocess(a,4);
	if(crc!=0) wn.tem1 = 0;
	else wn.tem1 = a[1]+(a[0]<<8);
	crc = 0;
	if(debug) printf("%d\n",wn.tem1);

        bb = atoi(azResult[4]);
	b[0] = bb>>24;
	b[1] = bb>>16;
	b[2] = bb>>8;
	b[3] = bb&0xff;
	crc = Crcprocess(b,4);
	if(crc!=0) wn.tem1 = 0;
	else wn.tem1 = b[1]+(b[0]<<8);
	crc = 0;
	if(debug) printf("%d\n",wn.tem2);
        cc = atoi(azResult[5]);
	c[0] = cc>>24;
	c[1] = cc>>16;
	c[2] = cc>>8;
	c[3] = cc&0xff;
	crc = Crcprocess(c,4);
	if(crc!=0) wn.tem1 = 0;
	else wn.tem1 = c[1]+(c[0]<<8);
	crc = 0;
	if(debug) printf("%d\n",wn.tint);

	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
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
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}

//开机模式读取
int RD_Mode()
{
	open_sqlite3();
	char sql[SQL_LEN];
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	int time;
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from mode where id = 1");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return NULL;

	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	time = atoi(azResult[3]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
	if(time == 1) return 1;
	else return 0;
}
//更新密码错误次数
void WR_ErrorPW()
{
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql, "update mode set mode=mode+1 where id=4");
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
//获取密码错误次数
int RD_ErrorPW()
{
	open_sqlite3();
	char sql[SQL_LEN];
	int errortimes;
	int nrow = 0,ncolumn = 0;
	char **azResult;//二维数组存放结果
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from mode where id = 4");
	if(sqlite3_get_table(db ,sql ,&azResult ,&nrow ,&ncolumn ,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return NULL;

	}
	int i;
	//for(i= 0 ; i<(nrow +1)*ncolumn;i++)
		//printf("azResult[%d] = %s\n", i ,azResult[i]);
	//printf("azResult[4] %d",azResult[4]);
	errortimes = atoi(azResult[3]);
	//释放掉azResult的内存空间
	sqlite3_free_table(azResult);
	sqlite3_close(db);
	return errortimes;
}

//解锁密码锁定
void Unlock_ErrorPW()
{
	open_sqlite3();
	char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	sprintf(sql, "update mode set mode=0 where id=4");
	if(sqlite3_exec(db , sql ,0 , 0,&zErrMsg))
	{
		if(debug) printf("%s\n",zErrMsg);
		sqlite3_close(db);
		return;
	}
	sqlite3_close(db);
}
























