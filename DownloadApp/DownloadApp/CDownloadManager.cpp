//
//  CDownloadManager.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/9/9.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CDownloadManager.h"
#include "CDownload.h"
#include "CUtil.h"
#include "CHttp.h"

CLock CDownloadManager::_lock;
CDownloadManager* CDownloadManager::Instance()
{
    static CDownloadManager* instance = NULL;
    if(instance==NULL)
    {
        CAutoLock lock(_lock);
        if(instance==NULL)
        {
            instance = new CDownloadManager();
        }
    }
    
    return instance;
}

CDownloadManager::CDownloadManager()
{
    _mempool = CMemPool::CreateMemPool(20, sizeof(CDownload));
    if(_mempool!=NULL)
    {
        Start();
    }
    //  初始化各个下载线程
    //    CPreprocess::Instance()->InitPreprocessEnv();
    //    CDataProcess::Instance()->InitDataProcessEnv();
    //    CPostprocess::Instance()->InitPostprocessEnv();
}

CDownloadManager::~CDownloadManager()
{
    this->_ReleaseDownload();
}

int CDownloadManager::InitDownload(unsigned long speedbytes)
{
//    DEBUGOUT("build time: %s %s\n", __DATE__, __TIME__);
    
    _curdownloadtaskcount = 0;
    _maxtasknumber = MAX_DOWNLOAD_TASK;
    
    SetLimitSpeed(speedbytes);
    
    memset(_savepath, 0, sizeof(_savepath));
    //_datablockmempool = MemPoolCreate(20, BUFFER_DATA_BLOCK_SIZE, 20);
    //_downloadnodemempool = MemPoolCreate(20, sizeof(DownloadInfo), 20);
    
    sprintf(_savepath, "./");
    
    return 0;
}

void CDownloadManager::_ReleaseDownload(void)
{
    //    CPreprocess::Instance()->Stop();
    //    CDataProcess::Instance()->Stop();
    //    CPostprocess::Instance()->Stop();
    this->Stop();
}

unsigned long CDownloadManager::SetLimitSpeed(unsigned long speedbytes)
{
    if(speedbytes < 1024 * 5 && speedbytes != 0)
    {
        speedbytes = 1024 * 5;
    }
    _limitspeed = speedbytes;
    return speedbytes;
}

//  获取下载模块的限速值
//  返回值：返回限速值
unsigned long CDownloadManager::GetLimitSpeed()
{
    return _limitspeed;
}

void CDownloadManager::SetMaxTaskNumber(int number)
{
    if(number < 1)
    {
        _maxtasknumber = 1;
        return;
    }
    
    if(number > MAX_DOWNLOAD_TASK)
    {
        number = MAX_DOWNLOAD_TASK;
    }
    _maxtasknumber = number;
}

int CDownloadManager::SetSavePath(const char *path)
{
    if(path)
    {
        int len;
        snprintf(_savepath, MAX_PATH, "%s", path);
        len = strlen(_savepath);
#ifdef WIN32
        if(_savepath[len - 1] != '\\')
        {
            _savepath[len] = '\\';
            _savepath[len + 1] = 0;
        }
#else
        if(_savepath[len - 1] != '/')
        {
            _savepath[len] = '/';
            _savepath[len + 1] = 0;
        }
#endif
        DEBUGOUT("%s[%d] g_savepath = \n", __FILE__, __LINE__);
    }
    return 0;
}

const char* CDownloadManager::GetSavePath(void)
{
    return _savepath;
}

