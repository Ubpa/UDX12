// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/include/DescriptorHeap.hpp
// 3. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/src/DescriptorHeap.cpp

#pragma once

#include "IDescriptorAllocator.h"
#include "DescriptorHeapAllocation.h"
#include "DescriptorHeapAllocMngr.h"

namespace Ubpa::UDX12 {
	// GPU descriptor heap provides storage for shader-visible descriptors
    // The heap contains single D3D12 descriptor heap that is split into two parts.
    // The first part stores static and mutable resource descriptor handles.
    // The second part is intended to provide temporary storage for dynamic resources.
    // Space for dynamic resources is allocated in chunks, and then descriptors are suballocated within every
    // chunk. DynamicSuballocMngr facilitates this process.
    //
    //
    //     static and mutable handles      ||                 dynamic space
    //                                     ||    chunk 0     chunk 1     chunk 2     unused
    //  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||
    //                                               |         |
    //                                     suballocation       suballocation
    //                                    within chunk 0       within chunk 1
    //
    // Render device contains two GPUDescriptorHeap instances (CBV_SRV_UAV and SAMPLER). The heaps
    // are used to allocate GPU-visible descriptors for shader resource binding objects. The heaps
    // are also used by the command contexts (through DynamicSuballocMngr to allocated dynamic descriptors)
    //
    //  _______________________________________________________________________________________________________________________________
    // | Render Device                                                                                                                 |
    // |                                                                                                                               |
    // | m_CPUDescriptorHeaps[CBV_SRV_UAV] |  X  X  X  X  X  X  X  X  |, |  X  X  X  X  X  X  X  X  |, |  X  O  O  X  O  O  O  O  |    |
    // | m_CPUDescriptorHeaps[SAMPLER]     |  X  X  X  X  O  O  O  X  |, |  X  O  O  X  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[RTV]         |  X  X  X  O  O  O  O  O  |, |  O  O  O  O  O  O  O  O  |                                  |
    // | m_CPUDescriptorHeaps[DSV]         |  X  X  X  O  X  O  X  O  |                                                                |
    // |                                                                               ctx1        ctx2                                |
    // | m_GPUDescriptorHeaps[CBV_SRV_UAV]  | X O O X X O X O O O O X X X X O  ||  | X X X O | | X X O O | | O O O O |  O O O O  ||    |
    // | m_GPUDescriptorHeaps[SAMPLER]      | X X O O X O X X X O O X O O O O  ||  | X X O O | | X O O O | | O O O O |  O O O O  ||    |
    // |                                                                                                                               |
    // |_______________________________________________________________________________________________________________________________|
    //
    //  ________________________________________________               ________________________________________________
    // |Device Context 1                                |             |Device Context 2                                |
    // |                                                |             |                                                |
    // | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |             | m_DynamicGPUDescriptorAllocator[CBV_SRV_UAV]   |
    // | m_DynamicGPUDescriptorAllocator[SAMPLER]       |             | m_DynamicGPUDescriptorAllocator[SAMPLER]       |
    // |________________________________________________|             |________________________________________________|
    //
    class GPUDescriptorHeap final : public IDescriptorAllocator {
    public:
        GPUDescriptorHeap(ID3D12Device*               pDevice,
                          uint32_t                    NumDescriptorsInHeap,
                          uint32_t                    NumDynamicDescriptors,
                          D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                          D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

        GPUDescriptorHeap            (const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap            (GPUDescriptorHeap&&)      = delete;
        GPUDescriptorHeap& operator= (const GPUDescriptorHeap&) = delete;
        GPUDescriptorHeap& operator= (GPUDescriptorHeap&&)      = delete;

        virtual DescriptorHeapAllocation Allocate(uint32_t count) override final
        { return m_HeapAllocationManager.Allocate(count); }

        DescriptorHeapAllocation AllocateDynamic(uint32_t count)
        { return m_DynamicAllocationsManager.Allocate(count); }

        virtual void     Free(DescriptorHeapAllocation&&) override final;
        virtual uint32_t GetDescriptorSize() const override final { return m_DescriptorSize; }

        const D3D12_DESCRIPTOR_HEAP_DESC& GetHeapDesc() const noexcept { return m_HeapDesc; }
        uint32_t                          GetMaxStaticDescriptors() const noexcept { return m_HeapAllocationManager.GetMaxDescriptors(); }
        uint32_t                          GetMaxDynamicDescriptors() const noexcept { return m_DynamicAllocationsManager.GetMaxDescriptors(); }
        ID3D12DescriptorHeap*             GetDescriptorHeap() const noexcept { return m_pd3d12DescriptorHeap.p; }

    protected:
        ID3D12Device* m_Device;

        const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
        CComPtr<ID3D12DescriptorHeap>    m_pd3d12DescriptorHeap;

        const UINT m_DescriptorSize;

        static constexpr size_t StaticHeapAllocatonManagerID  = 0;
        static constexpr size_t DynamicHeapAllocatonManagerID = 1;

        // Allocation manager for static/mutable part
        DescriptorHeapAllocMngr m_HeapAllocationManager;

        // Allocation manager for dynamic part
        DescriptorHeapAllocMngr m_DynamicAllocationsManager;
    };
}