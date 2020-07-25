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
    class DescriptorHeapAllocation
    {
    public:
        // Creates null allocation
        DescriptorHeapAllocation() noexcept :
            // clang-format off
            m_NumHandles{ 0 }, // One null descriptor handle
            m_pDescriptorHeap{ nullptr },
            m_DescriptorSize{ 0 }
            // clang-format on
        {
            m_FirstCpuHandle.ptr = 0;
            m_FirstGpuHandle.ptr = 0;
        }

        // Initializes non-null allocation
        DescriptorHeapAllocation(IDescriptorAllocator& Allocator,
            ID3D12DescriptorHeap* pHeap,
            D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
            uint32_t                    NHandles,
            uint16_t                    AllocationManagerId) noexcept :
            // clang-format off
            m_FirstCpuHandle{ CpuHandle },
            m_FirstGpuHandle{ GpuHandle },
            m_pAllocator{ &Allocator },
            m_NumHandles{ NHandles },
            m_pDescriptorHeap{ pHeap },
            m_AllocationManagerId{ AllocationManagerId }
            // clang-format on
        {
            assert(m_pAllocator != nullptr && m_pDescriptorHeap != nullptr);
            auto DescriptorSize = m_pAllocator->GetDescriptorSize();
            assert("DescriptorSize exceeds allowed limit" && DescriptorSize < std::numeric_limits<uint16_t>::max());
            m_DescriptorSize = static_cast<uint16_t>(DescriptorSize);
        }

        // Move constructor (copy is not allowed)
        DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept :
            // clang-format off
            m_FirstCpuHandle{ std::move(Allocation.m_FirstCpuHandle) },
            m_FirstGpuHandle{ std::move(Allocation.m_FirstGpuHandle) },
            m_NumHandles{ std::move(Allocation.m_NumHandles) },
            m_pAllocator{ std::move(Allocation.m_pAllocator) },
            m_AllocationManagerId{ std::move(Allocation.m_AllocationManagerId) },
            m_pDescriptorHeap{ std::move(Allocation.m_pDescriptorHeap) },
            m_DescriptorSize{ std::move(Allocation.m_DescriptorSize) }
            // clang-format on
        {
            Allocation.Reset();
        }

        // Move assignment (assignment is not allowed)
        DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& Allocation) noexcept
        {
            m_FirstCpuHandle = std::move(Allocation.m_FirstCpuHandle);
            m_FirstGpuHandle = std::move(Allocation.m_FirstGpuHandle);
            m_NumHandles = std::move(Allocation.m_NumHandles);
            m_pAllocator = std::move(Allocation.m_pAllocator);
            m_AllocationManagerId = std::move(Allocation.m_AllocationManagerId);
            m_pDescriptorHeap = std::move(Allocation.m_pDescriptorHeap);
            m_DescriptorSize = std::move(Allocation.m_DescriptorSize);

            Allocation.Reset();

            return *this;
        }

        void Reset()
        {
            m_FirstCpuHandle.ptr = 0;
            m_FirstGpuHandle.ptr = 0;
            m_pAllocator = nullptr;
            m_pDescriptorHeap = nullptr;
            m_NumHandles = 0;
            m_AllocationManagerId = static_cast<uint16_t>(-1);
            m_DescriptorSize = 0;
        }

        // clang-format off
        DescriptorHeapAllocation(const DescriptorHeapAllocation&) = delete;
        DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation&) = delete;
        // clang-format on


        // Destructor automatically releases this allocation through the allocator
        ~DescriptorHeapAllocation()
        {
            if (!IsNull() && m_pAllocator)
                m_pAllocator->Free(std::move(*this));
            // Allocation must have been disposed by the allocator
            assert("Non-null descriptor is being destroyed" && IsNull());
        }

        // Returns CPU descriptor handle at the specified offset
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t Offset = 0) const
        {
            assert(Offset >= 0 && Offset < m_NumHandles);

            D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_FirstCpuHandle;
            CPUHandle.ptr += m_DescriptorSize * Offset;

            return CPUHandle;
        }

        // Returns GPU descriptor handle at the specified offset
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t Offset = 0) const
        {
            assert(Offset >= 0 && Offset < m_NumHandles);
            D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = m_FirstGpuHandle;
            GPUHandle.ptr += m_DescriptorSize * Offset;

            return GPUHandle;
        }

        // Returns pointer to D3D12 descriptor heap that contains this allocation
        ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pDescriptorHeap; }


        // clang-format off
        uint32_t GetNumHandles()          const { return m_NumHandles; }
        bool     IsNull()                 const { return m_FirstCpuHandle.ptr == 0; }
        bool     IsShaderVisible()        const { return m_FirstGpuHandle.ptr != 0; }
        uint16_t GetAllocationManagerId() const { return m_AllocationManagerId; }
        uint16_t GetDescriptorSize()      const { return m_DescriptorSize; }
        // clang-format on

    private:
        // First CPU descriptor handle in this allocation
        D3D12_CPU_DESCRIPTOR_HANDLE m_FirstCpuHandle = { 0 };

        // First GPU descriptor handle in this allocation
        D3D12_GPU_DESCRIPTOR_HANDLE m_FirstGpuHandle = { 0 };

        // Keep strong reference to the parent heap to make sure it is alive while allocation is alive - TOO EXPENSIVE
        //RefCntAutoPtr<IDescriptorAllocator> m_pAllocator;

        // Pointer to the descriptor heap allocator that created this allocation
        IDescriptorAllocator* m_pAllocator = nullptr;

        // Pointer to the D3D12 descriptor heap that contains descriptors in this allocation
        ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;

        // Number of descriptors in the allocation
        uint32_t m_NumHandles = 0;

        // Allocation manager ID. One allocator may support several
        // allocation managers. This field is required to identify
        // the manager within the allocator that was used to create
        // this allocation
        uint16_t m_AllocationManagerId = static_cast<uint16_t>(-1);

        // Descriptor size
        uint16_t m_DescriptorSize = 0;
    };
}