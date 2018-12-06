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
/******************���ڷ����ӳ���bit SendUART(U8 Scnt,U8 Rcnt,U8 Delay)******/
/*���ܣ���DSP��������********************************************************/
/*������Scnt���ͳ�ȥͷβ�Լ�У��λ����Ч�ֽ�����Rcnt�����ֽ����� *********************/
/*����ֵ��TRUE �ɹ���FALSE ʧ��*******************************************************/
u8  TxAndRsCmd()
{
	const u8 Scnt = 5, Rcnt = 8;
	u8 i, j, checksum;
	checksum = 0;//У��λ ��ʾ�ڶ��ֽڵ������ֽڵ����
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

/******************ָ�Ʊȶ��ӳ���U8 CompFinger(void)*************************/
/*���ܣ��ɼ�ָ�������ָ�Ʊȶ�,��ʽΪ1��N ***********************************/
/*�������� ******************************************************************/
/*����ֵ��ACK_SUCCESS�ɹ���ACK_FAILʧ��*******************************************/
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
/******************ָ�ƴ洢�ӳ���**********************/
/*���ܣ��洢�û�ָ�� ��Ҫ�����洢����***********************************/
/*������id �û�ָ�ƴ洢��� permissionָ���û�Ȩ�� **************/
/*����ֵ��ACK_SUCCESS�ɹ���ACK_FAILʧ��**************************/
u8 AddUser(u16 id, u8 permission)
{
	while (1)  // ���޳���
	{
		if(debug) printf("add\n");
		hand_send_cmd[1] = CMD_ADD_1; hand_send_cmd[2] = id >> 8; hand_send_cmd[3] = id & 0xFF; hand_send_cmd[4] = permission;
		u8 m = TxAndRsCmd();                     // ��һ�����
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
	//ʧ��
	return ACK_FAIL;
}

/******************ָ��ָ��ɾ���ӳ���**********************/
/*���ܣ�ɾ���û�ָ�� *******************************/
/*������id �û�ָ�ƴ洢���  **************/
/*����ֵ��ACK_SUCCESS�ɹ���ACK_FAILʧ��****/
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

/******************ɾ�������û��ӳ���**********************/
/*���ܣ�ɾ��Ȩ���û��������û� *******************************/
/*������permission ָ���û�Ȩ��ɾ��  **************/
/*����ֵ��ACK_SUCCESS�ɹ���ACK_FAILʧ��****/
u8 DeleteAllUser(u8 permission)
{
	hand_send_cmd[0] = 0xF5;	hand_send_cmd[1] = 0x05; 	hand_send_cmd[2] = 0; hand_send_cmd[3] = 0; hand_send_cmd[4] = permission;//0ȫ���û���1/2/3ɾ��Ȩ��Ϊ1/2/3��ȫ���û�
	hand_send_cmd[5] = 0;       hand_send_cmd[6] = 0;       hand_send_cmd[7] = 0xF5;
	return TxAndRsCmd();
}


/*���ܣ��û�����
  ������
  ����ֵ�� �û��� */
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
            case ACK_NO_USER:  sprintf(szInfo, "%s", "�����޴��û�");   break;
            case ACK_TIMEOUT:  sprintf(szInfo, "%s", "���󣺳�ʱ");          break;
            case ACK_FINGER_OCCUPIED:   sprintf(szInfo, "%s", "����:ָ���Ѵ���");  break;
            case ACK_USER_OCCUPIED: sprintf(szInfo, "%s", "����:ID�Ѵ���");    break;
            case FAIL: sprintf(szInfo, "%s", "ͨ�Ŵ���");    break;
            case ACK_SUCCESS: sprintf(szInfo, "%s", "�����ɹ�");    break;
            default:  szInfo[0]  = 0;   break;
      }
}
