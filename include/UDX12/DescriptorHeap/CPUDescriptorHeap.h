// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/include/DescriptorHeap.hpp
// 3. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/src/DescriptorHeap.cpp

#pragma once

#include "IDescriptorAllocator.h"
#include "DescriptorHeapAllocMngr.h"

#include <unordered_set>

namespace Ubpa::UDX12 {
	// CPU descriptor heap is intended to provide storage for resource view descriptor handles.
    // It contains a pool of DescriptorHeapAllocationManager object instances, where every instance manages
    // its own CPU-only D3D12 descriptor heap:
    //
    //           m_HeapPool[0]                m_HeapPool[1]                 m_HeapPool[2]
    //   |  X  X  X  X  X  X  X  X |, |  X  X  X  O  O  X  X  O  |, |  X  O  O  O  O  O  O  O  |
    //
    //    X - used descriptor                m_AvailableHeaps = {1,2}
    //    O - available descriptor
    //
    // Allocation routine goes through the list of managers that have available descriptors and tries to process
    // the request using every manager. If there are no available managers or no manager was able to handle the request,
    // the function creates a new descriptor heap manager and lets it handle the request
    //
    // Render device contains four CPUDescriptorHeap object instances (one for each D3D12 heap type). The heaps are accessed
    // when a texture or a buffer view is created.
    //
    class CPUDescriptorHeap final : public IDescriptorAllocator {
    public:
        // Initializes the heap
        CPUDescriptorHeap(ID3D12Device*               pDevice,
                          uint32_t                    NumDescriptorsInHeap,
                          D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                          D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

        CPUDescriptorHeap            (const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap            (CPUDescriptorHeap&&)      = delete;
        CPUDescriptorHeap& operator= (const CPUDescriptorHeap&) = delete;
        CPUDescriptorHeap& operator= (CPUDescriptorHeap&&)      = delete;

        ~CPUDescriptorHeap();

        virtual DescriptorHeapAllocation Allocate(uint32_t Count) override final;
        virtual void                     Free(DescriptorHeapAllocation&& Allocation) override final;
        virtual uint32_t                 GetDescriptorSize() const noexcept override final { return m_DescriptorSize; }

    private:
        ID3D12Device* m_pDevice;

        // Pool of descriptor heap managers
        std::mutex                                    m_HeapPoolMutex;
        std::vector<DescriptorHeapAllocMngr>          m_HeapPool;
        // Indices of available descriptor heap managers
        std::unordered_set<size_t>                    m_AvailableHeaps;

        D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        const UINT                 m_DescriptorSize = 0;

        // Maximum heap size during the application lifetime - for statistic purposes
        uint32_t m_MaxSize     = 0;
        uint32_t m_CurrentSize = 0;
    };
}
