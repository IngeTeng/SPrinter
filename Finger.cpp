//////////////////////////////
#include "Finger.h"
#include "stdio.h"
#include "unistd.h"
#include "memory.h"
#include "fcntl.h"
#include "globals.h"

u8  hand_send_cmd[8] = { 0xF5, CMD_ADD_1, 1, 0, 1, 0, 0, 0xF5 };
u8  hand_ack[8] = { 0 };

void printArr(u8 *hand_ack)
{
	for (int i = 0; i < 8; i++)
		if(debug) printf("%1x ", hand_ack[i]);
	if(debug) printf("\n");
}

int ComWriteCmd(u8 *cmd)
{
    write(seri_fd5.fd,  cmd,  8);
}
int ComReadAck(u8 *ack)
{
	u8 val = 0;

	while (val != 0xF5 && seri_fd5.fd != -1)
	{
	      fd_set  fs;
	      FD_ZERO(&fs);   FD_SET(seri_fd5.fd,  &fs);  timeval  tv= {1, 0};
	      if(select(seri_fd5.fd +1,  &fs, 0, 0, &tv) > 0) read(seri_fd5.fd, &val,  1);
    }
	ack[0] = val;
	if(seri_fd5.fd > -1)	read(seri_fd5.fd, ack + 1,  7);
}
/******************串口发送子程序bit SendUART(U8 Scnt,U8 Rcnt,U8 Delay)******/
/*功能：向DSP发送数据********************************************************/
/*参数：Scnt发送除去头尾以及校验位的有效字节数；Rcnt接收字节数； *********************/
/*返回值：TRUE 成功；FALSE 失败*******************************************************/
u8  TxAndRsCmd()
{
	const u8 Scnt = 5, Rcnt = 8;
	u8 i, j, checksum;
	checksum = 0;//校验位 表示第二字节到第六字节的亦或
	for (i = 1; i <= Scnt; i++) checksum ^= hand_send_cmd[i];
	hand_send_cmd[6] = checksum;

	ComWriteCmd(hand_send_cmd);
	memset(hand_ack, 0, sizeof(hand_ack));
	ComReadAck(hand_ack);  printArr(hand_ack);

	if (hand_ack[0] != CMD_HEAD) return FAIL;
	if (hand_ack[7] != CMD_TAIL) return FAIL;
	if (hand_ack[1] != hand_send_cmd[1]) return FAIL;

	checksum = 0;
	for (j = 1; j < Rcnt - 1; j++) checksum ^= hand_ack[j];
	if (checksum != 0) return FAIL;

	return hand_ack[4];
}

/******************指纹比对子程序U8 CompFinger(void)*************************/
/*功能：采集指纹与库内指纹比对,方式为1：N ***********************************/
/*参数：无 ******************************************************************/
/*返回值：ACK_SUCCESS成功；ACK_FAIL失败*******************************************/
u8  MatchFinger()
{
	//printf("ismatch\n");
	hand_send_cmd[0] = 0xF5;	hand_send_cmd[1] = 0x0C; 	hand_send_cmd[2] = 0; hand_send_cmd[3] = 0; hand_send_cmd[4] = 0;
	hand_send_cmd[5] = 0;       hand_send_cmd[6] = 0;       hand_send_cmd[7] = 0xF5;
	if(op.cmd == DELUSER) isMatching = 1;
	u8 m = TxAndRsCmd();
	isMatching = 0;
	if (m != FAIL)
	{
		if (hand_ack[4] == ACK_NO_USER)  return ACK_NO_USER;
		else if(hand_ack[4]  == ACK_TIMEOUT)  return ACK_TIMEOUT;
		else if(hand_ack[4] > 0 )	{	printArr(hand_ack);    return ACK_SUCCESS; }
	}
	return FAIL;
}

