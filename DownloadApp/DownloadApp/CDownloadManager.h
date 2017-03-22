//
//  CDownloadManager.h
//  DownloadApp
//
//  Created by 魏志强 on 15/9/9.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CDownloadManager__
#define __DownloadApp__CDownloadManager__

#include <stdio.h>
#include "Internal.h"
#include "ActiveObj.h"
#include "CFile.h"
#include <list>
#include <sys/_types/_fd_def.h>
#include "CLock.h"
#include "CMemPool.h"

class CDownload;
class CDownloadManager : public ActiveObj
{
public:
    static CDownloadManager* Instance();
    //  下载模块初始化接口
    //  参数：
    //  speedbytes：下载模块限速值，单位字节
    //  返回值：成功返回0，错误返回非0
    int InitDownload(unsigned long speedbytes);
    
    //设置最大同时下载任务数量，最后决定范围 1~MAX_DOWNLOAD_TASK
    void SetMaxTaskNumber(int number);
    
    //  设置下载模块的限速值
    //  参数：
    //  speedbytes：限速值 ，不低于5k/s
    //  返回值：最后的设置的结果值
    unsigned long SetLimitSpeed(unsigned long speedbytes);
    
    //  获取下载模块的限速值
    //  返回值：返回限速值
    unsigned long GetLimitSpeed();
    
    //  设置整体的下载保存路径
    //  参数：
    //  path：保存路径
    //  返回值：成功返回0，错误返回非0
    int SetSavePath(const char *path);
    
    //  获取整体的下载保存路径
    //  返回值：保存路径字符串
    const char* GetSavePath(void);
    
    //callback function define
    //  下载回调函数
    //  参数：
    //  status：当前的下载状态
    //  usrparam：用户注册的参数
    //  curbytes：当前已下载的文件总字节数
    //  filesize：文件总尺寸
    //  返回值：目前没有使用
    typedef int (*NotifyDownloadStatus)(enum DLSTATUS status, void* usrparam, size_t curbytes, size_t filesize);
    
    //download from url
    //  从指定的url下载文件，文件保存在整体的下载文件路径下
    //  参数：
    //  URL：文件下载的URL
    //  md5：文件的md5校验值，没有时传入NULL
    //  cookie：下载文件时使用的cookie值
    //  notify：注册的回调函数
    //  usrdata：注册的用户数据
    //  返回值：如果创建成功，返回下载节点句柄，如果不成功，返回0
    CDownload* DownloadFileFromUrl(const char* URL, const unsigned char* md5, const char* cookie, NotifyDownloadStatus notify, void* usrdata);
    
    //  从指定的url下载文件，文件保存在savepath指定的文件路径下
    //  参数：
    //  URL：文件下载的URL
    //  md5：文件的md5校验值，没有时传入NULL
    //  cookie：下载文件时使用的cookie值
    //  savepath：文件保存路径
    //  notify：注册的回调函数
    //  usrdata：注册的用户数据
    //  返回值：如果创建成功，返回下载节点句柄，如果不成功，返回0
    CDownload* DownloadFileFromUrlEx(const char* URL, const unsigned char* md5, const char* cookie, const char* savepath, NotifyDownloadStatus notify, void* usrdata);
    
protected:
    CDownloadManager();
    ~CDownloadManager();
    
    int _CreateDownloadNode(CDownload** dlinfo, const char* URL, const unsigned char* md5, const char* cookie, const char* savepath, const char *destname, NotifyDownloadStatus notify, void* usrdata);
    
    void _AddNodeToDownloadList(CDownload* dlinfo);
    
private:
    //begin active obj virtual function
    virtual void _OnThreadStart(); //主动对象线程启动回调
    virtual void _OnRequest(void* reqParam);
    virtual void _OnThreadEnd(); //主动对象线程结束回调
    //end active obj virtual function
    
    //释放下载模块资源
    void _ReleaseDownload(void);
    
    //更新下载节点list中的下载节点状态，用于控制同时下载的数目
    void _UpdateDownloadStatus();
    
private:
    char _savepath[MAX_PATH];
    int _curdownloadtaskcount;//当前处于下载中状态的任务数
    int _maxtasknumber;//范围1~MAX_DOWNLOAD_TASK
    unsigned long  _limitspeed;
    static CLock _lock;
    CLock _downloadlock;
    
    CMemPool* _mempool;
    
    std::list<CDownload*> _downloadlist;
};

#endif /* defined(__DownloadApp__CDownloadManager__) */
