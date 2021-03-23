#include <UDX12/VarSizeAllocMngr.h>

#include <utility>
#include <cassert>
#include <type_traits>

using namespace Ubpa::UDX12;

namespace Ubpa::UDX12::details {
    template <typename T>
    constexpr bool IsPowerOfTwo(T val) noexcept {
        if constexpr (std::is_unsigned_v<T>)
            return (val & (val - 1)) == 0;
        else
            return val > 0 && (val & (val - 1)) == 0;
    }

    template <typename T>
    constexpr T Align(T val, T alignment) noexcept {
        assert(IsPowerOfTwo(alignment));
        //    ceil(val \div alignment)
        // == (val + alignment - 1) / alignment
        // == (val + (alignment - 1)) & ~(alignment - 1)
        return (val + (alignment - 1)) & ~(alignment - 1);
    }

    template <typename T>
    constexpr T AlignDown(T val, T alignment) noexcept {
        assert(IsPowerOfTwo(alignment));
        //    floor(val \div alignment)
        // == val / alignment
        // == val & ~(alignment - 1)
        return val & ~(alignment - 1);
    }
}

VarSizeAllocMngr::VarSizeAllocMngr(size_t capacity) :
    capacity(capacity),
    freeSize(capacity)
{
    // Insert single maximum-size block
    AddNewBlock(0, capacity);
    ResetCurrAlignment();
}

VarSizeAllocMngr::VarSizeAllocMngr(VarSizeAllocMngr&& rhs) noexcept :
    offset2freeblock{ std::move(rhs.offset2freeblock) },
    size2freeblock{ std::move(rhs.size2freeblock) },
    capacity{ rhs.capacity },
    freeSize{ rhs.freeSize },
    curMinAlignment{ rhs.curMinAlignment }
{
    rhs.capacity = 0;
    rhs.freeSize = 0;
    rhs.curMinAlignment = 0;
}

VarSizeAllocMngr& VarSizeAllocMngr::operator=(VarSizeAllocMngr&& rhs) noexcept {
    offset2freeblock = std::move(rhs.offset2freeblock);
    size2freeblock = std::move(rhs.size2freeblock);
    capacity = rhs.capacity;
    freeSize = rhs.freeSize;
    curMinAlignment = rhs.curMinAlignment;

    rhs.capacity = 0;
    rhs.freeSize = 0;
    rhs.curMinAlignment = 0;

    return *this;
}

void VarSizeAllocMngr::AddNewBlock(size_t Offset, size_t Size) {
    auto [newBlockIter, success] = offset2freeblock.emplace(Offset, Size);
    assert(success);
    auto orderIter = size2freeblock.emplace(Size, newBlockIter);
    newBlockIter->second.OrderBySizeIt = orderIter;
}

void VarSizeAllocMngr::ResetCurrAlignment() noexcept {
    curMinAlignment = 1;
    while (curMinAlignment * 2 <= capacity)
        curMinAlignment *= 2;
}

VarSizeAllocMngr::Allocation VarSizeAllocMngr::Allocate(size_t size, size_t alignment) {
    assert(size > 0);
    assert("alignment must be power of 2" && details::IsPowerOfTwo(alignment));
    size = details::Align(size, alignment);
    if (freeSize < size)
        return Allocation::Invalid();

    auto alignmentReserve = (alignment > curMinAlignment) ? alignment - curMinAlignment : 0;
    // Get the first block that is large enough to encompass size + alignmentReserve bytes
    // lower_bound() returns an iterator pointing to the first element that
    // is not less (i.e. >= ) than key
    auto SmallestBlockItIt = size2freeblock.lower_bound(size + alignmentReserve);
    if (SmallestBlockItIt == size2freeblock.end())
        return Allocation::Invalid();

    auto SmallestBlockIt = SmallestBlockItIt->second;
    assert(size + alignmentReserve <= SmallestBlockIt->second.size);
    assert(SmallestBlockIt->second.size == SmallestBlockItIt->first);

    //     SmallestBlockIt.Offset
    //        |                                  |
    //        |<------SmallestBlockIt.size------>|
    //        |<------size------>|<---newSize--->|
    //        |                  |
    //      offset              newOffset
    //
    auto offset = SmallestBlockIt->first;
    assert(offset % curMinAlignment == 0);
    auto alignedOffset = details::Align(offset, alignment);
    auto adjustedSize = size + (alignedOffset - offset);
    assert(adjustedSize <= size + alignmentReserve);

    auto newOffset = offset + adjustedSize;
    auto newSize = SmallestBlockIt->second.size - adjustedSize;
    assert(SmallestBlockItIt == SmallestBlockIt->second.OrderBySizeIt);
    size2freeblock.erase(SmallestBlockItIt);
    offset2freeblock.erase(SmallestBlockIt);
    if (newSize > 0)
        AddNewBlock(newOffset, newSize);

    freeSize -= adjustedSize;

    if ((size & (curMinAlignment - 1)) != 0) {
        if (details::IsPowerOfTwo(size)) {
            assert(size >= alignment && size < curMinAlignment);
            curMinAlignment = size;
        }
        else
            curMinAlignment = std::min(curMinAlignment, alignment);
    }
    return Allocation{ offset, adjustedSize };
}

