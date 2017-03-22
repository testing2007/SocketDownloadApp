//
//  CDownload.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CDownload__
#define __DownloadApp__CDownload__

#include <stdio.h>
#include "Internal.h"
#include "ActiveObj.h"
#include "CFile.h"
#include <list>
#include <sys/_types/_fd_def.h>

#include "CPreprocess.h"
#include "CDataProcess.h"
#include "CPostprocess.h"
#include "CMemPool.h"

class CDownload
{
public:
    
    void _CleanWorker();
    
    //  获取当前下载速度
    //  返回值：最后的设置的结果值
    unsigned long GetCurSpeed();
    
    //  设置http请求头中的User-Agent字段值
    //  参数：
    //  agentinfo：User-Agent字段值
    void SetUserAgent(const char *agentinfo);
    
    
    //  获取下载的文件尺寸大小
    //  返回值：文件大小，如果失败返回0
    size_t GetDownloadFileSize();
    
    //启动下载
    void StartDownloadFile();
    
    //  停止下载
    //  返回值：成功返回0，异常返回－1
    int StopDownloadFile(); //todo: StopDownloadFile与CloseDownload区别是什么
    
    //  恢复下载
    //  返回值：成功返回0，异常返回－1
    int ResumeDownloadFile();
    
    //  获取当前的下载状态
    //  返回值：下载的状态
    enum DLSTATUS GetCurDownloadStatus();
    
    //  关闭下载
    //  参数：
    //  needremovefile：是否需要删除任务
    //  返回值：成功返回0，异常返回－1
    int CloseDownload(int needremovefile);
    
    //  获取下载保存的路径文件名
    //  参数：
    //  filename：文件路径名指针
    //  返回值：成功返回0，错误返回－1
    int GetDownloadFilePathname(char** filename);
    
    //  发起一个http请求
    //  参数：
    //  URL：请求的URL
    //  postparaminfo：post参数信息，如果该参数为NULL，将使用get方式进行请求
    //  agentflag：请求头中是否包括User-Agent字段
    //  errcode：错误码，请求错误，将通过此参数返回错误码
    //  返回值：成功返回http的响应数据，不包括头信息，该内存资源需要用户进行释放，错误返回NULL
    char* GetUrlRespondContent(const char *URL, const char* postparaminfo, int agentflag, int* errcode);
    
    //  获取文件的md5值
    //  参数：
    //  sfilename：源文件名
    //  md5：文件的md5值，空间由用户自己分配。
    //  返回值：成功返回0，错误返回-1
    int GetFileMD5Value(const char* sfilename, unsigned char *md5);
    
    //  计算buffer的md5值
    //  参数：
    //  buffer：数据指针
    //  count：数据长度
    //  md5：计算出来的md5值，空间由用户自己分配。
    //  返回值：成功返回0，错误返回-1
    int GetBuffMD5Value(const char* buffer, int count, unsigned char *md5);
    
    //  base64加密
    //  参数：
    //  to：加密后的数据(内存空间由用户自己分配)
    //  from：加密前的数据
    //  dlen：数据长度
    //  返回值：成功返回加密后的数据长度，失败返回-1
    int EncodeBase64Block(unsigned char *to, const unsigned char *from, int dlen);
    
    //  base64解密
    //  参数：
    //  to：解密后的数据(内存空间由用户自己分配)
    //  from：加密前的数据
    //  n：数据长度
    //  返回值：成功返回解密后的数据长度，失败返回-1
    int DecodeBase64Block(unsigned char *to, const unsigned char *from, int n);

private:
    static void* _DownloadThread(void* param);
    void _DoDownload();
    SOCKET _PutDownloadSocket(fd_set *fdset);
    
    //获取每个时段因为限速可以读取的数据量
    long _GetLimitReadBytes(int curreadcount);

    //从socket 集中读取下载的数据。needreadbytes是最多可以读取的数据量
    int _GetDownloadData(fd_set *fdset, unsigned int needreadbytes);

public:
//    DownloadInfo _downloadInfo;
//    CDownload* _next;
//    int _refcount;//
    DLSTATUS _status;//DLS_*
    int _needremovefile; //是否需要删除中间文件及日志文件
    int _invalid; //标识此节点是否有效
    
    //----------------------------------
    NotifyDownloadStatus _notify;//用户下载时传入过来的回调函数
    void* _usrdata; //用户下载时传入过来的参数
    unsigned long _intervalsec;
    unsigned long _lastnotifysec;
    
    //---------------------------------
    struct url_t _urlinfo;
    const char* _cookie;
    
    SocketDownloadInfo _dltask[MAX_DOWNLOAD_TASK];
    
    CDlFileInfo _dlfileinfo;
    unsigned long  _curspeed;
    CMemPool* _datablockmempool;

    pthread_t _thread;//执行下载数据线程
    
    CPreprocess _preprocess;
    CDataProcess _dataprocess;
    CPostprocess _postprocess;
};

#endif /* defined(__DownloadApp__CDownload__) */
