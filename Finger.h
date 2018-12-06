#ifndef _FINGER_H_
#define _FINGER_H
//////////////////////////////
#include "Serial.h"

extern  serial_arg   seri_fd5;
typedef unsigned char  u8;
typedef unsigned short u16;

//基本应答信息定义
#define FAIL                                        0xCD  // 程序表示错误值
#define ACK_SUCCESS                       0x00   //
#define ACK_FAIL                               0x01   // finger 表示错误值
#define ACK_FULL                              0x04   // 指纹数据库已满
#define ACK_NO_USER		                 0x05   // 无此用户
#define ACK_USER_OCCUPIED         0x06   // 用户已存在
#define ACK_FINGER_OCCUPIED     0x07   // 指纹已存在
#define ACK_TIMEOUT                       0x08   // 采集超时

//用户信息定义
#define ACK_ALL_USER                  0x00
#define ACK_NORMAL_USER         0x01
#define ACK_ADMIN_USER 	          0x02
#define ACK_AUTHO_USER            0x03
#define USER_MAX_CNT	              0xFFF

//命令定义
#define CMD_HEAD		  0xF5
#define CMD_TAIL		      0xF5
#define CMD_ADD_1		  0x01  // 第1次扫描
#define CMD_ADD_2 		  0x02  // 第2次扫描
#define CMD_ADD_3  	  0x03  // 第3次扫描
#define CMD_MATCH	  0x0C  // 1:N 匹配
#define CMD_DEL			  0x04  // 删除指定用户
#define CMD_DEL_ALL	       0x05  // 删除所有用户
#define CMD_USER_CNT      0x09  // 取用户总数

extern volatile int g_exit ;
extern volatile int isMatching;

void printArr(u8 *hand_ack);
int  ComWriteCmd(u8 *cmd);
int  ComReadAck(u8 *ack);
u8  TxAndRsCmd();
u8  MatchFinger();
u8 AddUser(u16 id,  u8 UserGroup);
u8 DeleteUser(u16  UserID);
u8 DeleteAllUser(u8 UserGroup);   //  0 all   // 1, 2, 3
u16 GetUsersCnt();
u16 GetUserId();
void  ParseInfo(char *szInfo);

#endif
