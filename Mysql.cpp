#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "LcdUI.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "Serial.h"
#include "globals.h"
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "Mysql.h"

 /*
 * create table printer_copy(
 * UID INTEGER NOT NULL,
 * HANDLE_TIME CHAR(256) NOT NULL,
 * HANDLE_TYPE CHAR(8) NOT NULL,
 * PAPER_TYPE CHAR(8) NOT NULL,
 * PAPER_NUM INTEGER NOT NULL,
 * )
 */

MYSQL mysql; /*这是一个数据库连接*/
User_info user = {0};
Admin_info admin = {0};
Permission_info per = {1};//默认普通模式全权限

void insert_user_entry(MYSQL *mysql)//用户操作信息登记
{
		open_mysql();
		printf("time:\n");
        char sql[SQL_LEN];
        printf("插入用户条目\n");
        memset(sql, 0, SQL_LEN);
        sprintf(sql, "insert into printer_copy values(NULL, %d, '%s', '%s', '%s', %d, %d)", user.user_id, user.handle_time, user.handle_type,user.paper_type,user.paper_nums,user.copys);
        printf("插入数据 [%s]\n", sql);

        if(mysql_real_query(mysql, sql, strlen(sql)))
        {
                error(mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

//op：0 insert；1 delete ;2 delete all
void insert_admin_entry(MYSQL *mysql,int op,int uid)//管理员操作登记
{
		open_mysql();
		//printf("time:\n");
		get_time(admin.handle_time);
		switch (op)
		{
			case 0:sprintf(admin.handle_type,"添加用户：%d",uid);break;
			case 1:sprintf(admin.handle_type,"删除用户：%d",uid);break;
			case 2:sprintf(admin.handle_type,"清除所有用户");break;
		}

        char sql[SQL_LEN];
        printf("插入管理员条目\n");
        memset(sql, 0, SQL_LEN);
        sprintf(sql, "insert into admin values(NULL, %d, '%s', '%s')", admin.admin_id, admin.handle_time, admin.handle_type);
        printf("插入数据 [%s]\n", sql);

        if(mysql_real_query(mysql, sql, strlen(sql)))
        {
                error(mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

void update_user_entry(MYSQL *mysql,int nums)//页数更新
{
		usleep(10000);
		open_mysql();
        char sql[SQL_LEN];
        printf("更新用户条目\n");
        memset(sql, 0, SQL_LEN);
		sprintf(sql,"update printer_copy set nums=%d where time='%s' and uid=%d",nums,user.handle_time,user.user_id);
        printf("更新数据 [%s]\n", sql);
		usleep(10000);
        if(mysql_real_query(mysql, sql, strlen(sql)))
        {
                error(mysql);
                close_mysql();
                return;
        }
        close_mysql();
}


void get_time(char *time)//得到硬件时间
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

void open_mysql()
{
	  usleep(10000);
	  mysql_init(&mysql);
      if(!mysql_real_connect(&mysql,"localhost",USERNAME, NULL, DATABASE, 0, NULL, CLIENT_FOUND_ROWS))
          error(&mysql);
     // else
         // printf("success connect...\n");
	  mysql_query(&mysql,"SET NAMES 'UTF8'");
	  usleep(10000);
}

void close_mysql()
{
	mysql_close(&mysql);
	//printf("mysql end!\n");
}

void RD_user_permission(int id)//获取用户权限
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *permission;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from permission where uid = %d",id);
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);   close_mysql(); return;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);

	permission = row[2];
	mysql_free_result(res);
	per.print_per = permission[0]-48;
	per.scan_per = permission[1]-48;
	per.copy_per = permission[2]-48;
	close_mysql();
}

//0p:0打印，1扫描，2复印
//返回值：1有权限，0无权限
int Get_user_permission(int id,int op)
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *permission;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from permission where uid = %d",id);
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);   close_mysql();  return -1;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);

	permission = row[2];
	mysql_free_result(res);
	close_mysql();
	if(op==0||op==1||op==2)
	return permission[op]-48;
	else return -1;
}
void WR_TINT(int tint)
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "update tint set tint='%d' where id=1",tint);
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);   close_mysql();  return;
	}
	res = mysql_store_result(&mysql);
	mysql_free_result(res);
	close_mysql();
}

