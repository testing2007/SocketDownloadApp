//
//  CDataHandler.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CDataHandler__
#define __DownloadApp__CDataHandler__

#include <stdio.h>
#include "Internal.h"
#include "CLock.h"
#include "ActiveObj.h"

class CDownload;
class CDataProcess
{
public:
    CDataProcess();
    ~CDataProcess();

    //将待处理数据节点放入数据list
    void PutDataBlock(CDownload& download, DataBlockInfo* datablock);
};

#endif /* defined(__DownloadApp__CDataHandler__) */
