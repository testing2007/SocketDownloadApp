//
//  CDownload.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CDownload.h"
#include "CDownloadManager.h"
#include "CUtil.h"
#include "CHttp.h"
#include "CFile.h"
#include "Internal.h"
#include "CMySocket.h"
#include <sys/_types/_fd_set.h>
#include <sys/_types/_fd_def.h>
#include <sys/_types/_fd_zero.h>
#include <sys/_select.h>
#include <sys/socket.h>
#include <sys/errno.h>


void CDownload::_CleanWorker()
{
    //stop downloading
    if(_invalid)
    {
        return ;
    }
//    DEBUGOUT("%s[%d] ***---refcount[%d] -status[%d]**\n", __FUNCTION__, __LINE__, /*_refcount,*/ _status);
    DEBUGOUT("%s[%d] ***---status[%d]** \n", __FUNCTION__, __LINE__,  _status);
    
    _status = DLS_STOP;
    if(_notify)
    {
        _notify(DLS_STOP, _usrdata, 0, 0);
    }

    int i = 0;
    for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
    {
        if(_dltask[i]._socket != NULL)
        {
            _dltask[i]._socket->Close();
        }
    }
    _dlfileinfo.CloseDownloadFile(0);
    _invalid = 1;
}

unsigned long CDownload::GetCurSpeed()
{
    unsigned long limitspeed = CDownloadManager::Instance()->GetLimitSpeed();
    if(_curspeed>limitspeed && limitspeed!=0)
    {
        return limitspeed;
    }
    return _curspeed;
}

void CDownload::SetUserAgent(const char *agentinfo)
{
    
}

//  获取下载的文件尺寸大小
//  参数：
//  handle：下载句柄
//  返回值：文件大小，如果失败返回0
size_t CDownload::GetDownloadFileSize()
{
    return _invalid ? 0 : _dlfileinfo._loginfo._filesize;
}

void CDownload::_DoDownload()
{
    fd_set fdset;
    int maxsock;
    int ret;
    struct timeval tv;
    int timeoutcount;
    
    //限速相关变量
    //---------------------------
    unsigned long limitspeed = CDownloadManager::Instance()->GetLimitSpeed();
    unsigned long needreadbytes = limitspeed;
    unsigned long speedtime = GetTime(); //记录时间戳
    unsigned int curreadcount = 0; //当前读取的字节数
    unsigned long readbytes = 0; //累计读取的自己数
    long timeinterval;//时间间隔
    _curspeed = 0; //当前下载速度
    //---------------------------
    
    DEBUGOUT("%s[%d]  Start DownloadThread\n", __FUNCTION__, __LINE__);
    while(!_invalid) //todo:结束条件还没有想清楚
    {
        //----------------------------------------
        timeinterval = GetTime() - speedtime;
        readbytes += curreadcount;
        
        //    计算当前的下载速度
        if(timeinterval >= 1000)
        {
            //DEBUGOUT("---[%d]---current download speed = %dkB/s---\n", timeinterval, (readbytes >> 10));
            _curspeed = readbytes;
            readbytes = 0;
            speedtime = GetTime();
        }
        
        curreadcount = 0;
        //----------------------------------------
        //DEBUGOUT("%s[%d]  tick = %d\n", __FUNCTION__, __LINE__, GetTime());
        FD_ZERO(&fdset);
        
        //  进行下载list的socket失效处理，超时处理和连接处理
        maxsock = _PutDownloadSocket(&fdset);
        
        if(maxsock == 0)
        {
            Sleep(500);
            continue;
        }
        
        tv.tv_sec = 0;
        tv.tv_usec = 250 * 1000;
        //DEBUGOUT("%s[%d]  tick = %d\n", __FUNCTION__, __LINE__, GetTime());
        ret = select(maxsock + 1, &fdset, NULL, NULL, &tv);
        if(ret < 0)
        {
            DEBUGOUT("%s[%d] select < 0!!!\n", __FUNCTION__, __LINE__);
            break;
        }
        else if(ret == 0)
        {
            DEBUGOUT("%s[%d] select timeout!!!\n", __FUNCTION__, __LINE__);
            Sleep(1);
            continue;
        }
        
        //  计算出每个socket可以读取的数据量
        needreadbytes = 0;
        if(limitspeed > 0)
        {
            needreadbytes = _GetLimitReadBytes(0);
            if(needreadbytes <= 0)
            {
                assert(needreadbytes != 0);
                continue;
            }
            
            needreadbytes /= ret;
            if(needreadbytes == 0)
            {
                needreadbytes = 4;
            }
        }
        
        //  获取指定字节数下载数据
        curreadcount = _GetDownloadData(&fdset, needreadbytes);
        Sleep(1);
        _GetLimitReadBytes(curreadcount);
    }
    
    StopDownloadFile();
}

