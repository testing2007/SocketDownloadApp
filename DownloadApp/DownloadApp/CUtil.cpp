//
//  CUtil.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CUtil.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "CLock.h"
#include "Internal.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <stdlib.h>
#elif defined(__LINUX__)

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#endif


CLock g_lock;

void DEBUGOUT(const char*format, ...)
{
//    CAutoLock _lock(g_lock);
//    va_list varg;
//    char outputstr[4096] = {0};
//    char *ptr;
//    int len;
//    
//    //sprintf(outputstr, "%d : ", GetTime());
//    len = strlen(outputstr);
//    ptr = outputstr + len;
//    
//    va_start(varg, format);
//    vsnprintf(ptr, 4096 - len - 2, format, varg);
//    va_end(varg);
//    
//#if defined(WIN32)
//    OutputDebugStringA(outputstr);
//    printf(outputstr);
//#elif defined(ADRIOD_DEVICE)
//    __android_log_print(ANDROID_LOG_DEBUG, "ndk", outputstr);
//#else
//    strcat(ptr, "\n");
//    printf("%s", outputstr);
//#endif
    
    va_list args;
    char content[4096] = {0};
    char *ptr;
    int len;
    
    len = strlen(content);
    ptr = content+len;
    
    va_start(args, format);
    vsnprintf(ptr, 4096-len-2, format, args);
    va_end(args);
    
    strcat(ptr, "\n");
    printf("%s", content);
}

unsigned long GetTime(void)
{
#ifdef WIN32
    return GetTickCount();
#elif __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    return mts.tv_sec * 1000 + mts.tv_nsec / 1000000;
#else
    clock_gettime(CLOCK_REALTIME, ts);
    struct timeval now;
    struct timespec tsnow;
    if(0 == clock_gettime(CLOCK_MONOTONIC, &tsnow)) {
        return tsnow.tv_sec * 1000 + tsnow.tv_nsec / 1000000;
    }
    gettimeofday(&now, 0);
    return now.tv_sec * 1000 + tv_usec / 1000;
#endif
}

void Sleep(unsigned int ms)
{
    usleep(ms*1000);
}

//调整字节序
static unsigned int conv(unsigned int a)
{
    unsigned int b=0;
    b|=(a<<24)&0xff000000;
    b|=(a<<8)&0x00ff0000;
    b|=(a>>8)&0x0000ff00;
    b|=(a>>24)&0x000000ff;
    return b;
}

//md5数据转换为字符串
void GetMD5String(md5_byte_t digest[16], unsigned char *strmd5)
{
    unsigned int *ptr = (unsigned int *)digest;
    char tmpstrmd5[MD5_VALUE_LEN + 4] = {0};
    snprintf(tmpstrmd5, MD5_VALUE_LEN + 4, "%08x%08x%08x%08x", conv(*ptr), conv(ptr[1]), conv(ptr[2]), conv(ptr[3]));
    //DEBUGOUT("%s[%d] strmd5=%s\n", __FUNCTION__, __LINE__, tmpstrmd5);
    memcpy(strmd5, tmpstrmd5, MD5_VALUE_LEN);
}

//获取md5值
int GetBuffMD5Value(const char* buffer, int count, unsigned char *md5)
{
    md5_state_t state;
    md5_byte_t digest[16];
    
    memset(md5, 0, MD5_VALUE_LEN);
    if(count <= 0)
    {
        return -1;
    }
    
    memset(md5, 0, MD5_VALUE_LEN);
    md5_init(&state);
    md5_append(&state,(md5_byte_t*)buffer, count);
    md5_finish(&state, digest);
    GetMD5String(digest, md5);
    return 0;
}

//获取文件的md5值
int GetFileMD5Value(const char* sfilename, unsigned char *md5)
{
    FILE *fp;
    md5_state_t state;
    md5_byte_t digest[16];
    
    static unsigned char buffer[4096];
    int count;
    
    fp = fopen(sfilename, FILE_READ_MODE);
    if(fp == NULL)
    {
        DEBUGOUT("Can not open this file!\n");
        return -1;
    }
    
    memset(md5, 0, MD5_VALUE_LEN);
    md5_init(&state);
    while(!feof(fp))
    {
        count = fread(buffer, 1, 4096, fp);
        if(count > 0)
        {
            md5_append(&state, buffer, count);
        }
    }
    fclose(fp);
    md5_finish(&state, digest);
    GetMD5String(digest, md5);
    return 0;
}