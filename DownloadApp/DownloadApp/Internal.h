//
//  Internal.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include <sys/syslimits.h>
//class CDlFileInfo;
class CMySocket;
class CDownload;

#ifndef DownloadApp_Internal_h
#define DownloadApp_Internal_h

#define MAX_PATH (PATH_MAX)

#define SOCKET int

#define INVALID_SOCKET -1


//////////////////////////////////////////
#define HTTP_RECEIVE_SLEEPTIME_MS (2)

#define MD5_VALUE_LEN (32)

#define MAX_HOST_NAME_LEN (256)

#define MAX_DOWNLOAD_TASK (5)

#define BUFFER_DATA_BLOCK_SIZE (128*1024) //BUFFER_BLOCKSIZE
#define DATA_BLOCK_SIZE (1024*1024)
#define REFRESH_LOG_THRESHOLD_SIZE (1024*1024)

#define JOIN_SOCKET_SET_TIMEOUT_SEC (5000) //MAX_TIMEOUT_SEC

#define NOTIFY_LOGIC_INTERVAL_MS (500) //DEFUALT_INTERVAL_MS
#define CONNECT_TIMEOUT_MS (3000) //CONNECT_TIMEINTERVAL, 预处理

#define FILE_READ_MODE ("r")
#define FILE_READWRITE_MODE ("rb+")
#define FILE_WRITE_MODE ("w")


///////////////////////////////////////////

//分析url后的结构体
struct url_t
{
    char* _strurl;
    char* _scheme;
    char* _user;
    char* _pwd;
    char _host[MAX_HOST_NAME_LEN];
    char* _doc;
    char* _filename;
    int _port;
};

typedef struct PosInfo_t
{
    unsigned long _offset;
    unsigned long _end;
}PosInfo;

typedef struct DownloadLogInfo_t{
    unsigned char _md5[MD5_VALUE_LEN];//文件md5
    size_t _filesize; //文件大小
    size_t _blocksize; //块大小
    size_t _dltotalbytes; //总下载大小
    PosInfo _position[MAX_DOWNLOAD_TASK]; //每个下载块的偏移量及结束位置
}DownloadLogInfo;

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

#define SOCKETINFO_NONCONNECT -1
#define SOCKETINFO_CONNECTING 0
#define SOCKETINFO_CONNECTED 1
#define SOCKETINFO_COMPLETE 2
#define SOCKETINFO_NOFILE 404
#define SOCKETINFO_SINFINVALID 403

typedef struct SocketDownloadInfo_t
{
    int _status;//SOCKETINFO_***
    unsigned long _timeoutsec;
    //SOCKET sock;
    CMySocket* _socket;
    PosInfo _posinfo;
    char _host[MAX_HOST_NAME_LEN];
}SocketDownloadInfo;

enum DOWNLOAD_ERRCODE
{
    DL_ERRCODE_SUCCESS = 0,
    DL_ERRCODE_NET_ERR = 1,
    DL_ERRCODE_URL_ERR = 2,
    DL_ERRCODE_GETCONTENTLEN_ERR = 3,
    DL_ERRCODE_FILE_ERR = 4,
};

enum DLSTATUS{
    DLS_UNKNOW = 0,
    DLS_PREPROCCES,
    DLS_ALREADY,
    DLS_DOWNLOADING,
    DLS_POSTPROCCES,
    DLS_ICONCOMPLETE,
//    DLS_BLOCKCOMPLETE,// = 6, //块下载完成，回调以后，需要指定下一块的开始及结束位置。
    DLS_COMPLETE,
    DLS_STOP,
    DLS_REMOVE,
    DLS_FAIL,// = 10,
    DLS_FILEERR,
    DLS_CHECKSUMERR,
    DLS_OTHERERR,
    DLS_NETERR,
    DLS_APPINFOERR,
    DLS_CREATEFILEERR,//16
    DLS_NOFILEERR,
    DLS_WRITEFILEERR,
    DLS_IPAFILEERR,
    DLS_APPSINFINVALID,
    DLS_STOPPING,// 21
};
typedef int (*NotifyDownloadStatus)(enum DLSTATUS status, void* usrparam, size_t curbytes, size_t filesize);
//typedef struct DownloadInfo_t
//{
////    struct DownloadInfo_t *next;
//    
//    //------------------------------------
//    //    MemPoolHandler datablockmempool;
//    //    MemPoolHandler downloadnodemempool;
//    //---------------------------------
//    int refcount;//
//    DLSTATUS status;//DLS_*
//    int needremovefile;
//    int invalid;
//
//    //----------------------------------
//    NotifyDownloadStatus notify;//用户下载时传入过来的回调函数
//    void* usrdata; //用户下载时传入过来的参数
//    unsigned long intervalsec;
//    unsigned long lastnotifysec;
//    
//    //---------------------------------
//    struct url_t urlinfo;
//    const char *cookie;
//    
//    SocketDownloadInfo dltask[MAX_DOWNLOAD_TASK];
//    
//    CDlFileInfo* dlfileinfo;
//}DownloadInfo;

typedef struct DataBlockInfo_t{
//    struct DataBlockInfo_t *next;
//    CDownload* _downloadinfo;
    int _index;
    PosInfo _posinfo;
    size_t _length;
    char _data[0];
}DataBlockInfo;


#endif
