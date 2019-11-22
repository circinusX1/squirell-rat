/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS


#ifndef MEM_POOL
#include <stdint.h>
#include "sqmem.h"

minipool<128>       _pool128(128);
minipool<512>       _pool512(32);
minipool<1024>      _pool1k(16);
minipool<4096>      _pool4k(16);
minipool<16384>     _pool16k(16);

static size_t      _maxbytes = 0;
static size_t   _maxblock = 0;

void *sq_vm_malloc(size_t size)
{
    _maxbytes+=size;
    _maxblock = std::max(_maxblock,size);

   // printf("alloc sz=%d ammnt=%d maxblk=%d \n", size, _maxbytes, _maxblock);

    if(size<=128)
        return _pool128.alloc();
    if(size<=512)
        return _pool512.alloc();
    if(size<=1024)
        return _pool1k.alloc();
    if(size<=4096)
        return _pool4k.alloc();
    if(size<=16384)
        return _pool16k.alloc();
    printf("fall trough ... \n");
    return malloc(size);
}

void *sq_vm_realloc(void *p, size_t oldsiz, size_t size)
{
    _maxbytes -= oldsiz;
    _maxbytes += size;
    _maxblock = std::max(_maxblock,size);

   // printf("realloc sz=%d ammnt=%d maxblk=%d \n", size, _maxbytes, _maxblock);

    void *pn = nullptr;
    if(size<=128)
    {
        pn=_pool128.alloc();
        goto DONE;
    }
    if(size<=512)
    {
        pn=_pool512.alloc();
        goto DONE;
    }
    if(size<=1024)
    {
        pn=_pool1k.alloc();
        goto DONE;
    }
    if(size<=4096)
    {
        pn=_pool4k.alloc();
        goto DONE;
    }
    if(size<=16384)
        pn=_pool16k.alloc();
DONE:
    if(pn)
    {
        if(p)
        {
            ::memcpy(pn, p, oldsiz);
            ::sq_vm_free(p, oldsiz);
        }
        return pn;
    }
    return ::realloc(p, size);
}

void sq_vm_free(void *p, size_t size)
{
    _maxbytes-=size;

  //  printf("free sz=%d ammnt=%d maxblk=%d \n", size, _maxbytes, _maxblock);

    if(size<=128)
    {    _pool128.free(p);; return;}
    if(size<=512)
    {   _pool512.free(p);; return;}
    if(size<=1024)
    {   _pool1k.free(p);; return;}
    if(size<=4096)
    {   _pool4k.free(p);; return;}
    if(size<=16384)
    {   _pool16k.free(p); return;}
    ::free(p);
    return ;
}



#else

void *sq_vm_malloc(size_t size){ return malloc(size); }
void *sq_vm_realloc(void *p, size_t SQ_UNUSED_ARG(oldsize), size_t size){ return realloc(p, size); }
void sq_vm_free(void *p, size_t SQ_UNUSED_ARG(size)){ free(p); }

#endif

#endif
