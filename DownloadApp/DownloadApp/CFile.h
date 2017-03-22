//
//  CFile.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CFile__
#define __DownloadApp__CFile__

#include <stdio.h>
#include "Internal.h"

//typedef struct PosInfo_t
//{
//    unsigned long offset;
//    unsigned long end;
//}PosInfo;
//
//typedef struct DownloadLogInfo_t{
//    unsigned char md5[MD5_VALUE_LEN];//文件md5
//    size_t filesize; //文件大小
//    size_t blocksize; //块大小
//    size_t dltotalbytes; //总下载大小
//    PosInfo position[MAX_DOWNLOAD_TASK]; //每个下载块的偏移量及结束位置
//}DownloadLogInfo;
//
//typedef struct DlFileInfo_t{
//    char sfilename[MAX_PATH]; //保存文件名
//    char cdlfilename[MAX_PATH]; //中间文件名
//    char clogfilename[MAX_PATH]; //日志文件名
//    
//    //------------------------------------------
//    FILE* cdlfp;
//    
//    size_t curmaxend;//当前下载块里面最大结束位置
//    
//    unsigned long logcount;
//    //-----------------------MD5_VALUE_LEN------------------
//    unsigned char logmd5[MD5_VALUE_LEN];
//    DownloadLogInfo loginfo;
//}DlFileInfo;
class CDownload;
class CDlFileInfo
{
public:
    CDlFileInfo();
    ~CDlFileInfo();
    
    int LoadDownloadFileInfo(const char* filename, const unsigned char* md5, size_t filesize);
    //返回1：下载完成并成功将数据写入文件； 返回0：文件未下载完成但是成功将数据信息写入文件； 小于0：数据写入时出现错误
    int WriteDataToFile(CDownload& download, DataBlockInfo* datablock);
//    int WriteDataToFile(DataBlockInfo* datablock);
    int CloseDownloadFile(int needremovefile);
    
    char* GetSFilePath();
    char* GetCDLFilePath();
    char* GetLogFilePath();

private:
    FILE* _CreateFile(const char * filename, size_t filelen);
    int _LoadDownloadLogInfo(const char* dllogfilename, const unsigned char* filemd5/*DownloadLogInfo代替 , unsigned char *buffer*/);
    
    //更新下载日志文件
    int _RefreshLogfile();
    //计算下一块的下载范围
    int _SetNextDownloadPos(PosInfo *curposinfo);

public:
    char _sfilename[MAX_PATH]; //保存文件名
    char _cdlfilename[MAX_PATH]; //中间文件名
    char _clogfilename[MAX_PATH]; //日志文件名
    
    FILE* _cdlfp; //中间文件句柄
    
    size_t _curmaxend;//当前下载块里面最大结束位置
    
    unsigned long _logcount;
    
    //-----------------------MD5_VALUE_LEN------------------
    unsigned char _logmd5[MD5_VALUE_LEN];
    DownloadLogInfo _loginfo;
};


#endif /* defined(__DownloadApp__CFile__) */
