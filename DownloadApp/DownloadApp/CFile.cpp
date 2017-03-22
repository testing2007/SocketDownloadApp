//
//  CFile.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CFile.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "CUtil.h"
#include "Internal.h"
#include <assert.h>
#include <sys/errno.h>
#include "CDownload.h"

CDlFileInfo::CDlFileInfo()
{
    memset(_sfilename, 0, MAX_PATH);
    memset(_cdlfilename, 0, MAX_PATH);
    memset(_clogfilename, 0, MAX_PATH);
    _cdlfp = NULL;
    _curmaxend = 0;
    _logcount = 0;
    memset(_logmd5, 0, MD5_VALUE_LEN);
}

CDlFileInfo::~CDlFileInfo()
{
    if(_cdlfp!=NULL)
    {
        fclose(_cdlfp);
        _cdlfp = NULL;
    }
}

char* CDlFileInfo::GetSFilePath()
{
    return NULL;
}

char* CDlFileInfo::GetCDLFilePath()
{
    return NULL;
}

char* CDlFileInfo::GetLogFilePath()
{
    return NULL;
}

int CDlFileInfo::LoadDownloadFileInfo(const char* filename, const unsigned char* md5, size_t filesize)
{
    int iret = 0;
    int i;
    
    if(strlen(filename) + 12 > MAX_PATH)//?
    {
        return -1;
    }
    
    strncpy(_sfilename, filename, MAX_PATH);
    memcpy(_cdlfilename, filename, strlen(filename));
    strncat(_cdlfilename, ".cdl", MAX_PATH);
    
    memcpy(_clogfilename, filename, strlen(filename));
    strncat(_clogfilename, ".cdlog", MAX_PATH);
    
    //  已经存在下载的文件
    iret = access(filename, R_OK);
    if(iret == 0)
    {
        remove(_cdlfilename);
        remove(_clogfilename);
        _loginfo._blocksize = filesize;
        return 1;
    }
    
    iret = access(_cdlfilename, W_OK);
    if(iret != 0)
    {
        //下载临时文件错误
        DEBUGOUT("%s[%d] [%s] file isn't exist!", __FUNCTION__, __LINE__, _cdlfilename);
        goto start;
    }
    
    //  加载下载日志文件
    iret = _LoadDownloadLogInfo(_clogfilename, md5);
    if(iret == 0)
    {
        size_t curfilesize = _loginfo._filesize;
        size_t totaldlbytes = curfilesize;
        size_t maxend  = 0;
        for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
        {
            if(_loginfo._position[i]._end < _loginfo._position[i]._offset)
            {
                // ÷ÿ–¬ø™ º
                //assert(_loginfo.position[i].end >= _loginfo.position[i].offset);
                //remove(_cdlfilename);
                //remove(_clogfilename);
                goto start;
            }
            
            totaldlbytes -= _loginfo._position[i]._end - _loginfo._position[i]._offset;
            if(maxend < _loginfo._position[i]._end)
            {
                maxend = _loginfo._position[i]._end;
            }
        }
        
        if(maxend != curfilesize)
        {
            totaldlbytes -= curfilesize - maxend;
        }
        assert(totaldlbytes == _loginfo._dltotalbytes);
        
        _curmaxend = 0;
        for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
        {
            if(_curmaxend < _loginfo._position[i]._end)
            {
                _curmaxend = _loginfo._position[i]._end;
            }
        }
        return 0;
    }
    
start:
    remove(_cdlfilename);
    remove(_clogfilename);
    
    if(md5)
    {
        memcpy(_loginfo._md5, md5, MD5_VALUE_LEN);
    }
    
    _loginfo._blocksize = DATA_BLOCK_SIZE;
    if(MAX_DOWNLOAD_TASK == 1)
    {
        _loginfo._blocksize = filesize;
    }
    
    //  初始化下载块
    _loginfo._filesize = filesize;
    for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
    {
        _loginfo._position[i]._offset = i * _loginfo._blocksize;
        _loginfo._position[i]._end = _loginfo._position[i]._offset + _loginfo._blocksize;
        if(_loginfo._position[i]._end > _loginfo._filesize)
        {
            if(_loginfo._position[i]._offset > _loginfo._filesize)
            {
                _loginfo._position[i]._offset = _loginfo._filesize;
                _loginfo._position[i]._end = _loginfo._filesize;
            }
            else
            {
                _loginfo._position[i]._end = _loginfo._filesize;
            }
            i++;
            break;
        }
    }
    
    _curmaxend = _loginfo._position[i - 1]._end;
    _loginfo._dltotalbytes = 0;
    //DEBUGOUT("%s[%d], time = %d", __FUNCTION__, __LINE__, GetTime());
    return 0;
}

