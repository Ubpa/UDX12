// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/include/DescriptorHeap.hpp
// 3. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/src/DescriptorHeap.cpp

#pragma once

#include "IDescriptorAllocator.h"

#include <UContainer/VarSizeAllocMngr.h>

#include <mutex>

namespace Ubpa::DX12 {
    // The class performs suballocations within one D3D12 descriptor heap.
    // It uses VariableSizeAllocationsManager to manage free space in the heap
    //
    // |  X  X  X  X  O  O  O  X  X  O  O  X  O  O  O  O  |  D3D12 descriptor heap
    //
    //  X - used descriptor
    //  O - available descriptor
    //
    class DescriptorHeapAllocMngr
    {
    public:
        // Creates a new D3D12 descriptor heap
        DescriptorHeapAllocMngr(ID3D12Device*                     pDevice,
                                IDescriptorAllocator&             ParentAllocator,
                                size_t                            ThisManagerId,
                                const D3D12_DESCRIPTOR_HEAP_DESC& HeapDesc);

        // Uses subrange of descriptors in the existing D3D12 descriptor heap
        // that starts at offset FirstDescriptor and uses NumDescriptors descriptors
        DescriptorHeapAllocMngr(ID3D12Device*          pDevice,
                                IDescriptorAllocator&  ParentAllocator,
                                size_t                 ThisManagerId,
                                ID3D12DescriptorHeap*  pd3d12DescriptorHeap,
                                uint32_t               FirstDescriptor,
                                uint32_t               NumDescriptors);


        // = default causes compiler error when instantiating std::vector::emplace_back() in Visual Studio 2015 (Version 14.0.23107.0 D14REL)
        DescriptorHeapAllocMngr(DescriptorHeapAllocMngr&& rhs) noexcept :
            // clang-format off
            m_ParentAllocator           {rhs.m_ParentAllocator           },
            m_pDevice                   {rhs.m_pDevice                   },
            m_ThisManagerId             {rhs.m_ThisManagerId             },
            m_HeapDesc                  {rhs.m_HeapDesc                  },
            m_DescriptorSize            {rhs.m_DescriptorSize            },
            m_NumDescriptorsInAllocation{rhs.m_NumDescriptorsInAllocation},
	        m_FirstCPUHandle            {rhs.m_FirstCPUHandle            },
            m_FirstGPUHandle            {rhs.m_FirstGPUHandle            },
            m_MaxAllocatedSize          {rhs.m_MaxAllocatedSize          },
            // Mutex is not movable
            //m_FreeBlockManagerMutex     (std::move(rhs.m_FreeBlockManagerMutex))
            m_FreeBlockManager          {std::move(rhs.m_FreeBlockManager)    },
            m_pd3d12DescriptorHeap      {std::move(rhs.m_pd3d12DescriptorHeap)}
        // clang-format on
        {
            rhs.m_NumDescriptorsInAllocation = 0; // Must be set to zero so that debug check in dtor passes
            rhs.m_ThisManagerId              = static_cast<size_t>(-1);
            rhs.m_FirstCPUHandle.ptr         = 0;
            rhs.m_FirstGPUHandle.ptr         = 0;
            rhs.m_MaxAllocatedSize           = 0;
    #ifdef DILIGENT_DEVELOPMENT
            m_AllocationsCounter.store(rhs.m_AllocationsCounter.load());
            rhs.m_AllocationsCounter = 0;
    #endif
        }

        // clang-format off
        // No copies or move-assignments
        DescriptorHeapAllocMngr& operator = (DescriptorHeapAllocMngr&&)      = delete;
        DescriptorHeapAllocMngr             (const DescriptorHeapAllocMngr&) = delete;
        DescriptorHeapAllocMngr& operator = (const DescriptorHeapAllocMngr&) = delete;
        // clang-format on

        ~DescriptorHeapAllocMngr();

        // Allocates Count descriptors
        DescriptorHeapAllocation Allocate(uint32_t Count);
        void                     FreeAllocation(DescriptorHeapAllocation&& Allocation);

        // clang-format off
        size_t   GetNumAvailableDescriptors() const { return m_FreeBlockManager.GetFreeSize(); }
	    uint32_t GetMaxDescriptors()          const { return m_NumDescriptorsInAllocation;     }
        size_t   GetMaxAllocatedSize()        const { return m_MaxAllocatedSize;               }
        // clang-format on

    private:
        IDescriptorAllocator&  m_ParentAllocator;
        ID3D12Device* m_pDevice;

        // External ID assigned to this descriptor allocations manager
        size_t m_ThisManagerId = static_cast<size_t>(-1);

        // Heap description
        const D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;

        const UINT m_DescriptorSize = 0;

        // Number of descriptors in the allocation.
        // If this manager was initialized as a subrange in the existing heap,
        // this value may be different from m_HeapDesc.NumDescriptors
        uint32_t m_NumDescriptorsInAllocation = 0;

        // Allocations manager used to handle descriptor allocations within the heap
        std::mutex                     m_FreeBlockManagerMutex;
        VarSizeAllocMngr               m_FreeBlockManager;

        // Strong reference to D3D12 descriptor heap object
        CComPtr<ID3D12DescriptorHeap>  m_pd3d12DescriptorHeap;

        // First CPU descriptor handle in the available descriptor range
        D3D12_CPU_DESCRIPTOR_HANDLE    m_FirstCPUHandle = {0};

        // First GPU descriptor handle in the available descriptor range
        D3D12_GPU_DESCRIPTOR_HANDLE    m_FirstGPUHandle = {0};

        size_t m_MaxAllocatedSize = 0;

        // Note: when adding new members, do not forget to update move ctor
    };
}