int RD_TINT()
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from tint where id = 1");
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);   close_mysql();  return 0;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);
	int tint = atoi(row[1]);
	printf("%d\n",tint);
	mysql_free_result(res);
	close_mysql();
	return tint;
}
void WR_user_permission(int id,char per_print,char per_scan,char per_copy)//设置用户权限,此处不用
{
	    char sql[SQL_LEN];
        printf("插入条目\n");
        memset(sql, 0, SQL_LEN);
        char permission[4] = "000";
        printf("%c,%c,%c\n",per_print,per_scan,per_copy);
        permission[0] = per_print;
        permission[1] = per_scan;
        permission[2] = per_copy;
        printf("permission=%s\n",permission);
        sprintf(sql, "update permission set permission='%s' where uid=%d", permission, id);
        printf("插入数据 [%s]\n", sql);

        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
}

void Add_user(int id,char per_print,char per_scan,char per_copy)//指纹用户登记，默认权限全0,无权限
{
		open_mysql();
	    char sql[SQL_LEN];
        printf("插入条目\n");
        memset(sql, 0, SQL_LEN);
        char permission[4] = "000";
        printf("%c,%c,%c\n",per_print,per_scan,per_copy);
        permission[0] = per_print;
        permission[1] = per_scan;
        permission[2] = per_copy;
        printf("permission=%s\n",permission);
        sprintf(sql, "insert into permission values(NULL,%d,'%s')", id, permission);
        //sprintf(sql, "update permission set permission='%s' where uid=%d", permission, id);
        printf("插入数据 [%s]\n", sql);

        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}


void Delete_user(int id)//删除指定用户
{
		open_mysql();
        char sql[SQL_LEN];
        printf("删除\n");
        memset(sql, 0, SQL_LEN);
        sprintf(sql, "delete from permission where uid = %d", id);

        printf("删除数据 [%s]\n", sql);

        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

void Delete_All_user()//清除所有用户
{
		open_mysql();
        char sql[SQL_LEN];
        printf("删除\n");
        memset(sql, 0, SQL_LEN);
        sprintf(sql, "truncate table permission");

        printf("删除所有数据 [%s]\n", sql);

        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}


void boa_lock()//循环检测高位，到时间后拉低
{
	while(1){
		if(RD_time()=='1'){
			printf("login\n");
			sleep(60);
			printf("set low\n");
			WR_time('0');
		}
		printf("not login\n");
		sleep(5);
	}
}

void* LoginLock(void *)//循环检测高位，到时间后拉低
{
	while(1){
		if(RD_time()=='1'){//捕获到登陆成功，开始计时
			printf("login\n");
			sleep(60);
			printf("set low\n");
			WR_time('0');//一分钟后，置低，锁定页面只有重新登陆后才可以继续访问网页
		}
		//printf("not login\n");
		sleep(5);
	}
}

void WR_time(char op)//用于登陆成功后的置位
{
		open_mysql();
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
		sprintf(sql, "update lock_time set time='%c' where id=1",op);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
       close_mysql();
}

char RD_time()//用于boa页面登录许可检测
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *time;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from lock_time where id = 1");
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql); close_mysql();  return NULL;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);
	time = row[1];
	mysql_free_result(res);
	close_mysql();
	if(time[0] == '1') return '1';
	else return '0';
}




void RD_AllKindsofPages(AllKindsofPages &akps)
{
		printf("mysql rd_pages\n");
		open_mysql();
	    MYSQL_RES *res;
	    MYSQL_ROW row;
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	    sprintf(sql, "select * from all_pages where id = 1");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
            error(&mysql);   close_mysql(); return;
        }
        res = mysql_store_result(&mysql);
        row = mysql_fetch_row(res);
        akps.total = atoi(row[1]);
        akps.printed = atoi(row[2]);
        akps.duplicated = atoi(row[3]);
        akps.scaned = atoi(row[4]);
        mysql_free_result(res);
        close_mysql();
}