int CDlFileInfo::WriteDataToFile(CDownload& download, DataBlockInfo* datablock)
{
//    DlFileInfo* ptrdlfileinfo;
    int iret;
//    DownloadInfo* ptrdownloadinfo;
    size_t wlen;
    unsigned long logcount;
    
    if(download._invalid)
    {
        if(_cdlfp)
        {
            fclose(_cdlfp);
            _cdlfp = NULL;
        }
        return 0;
    }
    
    //如果发生过写错误，将直接返回
    if(download._status == DLS_WRITEFILEERR || download._status == DLS_CREATEFILEERR)
    {
        if(_cdlfp)
        {
            fclose(_cdlfp);
            _cdlfp = NULL;
        }
        return 0;
    }
    
    //    文件是否创建过
    if(_cdlfp == NULL)
    {
        FILE* fp;
        //  文件是否可写
        iret = access(_cdlfilename, W_OK);
        if(iret == 0)
        {
            //    打开文件
            fp = fopen(_cdlfilename, FILE_READWRITE_MODE);
            if(fp == NULL)
            {
                DEBUGOUT("%s[%d] [%s] can't open with write!!", __FUNCTION__, __LINE__, _cdlfilename);
                download._status = DLS_WRITEFILEERR;
                if(download._notify)
                {
                    download._notify(DLS_WRITEFILEERR, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, 0);
                }
                return -2;
            }
            _cdlfp = fp;
        }
        else
        {
            //是否存在文件
            iret = access(_cdlfilename, 0);
            if(iret == 0)
            {
                DEBUGOUT("%s[%d] %s can't write!!", __FUNCTION__, __LINE__, _cdlfilename);
                download._status = DLS_WRITEFILEERR;
                if(download._notify)
                {
                    download._notify(DLS_WRITEFILEERR, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, 0);
                }
                return -3;
            }
            //创建文件
            fp = _CreateFile(_cdlfilename, _loginfo._filesize);
            if(fp == NULL)
            {
                download._status = DLS_CREATEFILEERR;
                if(download._notify)
                {
                    download._notify(DLS_CREATEFILEERR, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, 0);
                }
                DEBUGOUT("%s[%d] Create download file failure!\n", __FUNCTION__, __LINE__);
                return -3;
            }
            
            _cdlfp = fp;
            fp = _CreateFile(_clogfilename,sizeof(DownloadLogInfo) + MD5_VALUE_LEN);
            if(fp == NULL)
            {
                if(_cdlfp)
                {
                    fclose(_cdlfp);
                    remove(_cdlfilename);
                }
                
                download._status = DLS_CREATEFILEERR;
                if(download._notify)
                {
                    download._notify(DLS_CREATEFILEERR, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, 0);
                }
                DEBUGOUT("%s[%d] Create log failure!\n", __FUNCTION__, __LINE__);
                return -3;
            }
            fclose(fp);
        }
    }
    
    //  文件定位
    fseek(_cdlfp, datablock->_posinfo._offset, SEEK_SET);
    if(_loginfo._dltotalbytes + datablock->_length > _loginfo._filesize)
    {
        DEBUGOUT("%s[%d]  filesize[%ld] dltotalbytes[%ld] length[%ld]\n", __FUNCTION__, __LINE__, _loginfo._filesize, _loginfo._dltotalbytes + datablock->_length, datablock->_length);
    }
    //写文件
    //assert(_loginfo.dltotalbytes + datablock->length <= _loginfo.filesize);
    {
        char* ptrdata = datablock->_data;
        int len = datablock->_length;
        while(len > 0)
        {
            wlen = fwrite(ptrdata, 1, len, _cdlfp);
            if(wlen <= 0)
            {
                
                DEBUGOUT("%s[%d]  WriteDataToFile error wlen=%d errno = %d _cdlfp = 0x%x\n", __FUNCTION__, __LINE__, wlen, errno, _cdlfp);
                fclose(_cdlfp);
                _cdlfp = NULL;
                download._status = DLS_WRITEFILEERR;
//                if(download._notify && download._invalid == 0)
//                {
//                    download._notify(DLS_WRITEFILEERR, download._usrdata, download._dlfileinfo.loginfo.dltotalbytes, 0);
//                }
                return -2;
            }
            len -= wlen;
            ptrdata += wlen;
        }
    }
    
    _loginfo._dltotalbytes += datablock->_length;
    datablock->_posinfo._offset += datablock->_length;
    //  下载完成
    if(_loginfo._dltotalbytes == _loginfo._filesize)
    {
        int iret;
        iret = fclose(_cdlfp);
        if(iret != 0)
        {
            DEBUGOUT("%s[%d]  fclose error errno[%d]\n", __FUNCTION__, __LINE__, errno);
            //iret = fclose(_cdlfp);
        }
        
        _cdlfp = NULL;
        //  删除下载日志文件
        iret = remove(_clogfilename);
        if(iret != 0)
        {
            DEBUGOUT("%s[%d]  remove error errno[%d]\n", __FUNCTION__, __LINE__, errno);
            iret = remove(_clogfilename);
        }
        //重命名文件
        iret = rename(_cdlfilename, _sfilename);
        if(iret != 0)
        {
            DEBUGOUT("%s[%d]  rename error errno[%d]\n", __FUNCTION__, __LINE__, errno);
#ifdef WIN32
            if(CopyFile(_cdlfilename, _sfilename, 1) == 0)
            {
                DEBUGOUT("%s[%d]  CopyFile failure\n", __FUNCTION__, __LINE__);
            }
#endif
        }
        //如果有md5校验，将进入后处理流程
        if(_loginfo._md5[0])
        {
            download._status = DLS_POSTPROCCES;
            if(download._notify && download._invalid == 0)
            {
                download._notify(DLS_POSTPROCCES, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, download._dlfileinfo._loginfo._filesize);
            }
            download._postprocess.PutDownloadInfoToPostprocess(download);
        }
        else
        {
            //    通知上层下载完成
            download._status = DLS_COMPLETE;
            if(download._notify && download._invalid == 0)
            {
                download._notify(DLS_COMPLETE, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, download._dlfileinfo._loginfo._filesize);
            }
        }
        return 1;
    }
    //健壮性处理
    if(datablock->_posinfo._offset > datablock->_posinfo._end)
    {
        DEBUGOUT("%s[%d] offset[%d] end[%d]\n", __FUNCTION__, __LINE__, datablock->_posinfo._offset, datablock->_posinfo._end);
        //assert(0);
    }
    
    if(datablock->_posinfo._offset == datablock->_posinfo._end)//øÈœ¬‘ÿÕÍ≥…£¨—∞’“–¬µƒœ¬‘ÿøÈ
    {
        _SetNextDownloadPos(&(_loginfo._position[datablock->_index]));
        // download._status = DLS_BLOCKCOMPLETE;
        //todo 这个怎么处理  datablock->downloadinfo->dltask[datablock->index].posinfo = _loginfo.position[datablock->index];
    }
    else
    {
        _loginfo._position[datablock->_index] = datablock->_posinfo;
    }

//  定时通知上层下载的情况
    {
        unsigned long lastnotifysec = GetTime() / download._intervalsec;
        if(download._lastnotifysec != lastnotifysec)
        {
            download._lastnotifysec = lastnotifysec;
            if(download._status == DLS_STOPPING)
            {
                if(download._notify && download._invalid == 0)
                {
                    download._notify(DLS_DOWNLOADING, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, download._dlfileinfo._loginfo._filesize);
                }
            }
            else
            {
                if(download._status != DLS_DOWNLOADING)
                {
                    DEBUGOUT("%s[%d] dlstatus=%d dltotalbytes=%d\n", __FUNCTION__, __LINE__, download._status, download._dlfileinfo._loginfo._dltotalbytes);
                }
                
                if(download._notify && download._invalid == 0)
                {
                    download._notify(download._status, download._usrdata, download._dlfileinfo._loginfo._dltotalbytes, download._dlfileinfo._loginfo._filesize);
                }
            }
        }
    }
    
    //  当下载的累积字节数大于1M时，更新日志文件
    logcount = _loginfo._dltotalbytes / REFRESH_LOG_THRESHOLD_SIZE;
    if(_logcount < logcount)
    {
        _logcount = logcount;
        _RefreshLogfile();
    }
    return 0;
    
}

