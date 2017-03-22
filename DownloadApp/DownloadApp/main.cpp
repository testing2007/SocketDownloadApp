//
//  main.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include <iostream>
#include "CUtil.h"
#include "CDownloadManager.h"

int DownloadStatusCallback(enum DLSTATUS status, void* usrparam, size_t curbytes, size_t filesize)
{
    static size_t s_curbytes = 0;
    
    if (status == DLS_COMPLETE)
    {
        int i = 0;
        i++;
        i++;
    }
    printf("ipa status = %d speed = %d bytes curbytes = %u totalsize=%u\n", status, (curbytes - s_curbytes) * 2 / 1024, curbytes, filesize);
    s_curbytes = curbytes;
    return 0;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    //http://jbcdn2.b0.upaiyun.com/2015/08/06c667f00b09132c6e554d962c93e534.jpg
    CDownloadManager *dm = CDownloadManager::Instance();
    dm->InitDownload(0);
    dm->SetSavePath("/Users/weizhiqiang/Documents/GitHubProject/ios_MoreMore/TestDownloadPrj/TestDownloadPrj/");
//    dm->SetUserAgent("duoduotest");
    //    SetAppInfoInterface(APP_INFO_URL_PREFIX);
    
    const char *url = "http://i2.sinaimg.cn/photo/2/2013-06-17/U11472P1505T2D2F46DT20150813114806.jpg";
    dm->DownloadFileFromUrl(url, NULL, NULL, DownloadStatusCallback, NULL);
    
    while (1) {
        Sleep(1000);
    }
    //    printf("user input q to exit the program\n");
    //    char *cmd = (char*)malloc(10);
    //    memset(cmd, 0, 10);
    //    scanf("%s", &cmd);
    //    while(strcmp(cmd, "q")!=0)
    //    {
    //        scanf("%s", &cmd);
    //    }
    return 0;
}

//int main(int argc, const char * argv[]) {
//    // insert code here...
//    std::cout << "Hello, World!\n";
//    return 0;
//}