void WR_AllKindsofPages(AllKindsofPages &akps)
{
		//usleep(10000);
		open_mysql();
		printf("mysql wr_pages\n");
        char sql[SQL_LEN];
		memset(sql, 0, SQL_LEN);
		sprintf(sql,"update all_pages set total=%d,print=%d,copy=%d,scan=%d where id=1",akps.total,akps.printed,akps.duplicated,akps.scaned);
        printf("更新页数 =%s\n", sql);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

//更新数据出中的复印和扫描的数据
void UpdatePageSize(PageSize &sizedata)
{
        open_mysql();
        printf("mysql wr_pagesize\n");
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
        sprintf(sql,"update page_size set hight=%d,weight=%d,ncolormode = %d ,totalsize=%d where id=%d",sizedata.hight,sizedata.weight,sizedata.ncolormode,sizedata.totalsize,sizedata.cate);
        printf("更新复印扫描数据的大小信息 =%s\n", sql);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();

}

//把更新的数据从数据库中读取出来
void ReadPageSize(int &pageH , int &pageW ,int &ncolormode,int cate)
{
        printf("mysql ReadPageSize\n");
        open_mysql();
        MYSQL_RES *res;
        MYSQL_ROW row;
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
        sprintf(sql, "select * from page_size where id = %d",cate);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
            error(&mysql);    curStage = MYSQLERROR; close_mysql();  return;
        }
        res = mysql_store_result(&mysql);
        row = mysql_fetch_row(res);
        //wn.gain = atoi(row[1]);
        pageH = atoi(row[1]);
        pageW = atoi(row[2]);
        ncolormode = atoi(row[3]);
        mysql_free_result(res);
        close_mysql();
}

void WR_WN(SetDateofWN &wn)//写温度浓度参数
{
		printf("mysql wr_wn\n");
		open_mysql();
        char sql[SQL_LEN];
		memset(sql, 0, SQL_LEN);
		sprintf(sql,"update all_pages set print=%d,copy=%d,scan=%d where id=2",wn.tem1,wn.tem2,wn.tint);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

void WR_TEM(SetDateofWN &wn)//写温度参数
{
		open_mysql();
		printf("mysql wr_tem\n");
        char sql[SQL_LEN];
		memset(sql, 0, SQL_LEN);
		sprintf(sql,"update all_pages set print=%d where id=2",wn.tem1);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                //curStage = MYSQLERROR;
                close_mysql();
                return;
        }
        close_mysql();
}

void RD_WN(SetDateofWN &wn)//读增益温度浓度参数
{
		printf("mysql rd_wn\n");
		open_mysql();
	   MYSQL_RES *res;
	   MYSQL_ROW row;
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	    sprintf(sql, "select * from all_pages where id = 2");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
            error(&mysql);    curStage = MYSQLERROR; close_mysql();  return;
        }
        res = mysql_store_result(&mysql);
        row = mysql_fetch_row(res);
        //wn.gain = atoi(row[1]);
        wn.tem1 = atoi(row[2]);
        wn.tem2 = atoi(row[3]);
        wn.tint = atoi(row[4]);
        //printf("wn.tint is %x",wn.tint);
        mysql_free_result(res);
        close_mysql();
}

void RD_GAIN(SetDateofWN &wn)//读增益参数
{
		printf("mysql rd_gain\n");
		open_mysql();
	    MYSQL_RES *res;
	    MYSQL_ROW row;
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
	    sprintf(sql, "select * from gain where id = 1");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
            error(&mysql);     close_mysql(); return;
        }
        res = mysql_store_result(&mysql);
        row = mysql_fetch_row(res);
        wn.gain = atoi(row[1]);
        mysql_free_result(res);
        close_mysql();
}

void WR_GAIN(SetDateofWN &wn)//写增益参数
{
		open_mysql();
		printf("mysql wr_gain\n");
        char sql[SQL_LEN];
		memset(sql, 0, SQL_LEN);
		sprintf(sql,"update gain set gain=%d where id=1",wn.gain);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}



unsigned short Crcprocess(char *p,unsigned char len)//CRC校验算法
{
    unsigned char i=0;
	unsigned short Crc=0;
	while(len--)
	  {
		  for(i=0x80;i!=0;i>>=1)
		{
			if((Crc&0x8000) != 0)    { Crc<<=1; Crc ^= 0x1021;}
		 else                     { Crc<<=1;}
		 if((*p&i) != 0)          Crc ^= 0x1021;
		}
	   p++;
	  }
	return Crc;
}

void WR_crcWN(SetDateofWN &wn)//crc写温度浓度参数
{
  open_mysql();
  char sql[SQL_LEN];
  memset(sql, 0, SQL_LEN);
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
	printf("%x\n",aa);

	b[1] = wn.tem2&0xff;
	b[0] = wn.tem2>>8;
	crc = Crcprocess(a,2);
	b[2] = crc>>8;
	b[3] = crc;
	bb = (b[0]<<24)+(b[1]<<16)+(b[2]<<8)+b[3];
	printf("%x\n",bb);

	c[1] = wn.tint&0xff;
	c[0] = wn.tint>>8;
	crc = Crcprocess(c,2);
	c[2] = crc>>8;
	c[3] = crc;
	cc = (c[0]<<24)+(c[1]<<16)+(c[2]<<8)+c[3];
	printf("%x\n",cc);

	sprintf(sql,"update wn set c0=%d,c1=%d,c2=%d where id=1",aa,bb,cc);
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
			error(&mysql);
			return;
	}
	close_mysql();
}