void* CDownload::_DownloadThread(void* param)
{
    CDownload* dl = (CDownload*)param;
    if(dl!=NULL)
    {
        dl->_DoDownload();
    }
    return NULL;
}

void CDownload::StartDownloadFile()
{
    _datablockmempool = CMemPool::CreateMemPool(20, BUFFER_DATA_BLOCK_SIZE);
    //todo:后续考虑使用线程池来替代
    pthread_create(&_thread, NULL, CDownload::_DownloadThread, this);
}

//  停止下载
//  参数：
//  handle：下载句柄
//  返回值：成功返回0，异常返回－1
int CDownload::StopDownloadFile()
{
    _CleanWorker();
    return 0;
}

//  恢复下载
//  参数：
//  handle：下载句柄
//  返回值：成功返回0，异常返回－1
int CDownload::ResumeDownloadFile()
{
    return 0;
}

//  获取当前的下载状态
//  参数：
//  handle：下载句柄
//  返回值：下载的状态
enum DLSTATUS CDownload::GetCurDownloadStatus()
{
    return (DLSTATUS)0;
}

//  关闭下载
//  参数：
//  handle：下载句柄
//  needremovefile：是否需要删除任务
//  返回值：成功返回0，异常返回－1
int CDownload::CloseDownload(int needremovefile)
{
    return 0;
}

//  获取下载保存的路径文件名
//  参数：
//  handle：下载句柄
//  filename：文件路径名指针
//  返回值：成功返回0，错误返回－1
int CDownload::GetDownloadFilePathname(char** filename)
{
    return 0;
}

//  发起一个http请求
//  参数：
//  URL：请求的URL
//  postparaminfo：post参数信息，如果该参数为NULL，将使用get方式进行请求
//  agentflag：请求头中是否包括User-Agent字段
//  errcode：错误码，请求错误，将通过此参数返回错误码
//  返回值：成功返回http的响应数据，不包括头信息，该内存资源需要用户进行释放，错误返回NULL
char* CDownload::GetUrlRespondContent(const char *URL, const char* postparaminfo, int agentflag, int* errcode)
{
    return NULL;
}

//  获取文件的md5值
//  参数：
//  sfilename：源文件名
//  md5：文件的md5值，空间由用户自己分配。
//  返回值：成功返回0，错误返回-1
int CDownload::GetFileMD5Value(const char* sfilename, unsigned char *md5)
{
    return 0;
}

//  计算buffer的md5值
//  参数：
//  buffer：数据指针
//  count：数据长度
//  md5：计算出来的md5值，空间由用户自己分配。
//  返回值：成功返回0，错误返回-1
int CDownload::GetBuffMD5Value(const char* buffer, int count, unsigned char *md5)
{
    return 0;
}

//  base64加密
//  参数：
//  to：加密后的数据(内存空间由用户自己分配)
//  from：加密前的数据
//  dlen：数据长度
//  返回值：成功返回加密后的数据长度，失败返回-1
int CDownload::EncodeBase64Block(unsigned char *to, const unsigned char *from, int dlen)
{
    return 0;
}

//  base64解密
//  参数：
//  to：解密后的数据(内存空间由用户自己分配)
//  from：加密前的数据
//  n：数据长度
//  返回值：成功返回解密后的数据长度，失败返回-1
int CDownload::DecodeBase64Block(unsigned char *to, const unsigned char *from, int n)
{
    return 0;
}

