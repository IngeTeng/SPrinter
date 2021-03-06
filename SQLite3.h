#ifndef SQLITE_INCLUDE
#define SQLITE_INCLUDE
#define MYSQL_H_INCLUDED
#define DATABASE "/bin/printer.db"	//数据库文件存放路径
#define RTC "/dev/rtc0"
#define  SQL_LEN 256
#define crc_mul 0x1021    /*生成多项式*/

#include <sqlite3.h>

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


void insert_user_entry();
void insert_admin_entry(int op,int uid);
void update_user_entry(int nums);

void RD_user_permission(int id);
void WR_user_permission(int id,char per_print,char per_scan,char per_copy);
void Add_user(int id,char per_print,char per_scan,char per_copy);
void Delete_user(int id);
void boa_lock();
void WR_time(char op);
char RD_time();
void WR_GAIN(SetDateofWN &wn);
void RD_AllKindsofPages(AllKindsofPages &akps);
void WR_AllKindsofPages(AllKindsofPages &akps);
int Get_user_permission(int id,int op);
void get_time(char *time);
void WR_Mode(int op);
int RD_Mode();
void Delete_All_user();
void Unlock_ErrorPW();
int RD_ErrorPW();
void WR_ErrorPW();
unsigned short Crcprocess(char *p,unsigned char len);
void open_mysql();
void close_mysql();
#endif // SQLITE_INCLUDE

