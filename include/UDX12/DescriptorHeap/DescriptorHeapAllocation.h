// ref
// 1. http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-descriptor-heaps/
// 2. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/include/DescriptorHeap.hpp
// 3. https://github.com/DiligentGraphics/DiligentCore/blob/master/Graphics/GraphicsEngineD3D12/src/DescriptorHeap.cpp

#pragma once

#include "IDescriptorAllocator.h"

namespace Ubpa::UDX12 {
    // The class represents descriptor heap allocation (continuous descriptor range in a descriptor heap)
    //
    //                  m_FirstCpuHandle
    //                   |
    //  | ~  ~  ~  ~  ~  X  X  X  X  X  X  X  ~  ~  ~  ~  ~  ~ |  D3D12 Descriptor Heap
    //                   |
    //                  m_FirstGpuHandle
    //
    class DescriptorHeapAllocation {
    public:
        // Creates null allocation
        DescriptorHeapAllocation() noexcept;

        // Initializes non-null allocation
        DescriptorHeapAllocation(
            IDescriptorAllocator*       pAllocator,
            ID3D12DescriptorHeap*       pHeap,
            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
            uint32_t                    NHandles,
            uint16_t                    AllocationManagerId
        ) noexcept;

        // Move constructor (copy is not allowed)
        DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept;

        // Destructor automatically releases this allocation through the allocator
        ~DescriptorHeapAllocation();

        // Move assignment (assignment is not allowed)
        DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& Allocation) noexcept;

        void Reset() noexcept;

        DescriptorHeapAllocation(const DescriptorHeapAllocation&) = delete;
        DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation&) = delete;

        // Returns CPU descriptor handle at the specified offset
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t Offset = 0) const noexcept {
            assert(Offset >= 0 && Offset < m_NumHandles);
            return { m_FirstCpuHandle.ptr + m_DescriptorSize * Offset };
        }

        // Returns GPU descriptor handle at the specified offset
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t Offset = 0) const noexcept {
            assert(Offset >= 0 && Offset < m_NumHandles);
            return { m_FirstGpuHandle.ptr + m_DescriptorSize * Offset };
        }

        // Returns pointer to D3D12 descriptor heap that contains this allocation
        ID3D12DescriptorHeap* GetDescriptorHeap() const noexcept { return m_pDescriptorHeap; }

        uint32_t GetNumHandles()          const noexcept { return m_NumHandles; }
        bool     IsNull()                 const noexcept { return m_FirstCpuHandle.ptr == 0; }
        bool     IsShaderVisible()        const noexcept { return m_FirstGpuHandle.ptr != 0; }
        uint16_t GetAllocationManagerId() const noexcept { return m_AllocationManagerId; }
        uint16_t GetDescriptorSize()      const noexcept { return m_DescriptorSize; }

    private:
        // First CPU descriptor handle in this allocation
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle{ 0 };

        // First GPU descriptor handle in this allocation
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle{ 0 };

        // Pointer to the descriptor heap allocator that created this allocation
        IDescriptorAllocator* m_pAllocator{ nullptr };

        // Pointer to the D3D12 descriptor heap that contains descriptors in this allocation
        ID3D12DescriptorHeap* m_pDescriptorHeap{ nullptr };

        // Number of descriptors in the allocation
        uint32_t m_NumHandles{ 0 };

        // Allocation manager ID. One allocator may support several
        // allocation managers. This field is required to identify
        // the manager within the allocator that was used to create
        // this allocation
        uint16_t m_AllocationManagerId{ static_cast<uint16_t>(-1) };

        // Descriptor size
        uint16_t m_DescriptorSize{ 0 };
    };
}