SOCKET CDownload::_PutDownloadSocket(fd_set *fdset)
{
    unsigned long curdownloadtaskcount = 0;
    
    SOCKET cursock = -1;
    SOCKET maxsock = 0;
    int i;
    int socketstatus;
    
    //todo:省略了一些错误判断
    //        curdownloadtaskcount++;
    for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
    {
        SocketDownloadInfo* sockinfo = _dltask + i;
        socketstatus = sockinfo->_status;
        cursock = sockinfo->_socket!=NULL ? sockinfo->_socket->_socketid : INVALID_SOCKET;
        if(socketstatus == SOCKETINFO_COMPLETE || socketstatus ==  SOCKETINFO_CONNECTING)
        {
            continue;
        }
        //socket出错状态处理
        if(socketstatus == SOCKETINFO_NOFILE)
        {
            DEBUGOUT("%s[%d]\n", __FUNCTION__, __LINE__);
            if(_notify)
            {
                _notify(DLS_NOFILEERR, _usrdata, 0, 0);
                _status = DLS_NOFILEERR;
            }
            break;
        }
        //socket出错状态处理
        if(socketstatus == SOCKETINFO_SINFINVALID)
        {
            DEBUGOUT("%s[%d] status[SOCKETINFO_SINFINVALID]\n", __FUNCTION__, __LINE__);
            if(_notify)
            {
                _notify(DLS_APPSINFINVALID, _usrdata, 0, 0);
            }
            _status = DLS_APPSINFINVALID;
            break;
        }
        
        //文件下载结束处理
        if(sockinfo->_posinfo._offset == _dlfileinfo._loginfo._filesize)
        {
            if(sockinfo->_socket!=NULL && sockinfo->_socket->_socketid != INVALID_SOCKET)
            {
                sockinfo->_socket->Close();
            }
            sockinfo->_status = SOCKETINFO_COMPLETE;
            DEBUGOUT("%s[%d]  index[%d] completed\n", __FUNCTION__, __LINE__, i);
            continue;
        }
        
        //块下载结束处理
        if(sockinfo->_posinfo._offset >= sockinfo->_posinfo._end)
        {
            if(sockinfo->_socket!=NULL && sockinfo->_socket->_socketid != -1)
            {
                sockinfo->_socket->Close();
                sockinfo->_status = SOCKETINFO_NONCONNECT;
            }
            continue;
        }
        
        //    如果当前socket状态为未连接
        if(socketstatus == SOCKETINFO_NONCONNECT)
        {
            _preprocess.PutDownloadInfoToPreprocess(this, i, sockinfo);
            sockinfo->_timeoutsec = GetTime();
            continue;
        }
        
        //    已连接状态
        if(socketstatus == SOCKETINFO_CONNECTED)
        {
            assert(cursock != -1);
            //      是否无数据超时
            if(GetTime() - sockinfo->_timeoutsec < JOIN_SOCKET_SET_TIMEOUT_SEC)
            {
                //          未超时
                //DEBUGOUT("%s[%d] sockinfo[%d] = 0x%x  add select\n", __FUNCTION__, __LINE__, i, sockinfo);
                FD_SET(cursock, fdset);
                if(maxsock < cursock)
                {
                    maxsock = cursock;
                }
                continue;
            }
            //超时重连
            if(sockinfo->_socket != NULL && sockinfo->_socket->_socketid != INVALID_SOCKET)
            {
                sockinfo->_socket->Close();
            }
            _preprocess.PutDownloadInfoToPreprocess(this, i, sockinfo);
            sockinfo->_timeoutsec = GetTime();
            continue;
        }
    }
    
    //    UpdateDownloadStatus(curdownloadtaskcount); //todo: 放在 downloadmanager 中
    return maxsock;
}

