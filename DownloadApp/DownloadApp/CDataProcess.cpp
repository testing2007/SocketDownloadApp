//
//  CDataProcess.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CDataProcess.h"
#include "CDownload.h"

CDataProcess::CDataProcess()
{
    
}

CDataProcess::~CDataProcess()
{
    
}

void CDataProcess::PutDataBlock(CDownload& download, DataBlockInfo* datablock)
{
    DLSTATUS status;
    int iret = download._dlfileinfo.WriteDataToFile(download, datablock);
    
    //datablock->_downloadinfo->_refcount--;
//    datablock->_downloadinfo->_datablockmempool->PutMemoryPool(datablock);
    //MemPoolPut(curdatablock->downloadinfo->datablockmempool, curdatablock);
}