CDownload* CDownloadManager::DownloadFileFromUrl(const char* URL,
                                                 const unsigned char* md5,
                                                 const char* cookie,
                                                 NotifyDownloadStatus notify,
                                                 void* usrdata)
{
    while(!_brunning)
    {
        Sleep(100);//直到线程运行起来
    }

    CDownload* dlinfo = NULL;
    int iret;
    
    if(URL == NULL || URL[0] == 0 || notify == NULL)
    {
        return 0;
    }
    
    //  创建下载节点
    iret = _CreateDownloadNode(&dlinfo, URL, md5, cookie, NULL, NULL, notify, usrdata);

    if(iret != DL_ERRCODE_SUCCESS)
    {
        assert(dlinfo == NULL);
        DEBUGOUT("%s[%d] CreateDownloadNode error!\n", __FUNCTION__, __LINE__);
        return 0;
    }
    
    dlinfo->_notify = notify;
    dlinfo->_usrdata = usrdata;
    //    dlinfo->_next = NULL;
    
    //  已经下载过该文件
    if(dlinfo->_status == DLS_COMPLETE)
    {
        notify(DLS_COMPLETE, usrdata, dlinfo->_dlfileinfo._loginfo._filesize, dlinfo->_dlfileinfo._loginfo._filesize);
    }
    else
    {
        notify(DLS_ALREADY, usrdata, 0, dlinfo->_dlfileinfo._loginfo._filesize);
    }
    
    //加入到下载节点list中进行管理
    _AddNodeToDownloadList(dlinfo);
    return dlinfo;
}

CDownload* CDownloadManager::DownloadFileFromUrlEx(const char* URL,
                                                   const unsigned char* md5,
                                                   const char* cookie,
                                                   const char* savepath,
                                                   NotifyDownloadStatus notify,
                                                   void* usrdata)
{
    while(!_brunning)
    {
        Sleep(100);//直到线程运行起来
    }

    CDownload* dlinfo = NULL;
    int iret;
    
    if(URL == NULL || URL[0] == 0 || notify == NULL)
    {
        return 0;
    }
    
    //  创建下载节点
    iret = _CreateDownloadNode(&dlinfo, URL, md5, cookie, NULL, NULL, notify, usrdata);
    
    if(iret != DL_ERRCODE_SUCCESS)
    {
        assert(dlinfo == NULL);
        DEBUGOUT("%s[%d] CreateDownloadNode error!\n", __FUNCTION__, __LINE__);
        return 0;
    }
    
    dlinfo->_notify = notify;
    dlinfo->_usrdata = usrdata;
    //    dlinfo->_next = NULL;
    
    //  已经下载过该文件
    if(dlinfo->_status == DLS_COMPLETE)
    {
        notify(DLS_COMPLETE, usrdata, dlinfo->_dlfileinfo._loginfo._filesize, dlinfo->_dlfileinfo._loginfo._filesize);
    }
    else
    {
        notify(DLS_ALREADY, usrdata, 0, dlinfo->_dlfileinfo._loginfo._filesize);
    }
    
    //加入到下载节点list中进行管理
    _AddNodeToDownloadList(dlinfo);
    return dlinfo;
}

//begin active obj virtual function
/*virtual*/ void CDownloadManager::_OnThreadStart()
{
}

/*virtual*/ void CDownloadManager::_OnRequest(void* reqParam)
{
    //DEBUGOUT("%s[%d]  Start DownloadThread\n", __FUNCTION__, __LINE__);
    assert(reqParam!=NULL);
    CDownload* download = (CDownload*)reqParam;
    _downloadlist.push_back(download);

    while(_curdownloadtaskcount>=_maxtasknumber)
    {
        Sleep(100);
    }
    
    _UpdateDownloadStatus();
    download->StartDownloadFile();
}
/*virtual*/void CDownloadManager::_OnThreadEnd()
{
    Sleep(20);
    //    int timeoutcount = 0;
    //    while(timeoutcount < 10)
    //    {
    //        if(!CPreprocess::Instance()->Isrunning() &&
    //           !CDataProcess::Instance()->Isrunning() &&
    //           !CDataProcess::Instance()->Isrunning())
    //        {
    //            break;
    //        }
    //        Sleep(100);
    //        timeoutcount++;
    //    }
    
    //DEBUGOUT("%s[%d] thread exit\n", __FUNCTION__, __LINE__);
    //释放回收资源
//    {
//        CAutoLock lock(_downloadlock);
//        while(_head)
//        {
//            int i;
//            CDownload *tmpdlinfo = (CDownload*)_head;
//            tmpdlinfo->StopDownloadFile();
//            _head = ((MemoryItemNode*)tmpdlinfo)->_next;
//            _mempool->PutMemoryPool(tmpdlinfo);
//        }
//        _head = NULL;
//        _tail = NULL;
//        _mempool->ReleaseMemPool();
//    }
    DEBUGOUT("%s[%d] thread exit\n", __FUNCTION__, __LINE__);
}
//end active obj virtual function