//获取每个时段因为限速可以读取的数据量
long CDownload::_GetLimitReadBytes(int curreadcount)
{
    unsigned long timeinterval;
    unsigned long timemode;
    
    static unsigned long limitspeedtime = 0;
    static int limitreadbytes = 0;
    static int pieceflag = 0;
    
    unsigned long limitspeed = CDownloadManager::Instance()->GetLimitSpeed();
    
    timeinterval = GetTime() - limitspeedtime;
    timemode = timeinterval % 1000;
    
    if(timeinterval >= 1000)
    {
        pieceflag = 0;
        limitreadbytes = (limitspeed >> 1);
        limitspeedtime = GetTime();
        return limitreadbytes;
    }
    
    limitreadbytes -= curreadcount;
    if(timemode < 500)
    {
        assert(pieceflag == 0);
        if(limitreadbytes <= 0)
        {
            if((500 - timemode - 10) > 0)
            {
                Sleep(500 - timemode);
            }
            limitreadbytes = 0;
            return -1;
        }
        return limitreadbytes;
    }
    
    if(pieceflag == 0)
    {
        limitreadbytes += (limitspeed >> 1);
        pieceflag = 1;
        return limitreadbytes;
    }
    
    if(limitreadbytes > 0)
    {
        return limitreadbytes;
    }
    
    if((1000 - timemode - 5) > 0)
    {
        Sleep(1000 - timemode);
    }
    limitspeedtime = GetTime();
    limitreadbytes = (limitspeed >> 1);
    pieceflag = 0;
    return -1;
}

//从socket 集中读取下载的数据。needreadbytes是最多可以读取的数据量
int CDownload::_GetDownloadData(fd_set *fdset, unsigned int needreadbytes)
{
    unsigned int totalbytes = 0;
    if(_status == DLS_DOWNLOADING)
    {
        int i;
        for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
        {
            SOCKET sock = _dltask[i]._socket!=NULL ? _dltask[i]._socket->_socketid : INVALID_SOCKET;
            if(sock != INVALID_SOCKET && FD_ISSET(sock, fdset))
            {
                int readcount;
                int curerrno = 0;
                
                //          分配一个数据处理节点
                DataBlockInfo *datablockinfo = (DataBlockInfo*)_datablockmempool->GetMemory();
                assert(datablockinfo);
                _dltask[i]._timeoutsec = GetTime();
                
                //          读取指定的数量的数据
                if(needreadbytes == 0 || needreadbytes > (BUFFER_DATA_BLOCK_SIZE - sizeof(DataBlockInfo)))
                {
                    readcount = recv(sock, datablockinfo->_data, BUFFER_DATA_BLOCK_SIZE - sizeof(DataBlockInfo), 0);
                }
                else
                {
                    readcount = recv(sock, datablockinfo->_data, needreadbytes, 0);
                }
                
                if(readcount == 0)
                {
                    _dltask[i]._socket->Close();
                    _dltask[i]._status = SOCKETINFO_NONCONNECT;
                    _datablockmempool->PutMemoryPool(datablockinfo);
                    continue;
                }
                
                curerrno = errno;
                // 读取出现可修复错误
                if(curerrno == EINTR || curerrno == EAGAIN)
                {
                    DEBUGOUT("%s[%d] errno = %d!!!\n", __FUNCTION__, __LINE__, errno);
                    _datablockmempool->PutMemoryPool(datablockinfo);
                    continue;
                }
                
                if(readcount < 0)
                {
                    if( _dltask[i]._socket!=NULL)
                    {
                        _dltask[i]._socket->Close();
                    }
                    _dltask[i]._status = SOCKETINFO_NONCONNECT;
                    _datablockmempool->PutMemoryPool(datablockinfo);
                    continue;
                }
                
                totalbytes += readcount;
                
                //设置数据处理节点的相关信息
                //put data to dataprocess thread
//                datablockinfo->_downloadinfo = this;
                datablockinfo->_length = readcount;
                datablockinfo->_posinfo = _dltask[i]._posinfo;
                datablockinfo->_index = i;
                
                _dltask[i]._posinfo._offset += readcount;
//                _refcount++;
                _dataprocess.PutDataBlock(*this, datablockinfo);
                
                if(_dltask[i]._posinfo._offset == _dltask[i]._posinfo._end)
                {
                    //complete receive data block
                    if( _dltask[i]._socket!=NULL)
                    {
                        _dltask[i]._socket->Close();
                    }
                    _dltask[i]._status = SOCKETINFO_NONCONNECT;
                    _dltask[i]._timeoutsec = 0;
                }

                _datablockmempool->PutMemoryPool(datablockinfo);
            }
        }
    }
    return totalbytes;
}