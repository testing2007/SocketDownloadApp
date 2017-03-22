//
//  CPostprocess.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CPostProcess__
#define __DownloadApp__CPostProcess__

#include <stdio.h>
#include "Internal.h"
#include "ActiveObj.h"
#include "CLock.h"

class CDownload;
class CPostprocess
{
public:
    CPostprocess();
    ~CPostprocess();
    void PutDownloadInfoToPostprocess(CDownload& downloadinfo);
};

#endif /* defined(__DownloadApp__CPostProcess__) */