int CDownloadManager::_CreateDownloadNode(CDownload** dlinfo,
                                          const char* URL,
                                          const unsigned char* md5,
                                          const char* cookie,
                                          const char* savepath,
                                          const char *destname,
                                          NotifyDownloadStatus notify,
                                          void* usrdata)
{
    CDownload* ptrdownloadinfo;
    //    struct url_t urlinfo;
    int iret;
    size_t filesize;
    char pathname[MAX_PATH] = {0};
    int i;
    int ifileexist = 0;
    
    *dlinfo = NULL;
    //  解析url下载地址
    CHttp http(URL);
    if(http._urlinfo == NULL)
    {
        DEBUGOUT("%s[%d] ParseAppFileInfo error!\n", __FUNCTION__, __LINE__);
        return DL_ERRCODE_URL_ERR;
    }
    
    //    if(g_exitflag)
    //    {
    //        return DL_ERRCODE_SUCCESS;
    //    }
    
    //获取下载内容的文件大小
    filesize = http.GetContentLength(cookie);
    //    if(g_exitflag)
    //    {
    //        return DL_ERRCODE_SUCCESS;
    //    }
    
    if(filesize == -1)
    {
        DEBUGOUT("%s[%d] Get Url error [%s]\n", __FUNCTION__, __LINE__, http._urlinfo->_doc);
        //        ReleaseUrl(&urlinfo);
        return DL_ERRCODE_URL_ERR;
    }
    
    if(filesize == 0)
    {
        DEBUGOUT("%s[%d] Can't GetContentLength net error  filesize = %d!\n", __FUNCTION__, __LINE__, filesize);
        //        ReleaseUrl(&urlinfo);
        return DL_ERRCODE_NET_ERR;
    }
    
    //构建下载存储路径
    if(destname)
    {
        if(savepath)
        {
            snprintf(pathname, MAX_PATH, "%s%s", savepath, destname);
        }
        else
        {
            snprintf(pathname, MAX_PATH, "%s%s", _savepath, destname);
        }
    }
    else
    {
        if(savepath)
        {
            snprintf(pathname, MAX_PATH, "%s%s", savepath, http._urlinfo->_filename);
        }
        else
        {
            snprintf(pathname, MAX_PATH, "%s%s", _savepath, http._urlinfo->_filename);
        }
    }
    
    //DEBUGOUT("%s[%d], LoadDownloadFileInfo start time = %d", __FUNCTION__, __LINE__, GetTime());
    
    //    加载续传文件
    CDlFileInfo dlFileInfo;
    ifileexist = dlFileInfo.LoadDownloadFileInfo(pathname, md5, filesize);
    if(ifileexist < 0)
    {
        DEBUGOUT("%s[%d] iret = %d\n", __FUNCTION__, __LINE__, iret);
        if(notify)
        {
            notify(DLS_CREATEFILEERR, usrdata, filesize, filesize);
        }
        //        ReleaseUrl(&urlinfo);
        return DL_ERRCODE_FILE_ERR;
    }
    
    //    if(g_exitflag)
    //    {
    //        return DL_ERRCODE_SUCCESS;
    //    }
    
    //DEBUGOUT("%s[%d], LoadDownloadFileInfo end time = %d", __FUNCTION__, __LINE__, GetTime());
    ptrdownloadinfo = (CDownload*)_mempool->GetMemory();
    assert(ptrdownloadinfo);
    
    //    ptrdownloadinfo->_next = NULL;
    ptrdownloadinfo->_status = DLS_ALREADY;
    ptrdownloadinfo->_needremovefile = 0;
    ptrdownloadinfo->_invalid = 0;
    
//    ptrdownloadinfo->_refcount = 1;
    ptrdownloadinfo->_urlinfo = *http._urlinfo;
    ptrdownloadinfo->_intervalsec = NOTIFY_LOGIC_INTERVAL_MS;
    ptrdownloadinfo->_lastnotifysec = GetTime() / ptrdownloadinfo->_intervalsec;
    ptrdownloadinfo->_cookie = cookie;
    //    ptrdownloadinfo->_datablockmempool = g_datablockmempool;
    //    ptrdownloadinfo->_downloadnodemempool = g_downloadnodemempool;
    
    ptrdownloadinfo->_dlfileinfo = dlFileInfo;
    ptrdownloadinfo->_dlfileinfo._logcount = 0;
    
    //    初始化下载soket信息节点
    for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
    {
        ptrdownloadinfo->_dltask[i]._posinfo = ptrdownloadinfo->_dlfileinfo._loginfo._position[i];
        snprintf(ptrdownloadinfo->_dltask[i]._host, MAX_HOST_NAME_LEN, "%s", http._urlinfo->_host);
        ptrdownloadinfo->_dltask[i]._socket = NULL;
        ptrdownloadinfo->_dltask[i]._status = SOCKETINFO_NONCONNECT;
        //todo：modify if(ptrdownloadinfo->_dltask[i]._posinfo._offset == filesize)
        if(ptrdownloadinfo->_dltask[i]._posinfo._offset == ptrdownloadinfo->_dltask[i]._posinfo._end)
        {
            ptrdownloadinfo->_dltask[i]._status = SOCKETINFO_COMPLETE;
        }
    }
    
    if(ifileexist == 0)
    {
        *dlinfo = ptrdownloadinfo;
        return DL_ERRCODE_SUCCESS;
    }
    
    assert(ifileexist == 1);
    ptrdownloadinfo->_status = DLS_COMPLETE;
    *dlinfo = ptrdownloadinfo;
    return DL_ERRCODE_SUCCESS;
}