int CDlFileInfo::CloseDownloadFile(int needremovefile)
{
    if(_cdlfp)
    {
        if(!needremovefile)
        {
            fflush(_cdlfp);
        }
        
        fclose(_cdlfp);
        _cdlfp = NULL;
        
        if(!needremovefile)
        {
            _RefreshLogfile();
        }
    }
    
    if(needremovefile)
    {
        //DEBUGOUT("%s[%d] filename = [%s] clogfile[%s]\n", __FILE__, __LINE__, _cdlfilename, _clogfilename);
        remove(_cdlfilename);
        remove(_clogfilename);
        return 0;
    }
    return 0;
}

FILE* CDlFileInfo::_CreateFile(const char * sfilename, size_t filelen)
{
    FILE* fp = fopen(sfilename, FILE_WRITE_MODE);
    if(fp == NULL)
    {
        DEBUGOUT("%s[%d] fopen fail! [%s] errno[%d]\n", __FUNCTION__, __LINE__, sfilename, errno);
        return NULL;
    }
    
    int iret = fseek(fp, filelen - 1, SEEK_SET);
    if(iret == 0 && (EOF != fputc(0xAA, fp)))
    {
        return fp;
    }
    
    DEBUGOUT("%s[%d] errno[%d] [%s] fseek fail!  \n", __FUNCTION__, __LINE__, errno, sfilename);
    fclose(fp);
    remove(sfilename);
    return NULL;
}

