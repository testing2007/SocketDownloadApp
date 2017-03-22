//
//  CUtil.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CUtil__
#define __DownloadApp__CUtil__

#include <stdio.h>
#include "CMD5.h"

void DEBUGOUT(const char*format, ...);

//////////////////////////////////////////
unsigned long  GetTime();

void Sleep(unsigned int ms);

//md5数据转换为字符串
void GetMD5String(md5_byte_t digest[16], unsigned char *strmd5);
//获取md5值
int GetBuffMD5Value(const char* buffer, int count, unsigned char *md5);
//获取文件的md5值
int GetFileMD5Value(const char* sfilename, unsigned char *md5);

#endif /* defined(__DownloadApp__CUtil__) */
