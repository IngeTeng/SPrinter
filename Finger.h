#ifndef _FINGER_H_
#define _FINGER_H
//////////////////////////////
#include "Serial.h"

extern  serial_arg   seri_fd5;
typedef unsigned char  u8;
typedef unsigned short u16;

//����Ӧ����Ϣ����
#define FAIL                                        0xCD  // �����ʾ����ֵ
#define ACK_SUCCESS                       0x00   //
#define ACK_FAIL                               0x01   // finger ��ʾ����ֵ
#define ACK_FULL                              0x04   // ָ�����ݿ�����
#define ACK_NO_USER		                 0x05   // �޴��û�
#define ACK_USER_OCCUPIED         0x06   // �û��Ѵ���
#define ACK_FINGER_OCCUPIED     0x07   // ָ���Ѵ���
#define ACK_TIMEOUT                       0x08   // �ɼ���ʱ

//�û���Ϣ����
#define ACK_ALL_USER                  0x00
#define ACK_NORMAL_USER         0x01
#define ACK_ADMIN_USER 	          0x02
#define ACK_AUTHO_USER            0x03
#define USER_MAX_CNT	              0xFFF

//�����
#define CMD_HEAD		  0xF5
#define CMD_TAIL		      0xF5
#define CMD_ADD_1		  0x01  // ��1��ɨ��
#define CMD_ADD_2 		  0x02  // ��2��ɨ��
#define CMD_ADD_3  	  0x03  // ��3��ɨ��
#define CMD_MATCH	  0x0C  // 1:N ƥ��
#define CMD_DEL			  0x04  // ɾ��ָ���û�
#define CMD_DEL_ALL	       0x05  // ɾ�������û�
#define CMD_USER_CNT      0x09  // ȡ�û�����

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
