//
//  CPreprocess.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CPreProcess__
#define __DownloadApp__CPreProcess__

#include <stdio.h>
#include "Internal.h"
#include "ActiveObj.h"
#include "CLock.h"

class CDownload;
//typedef struct PreprocessorList_t
//{
//    struct PreprocessorList_t* _next;
//    CDownload* _downloadinfo;
//    int _index;
//    SocketDownloadInfo* _cursockinfo;
//    unsigned long _lastconnecttime;
//    int _retrycount;
//} PreprocessorList;


class CPreprocess //: public ActiveObj
{
public:
    CPreprocess();
    ~CPreprocess();
//    static CPreprocess* Instance();
//    void InitPreprocessEnv(void);
    void PutDownloadInfoToPreprocess(CDownload* downloadinfo, int index, SocketDownloadInfo *sockinfo);
//    void *PreprocessThread(void * param);
//    void ExitPreprocessThread();

    
private:
    //begin active obj virtual function
//    virtual void _OnThreadStart(); //主动对象线程启动回调
//    virtual void _OnRequest(void* reqParam);
//    virtual void _OnThreadEnd(); //主动对象线程结束回调
    //end active obj virtual function

private:
//    static CLock _lock;
};
#endif /* defined(__DownloadApp__CPreProcess__) */