void CDownloadManager::_AddNodeToDownloadList(CDownload* dlinfo)
{
    int iret = 0;
    this->Dispatch(dlinfo);
}

void CDownloadManager::_UpdateDownloadStatus()
{
    if(_curdownloadtaskcount == _maxtasknumber)
    {
        return;
    }
    std::list<CDownload*>::iterator dllistitor;
    
    //在下载任务不足的时候，将就绪的下载任务改变为下载中状态
    if(_curdownloadtaskcount < _maxtasknumber)
    {
        for(dllistitor=_downloadlist.begin(); dllistitor != _downloadlist.end(); ++dllistitor)
        {
            printf("invalid=%d, status=%d\n", (*dllistitor)->_invalid, (*dllistitor)->_status);
            if((*dllistitor)->_invalid || (*dllistitor)->_status != DLS_ALREADY)
            {
                continue;
            }
            
            if((*dllistitor)->_status == DLS_ALREADY)
            {
                (*dllistitor)->_status = DLS_DOWNLOADING;
                _curdownloadtaskcount++;
                if(_curdownloadtaskcount >= _maxtasknumber)
                {
                    break;
                }
            }
        }
        return ;
    }
    
    //计算当前的下载任务数
    _curdownloadtaskcount = 0;
    for(dllistitor=_downloadlist.begin(); dllistitor!=_downloadlist.end(); ++dllistitor)
    {
        if((*dllistitor)->_status == DLS_DOWNLOADING)
        {
            if(_curdownloadtaskcount <= _maxtasknumber)
            {
                _curdownloadtaskcount++;
            }
            else
            {
                (*dllistitor)->_status = DLS_ALREADY;
            }
        }
    }
    return ;
}