int CDlFileInfo::_LoadDownloadLogInfo(const char* dllogfilename, const unsigned char* filemd5/*DownloadLogInfo代替 , unsigned char *buffer*/)
{
    FILE *fp;
    size_t loglen;
    char logbuf[1024] = {0};
    md5_state_t state;
    unsigned char curmd5[MD5_VALUE_LEN];
    md5_byte_t digest[16];
    int ret;
    
    char* ptr;
    
    fp = fopen(dllogfilename, FILE_READ_MODE);
    if(fp == NULL)
    {
        return -1;
    }
    
    //进行md5校验
    fseek(fp, 0, SEEK_END);
    loglen = ftell(fp);
    if(loglen != (MD5_VALUE_LEN + sizeof(DownloadLogInfo)))
    {
        DEBUGOUT("%s[%d] file size not right\n", __FUNCTION__, __LINE__);
        fclose(fp);
        return -1;
    }
    
    fseek(fp, 0, SEEK_SET);
    ret = fread(logbuf, 1, loglen, fp);
    fclose(fp);
    
    if(ret != loglen)
    {
        DEBUGOUT("%s[%d] logfile read error\n", __FUNCTION__, __LINE__);
        return -2;
    }
    
    ptr = logbuf + MD5_VALUE_LEN;
    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)(logbuf + MD5_VALUE_LEN), loglen - MD5_VALUE_LEN);
    md5_finish(&state, digest);
    
    GetMD5String(digest, curmd5);
    ret = memcmp(logbuf, curmd5, MD5_VALUE_LEN);
    if(ret == 0)
    {
        if(filemd5 && memcmp(logbuf + MD5_VALUE_LEN, filemd5, MD5_VALUE_LEN) != 0)
        {
            DEBUGOUT("%s[%d] file md5 not right\n", __FUNCTION__, __LINE__);
            return -3;
        }
        memcpy(&_loginfo, (const md5_byte_t *)(logbuf + MD5_VALUE_LEN), loglen - MD5_VALUE_LEN);
        return 0;
    }
    DEBUGOUT("%s[%d] logfile md5 not right\n", __FUNCTION__, __LINE__);
    return -3;
}

