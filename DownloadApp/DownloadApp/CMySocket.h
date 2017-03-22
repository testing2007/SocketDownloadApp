//
//  CMySocket.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CMySocket__
#define __DownloadApp__CMySocket__

#include <stdio.h>
#include "Internal.h"

class CMySocket
{
public:
    CMySocket();
    ~CMySocket();
    
    bool Connect(const char* host, int port);
    int SendData(unsigned char* buf, int len);
    int ReceiveData(unsigned char* buf, int len);
    int Close();
    
    int SetNonBlock();
    int SetBlock();
    
    SOCKET _socketid;
};

#endif /* defined(__DownloadApp__CMySocket__) */