u16 GetUserId()
{
       return (hand_ack[2] << 8) +  hand_ack[3];
}
/******************指纹存储子程序**********************/
/*功能：存储用户指纹 需要连续存储三次***********************************/
/*参数：id 用户指纹存储编号 permission指定用户权限 **************/
/*返回值：ACK_SUCCESS成功，ACK_FAIL失败**************************/
u8 AddUser(u16 id, u8 permission)
{
	while (1)  // 无限尝试
	{
		if(debug) printf("add\n");
		hand_send_cmd[1] = CMD_ADD_1; hand_send_cmd[2] = id >> 8; hand_send_cmd[3] = id & 0xFF; hand_send_cmd[4] = permission;
		u8 m = TxAndRsCmd();                     // 第一次添加
		if (hand_ack[4]) return hand_ack[4];

		if (m == ACK_SUCCESS && hand_ack[4] == ACK_SUCCESS)
		{
			hand_send_cmd[1] = CMD_ADD_2;
			m = TxAndRsCmd();
			if (m == ACK_SUCCESS && hand_ack[4] == ACK_SUCCESS)
			{
				hand_send_cmd[1] = CMD_ADD_3;
				m = TxAndRsCmd();
				if (m == ACK_SUCCESS && hand_ack[4] == ACK_SUCCESS)	return ACK_SUCCESS;
			}
		}
		if (g_exit) break;
	}
	//失败
	return ACK_FAIL;
}

/******************指纹指定删除子程序**********************/
/*功能：删除用户指纹 *******************************/
/*参数：id 用户指纹存储编号  **************/
/*返回值：ACK_SUCCESS成功，ACK_FAIL失败****/
u8 DeleteUser(u16 id)
{
	u8 idLow = id & 0xff;
	u8 idHigh = id >> 8;
	//	strcpy(hand_send_cmd, "0xF5,0x04,0,0,0,0,0,0xF5");
	hand_send_cmd[0] = 0xF5;	hand_send_cmd[1] = 0x04; 	hand_send_cmd[2] = idHigh;  	hand_send_cmd[3] = idLow;   hand_send_cmd[4] = 0;
	hand_send_cmd[5] = 0;       hand_send_cmd[6] = 0;       hand_send_cmd[7] = 0xF5;
	u8 m = TxAndRsCmd();
	return m;
}

/******************删除所有用户子程序**********************/
/*功能：删除权限用户或所有用户 *******************************/
/*参数：permission 指定用户权限删除  **************/
/*返回值：ACK_SUCCESS成功，ACK_FAIL失败****/
u8 DeleteAllUser(u8 permission)
{
	hand_send_cmd[0] = 0xF5;	hand_send_cmd[1] = 0x05; 	hand_send_cmd[2] = 0; hand_send_cmd[3] = 0; hand_send_cmd[4] = permission;//0全部用户，1/2/3删除权限为1/2/3的全部用户
	hand_send_cmd[5] = 0;       hand_send_cmd[6] = 0;       hand_send_cmd[7] = 0xF5;
	return TxAndRsCmd();
}


/*功能：用户总数
  参数：
  返回值： 用户数 */
u16 GetUsersCnt()
{

	hand_send_cmd[0] = 0xF5;	hand_send_cmd[1] = 0x09; 	hand_send_cmd[2] = 0; hand_send_cmd[3] = 0; hand_send_cmd[4] = 0;
	hand_send_cmd[5] = 0;       hand_send_cmd[6] = 0;       hand_send_cmd[7] = 0xF5;
	u8 ret = TxAndRsCmd();
	if (ret == ACK_SUCCESS)
	{
		if(debug) printf("user num=%d\n",hand_ack[3] + (hand_ack[2] << 8));
		return hand_ack[3] + (hand_ack[2] << 8);
	}

	return 0;
}

void  ParseInfo(char *szInfo)
{
      if(hand_ack[0] == 0xF5 && hand_ack[7] == 0xF5)
      switch(hand_ack[4])
      {
            case ACK_NO_USER:  sprintf(szInfo, "%s", "错误：无此用户");   break;
            case ACK_TIMEOUT:  sprintf(szInfo, "%s", "错误：超时");          break;
            case ACK_FINGER_OCCUPIED:   sprintf(szInfo, "%s", "错误:指纹已存在");  break;
            case ACK_USER_OCCUPIED: sprintf(szInfo, "%s", "错误:ID已存在");    break;
            case FAIL: sprintf(szInfo, "%s", "通信错误");    break;
            case ACK_SUCCESS: sprintf(szInfo, "%s", "操作成功");    break;
            default:  szInfo[0]  = 0;   break;
      }
}