//更新下载日志文件
int CDlFileInfo::_RefreshLogfile()
{
    FILE* fp;
    int i;
    size_t maxend  = 0;
    size_t filesize = _loginfo._filesize;
    size_t totaldlbytes = filesize;
    
    //DEBUGOUT("------------------------filesize = %u ------------------\n", filesize);
    for(i = 0; i < MAX_DOWNLOAD_TASK; i++)
    {
        size_t len;
        if(_loginfo._position[i]._end == _loginfo._position[i]._offset && _loginfo._filesize != _loginfo._position[i]._offset)
        {
            if(_loginfo._position[i]._offset == 0)
            {
                _loginfo._position[i]._offset = _loginfo._filesize;
                _loginfo._position[i]._end = _loginfo._filesize;
            }
            else
            {
                DEBUGOUT("%s[%d] index[%d] offset[%d] filesize[%d]\n", __FUNCTION__, __LINE__, i, _loginfo._position[i]._offset, _loginfo._filesize);
                //assert(0);
            }
        }
        
        len = _loginfo._position[i]._end - _loginfo._position[i]._offset;
        totaldlbytes -= len;
        if(maxend < _loginfo._position[i]._end)
        {
            maxend = _loginfo._position[i]._end;
        }
        //DEBUGOUT("-----------index[%d]--offset[%u] end[%u] --len = %u maxend = %u\n", i, _loginfo._position[i].offset, _loginfo._position[i].end, len, maxend);
    }
    
    if(maxend != filesize)
    {
        totaldlbytes -= filesize - maxend;
    }
    
    //DEBUGOUT("%s[%d] clogname[%s]--maxend[%u]--totaldlbytes = %u [dltotalbytes = %d] value=%d----\n", __FUNCTION__, __LINE__, _clogfilename, maxend, totaldlbytes, _loginfo.dltotalbytes, totaldlbytes - _loginfo.dltotalbytes);
    if(totaldlbytes != _loginfo._dltotalbytes)
    {
        DEBUGOUT("%s[%d] clogname[%s]---totaldlbytes = %u [dltotalbytes = %d] value=%d----\n", __FUNCTION__, __LINE__, _clogfilename, totaldlbytes, _loginfo._dltotalbytes, totaldlbytes - _loginfo._dltotalbytes);
        return -1;
    }
    
    fp = fopen(_clogfilename, FILE_READWRITE_MODE);
    if(fp == NULL)
    {
        fp = fopen(_clogfilename, FILE_WRITE_MODE);
        if(fp == NULL)
        {
            DEBUGOUT("%s[%d] fopen fail! errno[%d]\n", __FUNCTION__, __LINE__, errno);
            return -1;
        }
    }
    
    GetBuffMD5Value((const char *)&(_loginfo), sizeof(DownloadLogInfo), _logmd5);
    
    fwrite(_logmd5, 1, MD5_VALUE_LEN, fp);
    fwrite(&(_loginfo), 1, sizeof(DownloadLogInfo), fp);
    fclose(fp);
    return 0;
}

//计算下一块的下载范围
int CDlFileInfo::_SetNextDownloadPos(PosInfo *posinfo)
{
    if(_curmaxend >= _loginfo._filesize)
    {
        posinfo->_offset = _loginfo._filesize;
        posinfo->_end = posinfo->_offset;
        return 1;
    }
    
    posinfo->_offset = _curmaxend;
    posinfo->_end = posinfo->_offset + _loginfo._blocksize;
    if(posinfo->_end > _loginfo._filesize)
    {
        posinfo->_end = _loginfo._filesize;
    }
    _curmaxend = posinfo->_end;
    return 0;
}

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

//private:
//char _sfilename[MAX_PATH]; //保存文件名
//char _cdlfilename[MAX_PATH]; //中间文件名
//char _clogfilename[MAX_PATH]; //日志文件名
//
//FILE* _cdlfp; //中间文件句柄
//
//size_t _curmaxend;//当前下载块里面最大结束位置
//
//unsigned long _logcount;
//
////-----------------------MD5_VALUE_LEN------------------
//unsigned char _logmd5[MD5_VALUE_LEN];
//DownloadLogInfo _loginfo;
