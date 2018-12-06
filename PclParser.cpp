#include "PclParser.h"
#include <stdio.h>
#include "globals.h"

#define DEBUG

int PclParse(char *p, int bufsize, PCL* pcl)
{
    char terminator;
    int value = 0,  len = 0;

    pcl->cmd = PCL_UNKNOWN;
    if((*p) != PCL_ESCAPTE)
    {
#ifdef DEBUG
    	if(debug) printf("\n buffer  PCL_ESCAPTE\n");    	for(len=0; len<32; len++)   	{ 	if(debug) printf("%x    ",*(p+len));    	}    	if(debug) printf("\n");
#endif
        return -1;
    }

    switch(*(p+1))
    {
        case 'E':
        {
            pcl->len   = 2;
            pcl->cmd = PCL_RESET;
        }
        break;
        case '*':
        {
                switch(*(p+2))
                {
                       case 'b':
                                  if(PclParseOneValueCmd(p+3, bufsize-3, &terminator, &value, &len))      {         return  -1;   }
                                   pcl->len     =  len+3;             pcl->value =  value;             pcl->cmd   =  PCL_RASTER_DATA;//光栅数据开始
                                   break;
                       case 'r':
                                    if(*(p+3)=='C')
                                    {
                                            pcl->len   = 4;      pcl->cmd = PCL_RASTER_END;//光栅数据结束
                                     }
                                     else
                                     {
                                                if(PclParseOneValueCmd(p+3, bufsize-3, &terminator, &value, &len))     {    return -1;   }
                                                else
                                                {
                                                    pcl->len     =  len+3;         pcl->value = value;
                                                    switch(terminator)
                                                    {
                                                        case 'S':       pcl->cmd=PCL_RASTER_WIDTH;         break;
                                                        case 'T':       pcl->cmd=PCL_RASTER_HEIGHT;       break;
                                                        case 'A':       pcl->cmd=PCL_RASTER_START;        break;
                                                        case 'U':       pcl->cmd=PCL_COLOR_MODE;           break;
                                                    }
                                                }
                                        }
                                        break;
                           case 't':
                                        if(PclParseOneValueCmd(p+3,bufsize-3,&terminator,&value,&len))      { return -1;  }
                                        pcl->len = len+3;          pcl->value = value;
                                         if(terminator=='R')
                                        {
                                            pcl->cmd = PCL_RESOLUTION;
                                        }
                                        break;
                       }
                  break;
              }
         case '&':
         {
                if ( *(p+2) ==  'l'  )
                {
                            if(PclParseOneValueCmd(p+3,  bufsize-3,  &terminator,  &value,  &len))    {       return -1;      }
                            else
                            {
                                pcl->len = len+3;          pcl->value = value;
                                switch(terminator)
                                {
                                        case 'X':         pcl->cmd = PCL_COPIES;             break;
                                        case 'A':         pcl->cmd = PCL_PAGE_SIZE;          break;
                                        case 'H':         pcl->cmd = PCL_PAPER_SRC;          break;
                                        case 'S':          pcl->cmd = PCL_PAPER_COPY;        break;
                                        case 'T':         pcl->cmd  = PCL_PAPER_CMBMODE;     break;      // Esc&l#T  多合1打印：0 禁用， 2 4 6 9 16 合1可选
                                }
                            }
                }
                else if ( *(p+2) == 'a' )
                {
                         if(PclParseOneValueCmd(p+3,  bufsize-3,  &terminator,  &value,  &len))         {         return -1;        }
                         else
                         {
                                pcl->len = len+3;                               pcl->value = value;
                                 switch(terminator)
                                {
                                 case 'G':          pcl->cmd = PCL_SINGLE_DOUBLE;   break;   // Esc&a#G 单双面打印模式： 0单， 1长边翻转， 2 短边翻转
                               }
                         }
               }
         break;
        }
        case '%':
        {
                char szUid[6] = { 0 };    snprintf(szUid,  6,  "%s",  p+3);     curPrnPara.nUserId = atoi(szUid);    if(debug) printf(" Uid: %d\n",  curPrnPara.nUserId );  // 获得用户ID
                pcl->len = 9;        pcl->cmd = PCL_EXIT;
        }
        break;
    }
#ifdef DEBUG
    if(pcl->cmd == PCL_UNKNOWN)
    {
    	if(debug) printf("\n buffer PCL_UNKNOWN \n");   	for(len = 0;  len<32; len++) 	{ 	if(debug) printf("%x    ",*(p+len));   	}    	if(debug) printf("\n");
    }
#endif
    return 0;
}

// len -- the total len from p to the terminator,解析从p到terminator之间的数据
int PclParseOneValueCmd(char *p, int bufsize, char *terminator, int*val, int*len)
{
    int id = 0;
    char *start = p;
    char buf[8] = {0};
    while((((*p) <= 0x39&&(*p) >= 0x30)||(*p) == 0x2d) && id < bufsize)      //0-9 or '-'
    {
        p++;        id++;
    }
    if(id == bufsize)
    {
        return -1;
    }
    else
    {
        *terminator = *p;
        memcpy(buf, start, id);
        *val = atoi(buf);
        *len = id+1;
    }

    return 0;
}


