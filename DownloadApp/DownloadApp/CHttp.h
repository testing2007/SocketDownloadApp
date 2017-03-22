//
//  CHttp.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CHttp__
#define __DownloadApp__CHttp__

#include <stdio.h>
#include "CMySocket.h"
#include "Internal.h"
#include "SmartPtr.h"

class CHttp
{
public:
    CHttp(const char* URL);
    CHttp(struct url_t* url_t);
    ~CHttp();
    
    size_t GetContentLength(const char* cookie);

    //功能：发出断点续传请求，并将返回获取到的数据填充到缓冲区中，同时返回缓冲区数据长度
    //返回值：成功返回0， 否则返回-1，
    int RequestStartDownload(CMySocket **mysocket,
                             const char* host,
                             const char* cookie,
                             PosInfo *posinfo,
                             char *buff,
                             int* len);


private:
    int _GetResponseStatusCode(const char* httphead, char** ptr);
    char* _GetRedirectLocaltion(const char *httphead);
    size_t _GetHttpResponsePackLength(const char* httphead);
    
    int _ParserUrl(const char* URL);
    void _ReleaseUrl();

public:
    int n;
    struct url_t* _urlinfo;
    
};

#endif /* defined(__DownloadApp__CHttp__) */