void VarSizeAllocMngr::Free(size_t offset, size_t size) {
    assert(offset + size <= capacity);

    // Find the first element whose offset is greater than the specified offset.
    // upper_bound() returns an iterator pointing to the first element in the
    // container whose key is considered to go after k.
    auto nextBlockIt = offset2freeblock.upper_bound(offset);
    // Block being deallocated must not overlap with the next block
    assert(nextBlockIt == offset2freeblock.end() || offset + size <= nextBlockIt->first);
    auto prevBlockIt = nextBlockIt;
    if (prevBlockIt != offset2freeblock.begin()) {
        --prevBlockIt;
        // Block being deallocated must not overlap with the previous block
        assert(offset >= prevBlockIt->first + prevBlockIt->second.size);
    }
    else
        prevBlockIt = offset2freeblock.end();

    size_t newSize, newOffset;
    if (prevBlockIt != offset2freeblock.end() && offset == prevBlockIt->first + prevBlockIt->second.size)
    {
        //  PrevBlock.Offset             Offset
        //       |                          |
        //       |<-----PrevBlock.Size----->|<------Size-------->|
        //
        newSize = prevBlockIt->second.size + size;
        newOffset = prevBlockIt->first;

        if (nextBlockIt != offset2freeblock.end() && offset + size == nextBlockIt->first)
        {
            //   PrevBlock.Offset           Offset            NextBlock.Offset
            //     |                          |                    |
            //     |<-----PrevBlock.Size----->|<------Size-------->|<-----NextBlock.Size----->|
            //
            newSize += nextBlockIt->second.size;
            size2freeblock.erase(prevBlockIt->second.OrderBySizeIt);
            size2freeblock.erase(nextBlockIt->second.OrderBySizeIt);
            // Delete the range of two blocks
            ++nextBlockIt;
            offset2freeblock.erase(prevBlockIt, nextBlockIt);
        }
        else
        {
            //   PrevBlock.Offset           Offset                     NextBlock.Offset
            //     |                          |                             |
            //     |<-----PrevBlock.Size----->|<------Size-------->| ~ ~ ~  |<-----NextBlock.Size----->|
            //
            size2freeblock.erase(prevBlockIt->second.OrderBySizeIt);
            offset2freeblock.erase(prevBlockIt);
        }
    }
    else if (nextBlockIt != offset2freeblock.end() && offset + size == nextBlockIt->first)
    {
        //   PrevBlock.Offset                   Offset            NextBlock.Offset
        //     |                                  |                    |
        //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->|<-----NextBlock.Size----->|
        //
        newSize = size + nextBlockIt->second.size;
        newOffset = offset;
        size2freeblock.erase(nextBlockIt->second.OrderBySizeIt);
        offset2freeblock.erase(nextBlockIt);
    }
    else
    {
        //   PrevBlock.Offset                   Offset                     NextBlock.Offset
        //     |                                  |                            |
        //     |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
        //
        newSize = size;
        newOffset = offset;
    }

    AddNewBlock(newOffset, newSize);

    freeSize += size;
    if (IsEmpty()) {
        // Reset current alignment
        assert(offset2freeblock.size() == 1);
        ResetCurrAlignment();
    }
}
