// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/09b9f1dc1ec69683c3143f309e6e8a0141361624/Graphics/GraphicsAccessories/interface/VarSizeAllocMngr.hpp
// 3. https://github.com/DiligentGraphics/DiligentCore/blob/09b9f1dc1ec69683c3143f309e6e8a0141361624/Graphics/GraphicsEngineD3D12/src/DescriptorHeap.cpp

#pragma once

#include <map>

namespace Ubpa::UDX12 {
    // The class handles free memory block management to accommodate variable-size allocation requests.
    // It keeps track of free blocks only and does not record allocation sizes. The class uses two ordered maps
    // to facilitate operations. The first map keeps blocks sorted by their offsets. The second multimap keeps blocks
    // sorted by their sizes. The elements of the two maps reference each other, which enables efficient block
    // insertion, removal and merging.
    //
    //   8                 32                       64                           104
    //   |<---16--->|       |<-----24------>|        |<---16--->|                 |<-----32----->|
    //
    //
    //          size2freeblock         offset2freeblock
    //           size->offset            offset->size
    //
    //                16 ------------------>   8 ---------->  {size = 16, &size2freeblock[0]}
    //
    //                16 ------.   .------->  32 ---------->  {size = 24, &size2freeblock[2]}
    //                          '.'
    //                24 -------' '-------->  64 ---------->  {size = 16, &size2freeblock[1]}
    //
    //                32 ------------------> 104 ---------->  {size = 32, &size2freeblock[3]}
    //
    class VarSizeAllocMngr {
    public:
        VarSizeAllocMngr(size_t capacity);
        VarSizeAllocMngr(VarSizeAllocMngr&&) noexcept;

        VarSizeAllocMngr& operator=(VarSizeAllocMngr&&) noexcept;

        VarSizeAllocMngr(const VarSizeAllocMngr&) = delete;
        VarSizeAllocMngr& operator=(const VarSizeAllocMngr&) = delete;

        // Offset returned by Allocate() may not be aligned, but the size of the allocation
        // is sufficient to properly align it
        struct Allocation {
            static constexpr size_t InvalidOffset = static_cast<size_t>(-1);

            static constexpr Allocation Invalid() noexcept { return {}; }

            constexpr bool IsValid() const noexcept { return unalignedOffset != Invalid().unalignedOffset; }
            constexpr void Reset() noexcept { *this = Invalid(); }

            size_t unalignedOffset{ InvalidOffset };
            size_t size{ 0 };
        };

        Allocation Allocate(size_t size, size_t alignment);

        void Free(Allocation&& allocation) {
            Free(allocation.unalignedOffset, allocation.size);
            allocation.Reset();
        }
        void Free(size_t offset, size_t size);

        bool IsFull() const noexcept { return freeSize == 0; };
        bool IsEmpty() const noexcept { return freeSize == capacity; };
        size_t GetCapacity() const noexcept { return capacity; }
        size_t GetFreeSize() const noexcept { return freeSize; }
        size_t GetUsedSize() const noexcept { return capacity - freeSize; }

    private:
        void AddNewBlock(size_t Offset, size_t Size);

        void ResetCurrAlignment() noexcept;

        struct FreeBlockInfo;

        // Type of the map that keeps memory blocks sorted by their offsets
        using TFreeBlocksByOffsetMap = std::map<size_t, FreeBlockInfo>;

        // Type of the map that keeps memory blocks sorted by their sizes
        using TFreeBlocksBySizeMap = std::multimap<size_t, TFreeBlocksByOffsetMap::iterator>;

        struct FreeBlockInfo {
            // Block size (no reserved space for the size of the allocation)
            size_t size;

            // Iterator referencing this block in the multimap sorted by the block size
            TFreeBlocksBySizeMap::iterator OrderBySizeIt;

            FreeBlockInfo(size_t size) : size(size) {}
        };

        TFreeBlocksByOffsetMap offset2freeblock;
        TFreeBlocksBySizeMap   size2freeblock;

        size_t capacity = 0;
        size_t freeSize = 0;
        size_t curMinAlignment = 0; // min alignment of all free blocks
    };
}
