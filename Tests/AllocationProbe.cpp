#include "AllocationProbe.h"
#include <new>
#include <cstdlib>
#include <malloc.h>
namespace aeriform::test {thread_local bool allocationProbeEnabled=false;thread_local AllocationCounts allocationCounts;}
namespace {
void recordAllocation(size_t size) noexcept {using namespace aeriform::test;if(allocationProbeEnabled){++allocationCounts.allocations;allocationCounts.bytes+=size;}}
void recordFree(void* p) noexcept {using namespace aeriform::test;if(p&&allocationProbeEnabled)++allocationCounts.deallocations;}
}
void* operator new(size_t size){void* p=std::malloc(size?size:1);if(!p)throw std::bad_alloc();recordAllocation(size);return p;}
void* operator new[](size_t size){return ::operator new(size);}
void operator delete(void* p) noexcept {recordFree(p);std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
void* operator new(size_t size,const std::nothrow_t&) noexcept {try{return ::operator new(size);}catch(...){return nullptr;}}
void* operator new[](size_t size,const std::nothrow_t& tag) noexcept {return ::operator new(size,tag);}
void operator delete(void* p,const std::nothrow_t&) noexcept {::operator delete(p);}
void operator delete[](void* p,const std::nothrow_t&) noexcept {::operator delete(p);}
void* operator new(size_t size,std::align_val_t alignment){
#if defined(_WIN32)
    void* p=_aligned_malloc(size?size:1,(size_t)alignment);
#else
    void* p=nullptr;if(posix_memalign(&p,(size_t)alignment,size?size:1)!=0)p=nullptr;
#endif
    if(!p)throw std::bad_alloc();recordAllocation(size);return p;
}
void* operator new[](size_t size,std::align_val_t alignment){return ::operator new(size,alignment);}
void operator delete(void* p,std::align_val_t) noexcept {recordFree(p);
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p,std::align_val_t a) noexcept {::operator delete(p,a);}
void operator delete(void* p,size_t,std::align_val_t a) noexcept {::operator delete(p,a);}
void operator delete[](void* p,size_t,std::align_val_t a) noexcept {::operator delete(p,a);}
