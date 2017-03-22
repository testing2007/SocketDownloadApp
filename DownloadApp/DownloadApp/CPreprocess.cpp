//
//  CPreprocess.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CPreprocess.h"
#include "CDownload.h"
#include "CMySocket.h"
#include "CHttp.h"
#include "CUtil.h"

CPreprocess::CPreprocess()
{
    
}

CPreprocess::~CPreprocess()
{
    
}


void CPreprocess::PutDownloadInfoToPreprocess(CDownload* downloadinfo, int index, SocketDownloadInfo *sockinfo)
{
    sockinfo->_status = SOCKETINFO_CONNECTING;
//    assert(sockinfo->_socket==NULL);
    if(downloadinfo->_status == DLS_DOWNLOADING && downloadinfo->_invalid == 0)
    {
        DataBlockInfo* datablockinfo = (DataBlockInfo *)downloadinfo->_datablockmempool->GetMemory();
        assert(datablockinfo);
        int len = BUFFER_DATA_BLOCK_SIZE - sizeof(DataBlockInfo);
        if(len > 1024)
        {
            len = 1024;
        }
        
        //DEBUGOUT("%s[%d] downloadinfo=0x%x doc[%s] refcount = %d\n", __FUNCTION__, __LINE__, downloadinfo, downloadinfo->urlinfo.doc, downloadinfo->refcount);
        SOCKET cursock = -1;
        
        //        进行socket连接操作
        if(sockinfo->_socket!=NULL)
        {
            delete sockinfo->_socket;
            sockinfo->_socket = NULL;
        }
        CHttp http(&(downloadinfo->_urlinfo));
        int iret = http.RequestStartDownload(&sockinfo->_socket,
                                    sockinfo->_host,
                                    downloadinfo->_cookie,
                                    &(sockinfo->_posinfo),
                                    datablockinfo->_data,
                                    &len);
        
        //DEBUGOUT("%s[%d] RequestStartDownload iret[%d] len=%d\n", __FUNCTION__, __LINE__, iret, len);
        //        如果该socket已经不可取或者不是下载状态
        if(downloadinfo->_status != DLS_DOWNLOADING || downloadinfo->_invalid)
        {
            DEBUGOUT("%s[%d] RequestStartDownload end!!! iret = %d status[%d]\n", __FUNCTION__, __LINE__, iret, downloadinfo->_status);
            if(iret == 0 && sockinfo->_socket!=NULL)
            {
                sockinfo->_socket->Close();
            }
            sockinfo->_status = SOCKETINFO_NONCONNECT;
            if(iret == -2)//403
            {
                sockinfo->_status = SOCKETINFO_SINFINVALID;
            }
            else if(iret == -3)
            {
                sockinfo->_status = SOCKETINFO_NOFILE;
            }
            
            downloadinfo->_datablockmempool->PutMemoryPool(datablockinfo);
            return ;
        }
        
        // 创建成功
        if(iret == 0)
        {
            sockinfo->_timeoutsec = GetTime();
            sockinfo->_status = SOCKETINFO_CONNECTED;
            if(sockinfo->_socket!=NULL)
            {
                sockinfo->_socket->SetNonBlock();
            }
            
            //          读取了部分数据
            if(len > 0)
            {
                //有数据
//                datablockinfo->_downloadinfo = downloadinfo;
                datablockinfo->_length = len;
                datablockinfo->_posinfo = sockinfo->_posinfo;
                datablockinfo->_index = index;
                sockinfo->_posinfo._offset += len;
                // g_curspeed += len;//todo
                downloadinfo->_dataprocess.PutDataBlock(*downloadinfo, datablockinfo);
            }
            else
            {
                downloadinfo->_datablockmempool->PutMemoryPool(datablockinfo);
                // downloadinfo->_refcount--;//todo
            }
            return ;
        }
        
        downloadinfo->_datablockmempool->PutMemoryPool(datablockinfo);
        
        //        连接出错
        if(iret == -2)
        {
            sockinfo->_status = SOCKETINFO_SINFINVALID;
        }
        else if(iret == -3)
        {
            sockinfo->_status = SOCKETINFO_NOFILE;
        }
        else
        {
            sockinfo->_status = SOCKETINFO_NONCONNECT;
        }
    }
}
