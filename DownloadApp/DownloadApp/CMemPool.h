//
//  CMemPool.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CMemPool__
#define __DownloadApp__CMemPool__

#include <stdio.h>
#include <stdlib.h>
#include "CLock.h"
#include <assert.h>
#include <string.h>

struct MemoryItemNode
{
    MemoryItemNode* _next;
//    size_t totalbytes;
//    size_t usedbytes;
//    unsigned char* data[0];
};

class CMemPool
{
public:
    
//    typedef unsigned long MEMORY_POOL_HANDLE;
    
    static CMemPool* CreateMemPool(int nblks, size_t blksize)
    {
        int i = 0;
        MemoryItemNode* head = NULL;
        MemoryItemNode* freeZone = NULL;
        for(i=0; i<nblks; ++i)
        {
            freeZone = (MemoryItemNode*)calloc(1, blksize);
            if(freeZone==NULL)
            {
                //分配失败
                return NULL;
            }
            if(i==0)
            {
                freeZone->_next = NULL;
//                freeZone->totalbytes =  blksize;
//                freeZone->usedbytes = 0;
//                freeZone->used = false;
//                freeZone.data = freeZone+sizeof(
                head = freeZone;
            }
            else
            {
                freeZone->_next = head;
//                freeZone->totalbytes =  blksize;
//                freeZone->usedbytes = 0;
//                freeZone->used = false;
                head = freeZone;
            }
        }
        
        CMemPool *mempool =  new CMemPool();
        mempool->_totalblks = nblks;
        mempool->_blksize = blksize;
        mempool->_freeblks = nblks;
        mempool->_list = head;
        return mempool;
    }
    
    void ReleaseMemPool()
    {
        CAutoLock lock(_lock);
        MemoryItemNode* temp = NULL;
        for(; _list!=NULL; _list=temp->_next)
        {
            temp=_list;
            free(_list);
        }
        this->_blksize = 0;
        this->_freeblks = 0;
        this->_totalblks = 0;
        this->_list = NULL;
    }
    
    void* GetMemory()
    {
        CAutoLock lock(_lock);
        MemoryItemNode* ret = NULL;
        if(this->_list!=NULL && this->_blksize>0 && this->_freeblks>=0 && this->_freeblks<=this->_totalblks)
        {
            if(this->_freeblks==0)
            {
                //需要重新分配内存
                ret = (this->_AllocateOneItemMemory()) ? this->_list : NULL;
                if(ret!=NULL)
                {
//                    this->list->used = true;
                    MemoryItemNode* temp = ret->_next;
                    ret->_next = NULL;
                    this->_list = temp;
                }
                this->_freeblks = this->_freeblks-- >= 0 ? this->_freeblks : 0;
                this->_totalblks = this->_totalblks-- >= 0 ? this->_totalblks : 0;
            }
            else
            {
                ret = this->_list;
                this->_list  = ret->_next;
                ret->_next = NULL;
                this-_freeblks--;
            }
            
        }
        return ret;
    }
    
    void  PutMemoryPool(void* allocate_mem)
    {
        CAutoLock lock(_lock);
        assert(allocate_mem!=NULL);
        ((MemoryItemNode*)allocate_mem)->_next = (_list!=NULL) ? _list : NULL;
        _list = (MemoryItemNode*)allocate_mem;
        memset(_list, 0, _blksize);
        this->_freeblks++;
    }
    
    int GetUsedSize()
    {
        CAutoLock lock(_lock);
        return _totalblks-_freeblks;
    }
    
    int GetTotalSize()
    {
        CAutoLock lock(_lock);
        return _totalblks;
    }
    
    int GetFreeSize()
    {
        CAutoLock lock(_lock);
        return _freeblks;
    }

private:
    bool _AllocateOneItemMemory()
    {
        MemoryItemNode *freeZone = (MemoryItemNode*)calloc(1, _blksize);
        if(freeZone==NULL)
        {
            //分配失败
            return false;
        }
        freeZone->_next = this->_list;
//        freeZone->totalbytes =  blksize;
//        freeZone->usedbytes = 0;
//      freeZone->used = false;
        this->_list = freeZone;
        this->_freeblks++;
        this->_totalblks++;
        return true;
    }

private:
    int _blksize;
    int _freeblks;
    int _totalblks;
    MemoryItemNode *_list;
    CLock _lock;
};

#endif /* defined(__DownloadApp__CMemPool__) */
