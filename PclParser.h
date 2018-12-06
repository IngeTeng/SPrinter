#ifndef PCLPARSER_H_INCLUDED
#define PCLPARSER_H_INCLUDED
#include "Pcl.h"
#include <string.h>
#include <stdlib.h>

int PclParse(char *p,int bufsize,PCL * pcl);
int PclParseOneValueCmd(char *p,int bufsize,char *terminator,int*val,int*len);

#endif // PCLPARSER_H_INCLUDED