void RD_crcWN(SetDateofWN &wn)//crc读温度浓度参数
{
	   open_mysql();
	   MYSQL_RES *res;
	   MYSQL_ROW row;
	   char sql[SQL_LEN];
	   memset(sql, 0, SQL_LEN);
	   char a[4] = {0x00,0x00,0x00,0x00};
	   char b[4] = {0x00,0x00,0x00,0x00};
       char c[4] = {0x00,0x00,0x00,0x00};
	   int aa = 0,bb = 0,cc = 0;
	   unsigned short crc = 0;
		sprintf(sql, "select * from wn where id = 1");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
            error(&mysql);   return;
        }
        res = mysql_store_result(&mysql);
        row = mysql_fetch_row(res);

		aa = atoi(row[1]);
		a[0] = aa>>24;
		a[1] = aa>>16;
		a[2] = aa>>8;
		a[3] = aa&0xff;
		crc = Crcprocess(a,4);
		if(crc!=0) wn.tem1 = 0;
	    else wn.tem1 = a[1]+(a[0]<<8);
	    crc = 0;
		printf("%d\n",wn.tem1);

		bb = atoi(row[2]);
		b[0] = bb>>24;
		b[1] = bb>>16;
		b[2] = bb>>8;
		b[3] = bb&0xff;
		crc = Crcprocess(b,4);
		if(crc!=0) wn.tem1 = 0;
	    else wn.tem1 = b[1]+(b[0]<<8);
	    crc = 0;
		printf("%d\n",wn.tem2);

		cc = atoi(row[3]);
		c[0] = cc>>24;
		c[1] = cc>>16;
		c[2] = cc>>8;
		c[3] = cc&0xff;
		crc = Crcprocess(c,4);
		if(crc!=0) wn.tem1 = 0;
	    else wn.tem1 = c[1]+(c[0]<<8);
	    crc = 0;
		printf("%d\n",wn.tint);

        mysql_free_result(res);
        close_mysql();
}

void WR_Mode(int op)//开机模式设置
{
//		mysql_init(&mysql);
//        if(!mysql_real_connect(&mysql,
//							"localhost",USERNAME, NULL, DATABASE, 0, NULL, CLIENT_FOUND_ROWS))
//                error(&mysql);
//		mysql_query(&mysql,"SET NAMES 'UTF8'");
		open_mysql();
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
		sprintf(sql, "update mode set mode=%d where id=1",op);
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
        //mysql_close(&mysql);
}

int RD_Mode()//开机模式读取
{
//	mysql_init(&mysql);
//	if(!mysql_real_connect(&mysql,
//						"localhost",USERNAME, NULL, DATABASE, 0, NULL, CLIENT_FOUND_ROWS))
//			error(&mysql);
//	mysql_query(&mysql,"SET NAMES 'UTF8'");
    open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	int time;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from mode where id = 1");
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);   close_mysql(); return NULL;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);
	time = atoi(row[1]);
	mysql_free_result(res);
	close_mysql();
	if(time == 1) return 1;
	else return 0;
	//mysql_close(&mysql);
}

void WR_ErrorPW()//更新密码错误次数
{
		open_mysql();
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
		sprintf(sql, "update mode set mode=mode+1 where id=4");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}

int RD_ErrorPW()//获取密码错误次数
{
	open_mysql();
	MYSQL_RES *res;
	MYSQL_ROW row;
	int errortimes;
	char sql[SQL_LEN];
	memset(sql, 0, SQL_LEN);
	sprintf(sql, "select * from mode where id = 4");
	if(mysql_real_query(&mysql, sql, strlen(sql)))
	{
		error(&mysql);  close_mysql(); return NULL;
	}
	res = mysql_store_result(&mysql);
	row = mysql_fetch_row(res);
	errortimes = atoi(row[1]);//读errortimes
	mysql_free_result(res);
	close_mysql();
	return errortimes;
}

void Unlock_ErrorPW()//解锁密码锁定
{
		open_mysql();
        char sql[SQL_LEN];
        memset(sql, 0, SQL_LEN);
		sprintf(sql, "update mode set mode=0 where id=4");
        if(mysql_real_query(&mysql, sql, strlen(sql)))
        {
                error(&mysql);
                close_mysql();
                return;
        }
        close_mysql();
}





