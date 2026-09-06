#pragma once
#include <cstddef>
namespace aeriform::test {
struct AllocationCounts {size_t allocations=0,deallocations=0,bytes=0;};
extern thread_local bool allocationProbeEnabled;
extern thread_local AllocationCounts allocationCounts;
class AllocationProbe {
public:
    AllocationProbe() noexcept {allocationCounts={};allocationProbeEnabled=true;}
    ~AllocationProbe(){allocationProbeEnabled=false;}
    AllocationCounts finish() noexcept {allocationProbeEnabled=false;return allocationCounts;}
};
}
