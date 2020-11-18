#include <UDX12/DescriptorHeap/DescriptorHeapAllocation.h>

using namespace Ubpa;

UDX12::DescriptorHeapAllocation::DescriptorHeapAllocation() noexcept :
    m_NumHandles{ 0 }, // One null descriptor handle
    m_pDescriptorHeap{ nullptr },
    m_DescriptorSize{ 0 }
{
    m_FirstCpuHandle.ptr = 0;
    m_FirstGpuHandle.ptr = 0;
}

UDX12::DescriptorHeapAllocation::DescriptorHeapAllocation(
    IDescriptorAllocator* pAllocator,
    ID3D12DescriptorHeap* pHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle,
    uint32_t                    NHandles,
    uint16_t                    AllocationManagerId
) noexcept :
    m_FirstCpuHandle{ CpuHandle },
    m_FirstGpuHandle{ GpuHandle },
    m_pAllocator{ pAllocator },
    m_NumHandles{ NHandles },
    m_pDescriptorHeap{ pHeap },
    m_AllocationManagerId{ AllocationManagerId }
{
    assert(m_pAllocator != nullptr && m_pDescriptorHeap != nullptr);
    auto DescriptorSize = m_pAllocator->GetDescriptorSize();
    assert("DescriptorSize exceeds allowed limit" && DescriptorSize < std::numeric_limits<uint16_t>::max());
    m_DescriptorSize = static_cast<uint16_t>(DescriptorSize);
}

UDX12::DescriptorHeapAllocation::DescriptorHeapAllocation(DescriptorHeapAllocation&& Allocation) noexcept :
    m_FirstCpuHandle{ std::move(Allocation.m_FirstCpuHandle) },
    m_FirstGpuHandle{ std::move(Allocation.m_FirstGpuHandle) },
    m_NumHandles{ std::move(Allocation.m_NumHandles) },
    m_pAllocator{ std::move(Allocation.m_pAllocator) },
    m_AllocationManagerId{ std::move(Allocation.m_AllocationManagerId) },
    m_pDescriptorHeap{ std::move(Allocation.m_pDescriptorHeap) },
    m_DescriptorSize{ std::move(Allocation.m_DescriptorSize) }
{
    Allocation.Reset();
}

UDX12::DescriptorHeapAllocation::~DescriptorHeapAllocation() {
    if (!IsNull() && m_pAllocator)
        m_pAllocator->Free(std::move(*this));
    // Allocation must have been disposed by the allocator
    assert("Non-null descriptor is being destroyed" && IsNull());
}

UDX12::DescriptorHeapAllocation& UDX12::DescriptorHeapAllocation::operator=(DescriptorHeapAllocation&& Allocation) noexcept {
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

void UDX12::DescriptorHeapAllocation::Reset() noexcept {
    m_FirstCpuHandle.ptr = 0;
    m_FirstGpuHandle.ptr = 0;
    m_pAllocator = nullptr;
    m_pDescriptorHeap = nullptr;
    m_NumHandles = 0;
    m_AllocationManagerId = static_cast<uint16_t>(-1);
    m_DescriptorSize = 0;
}