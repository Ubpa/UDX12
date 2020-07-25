#include <UDX12/DescriptorHeap/DynamicSuballocMngr.h>

using namespace Ubpa;

UDX12::DynamicSuballocMngr::DynamicSuballocMngr(
    GPUDescriptorHeap& ParentGPUHeap,
    uint32_t           DynamicChunkSize,
    std::string        ManagerName)
    :
    // clang-format off
    m_ParentGPUHeap{ ParentGPUHeap },
    m_DynamicChunkSize{ DynamicChunkSize },
    m_ManagerName{ std::move(ManagerName) }
    // clang-format on
{
}

UDX12::DynamicSuballocMngr::~DynamicSuballocMngr()
{
    assert(m_Suballocations.empty() && m_CurrDescriptorCount == 0 && m_CurrSuballocationsTotalSize == 0 && "All dynamic suballocations must be released!");
}

void UDX12::DynamicSuballocMngr::ReleaseAllocations()
{
    // Clear the list and dispose all allocated chunks of GPU descriptor heap.
    // The chunks will be added to release queues and eventually returned to the
    // parent GPU heap.
    for (auto& Allocation : m_Suballocations)
        m_ParentGPUHeap.Free(std::move(Allocation));
    m_Suballocations.clear();
    m_CurrDescriptorCount = 0;
    m_CurrSuballocationsTotalSize = 0;
}

UDX12::DescriptorHeapAllocation UDX12::DynamicSuballocMngr::Allocate(uint32_t Count)
{
    // This method is intentionally lock-free as it is expected to
    // be called through device context from single thread only

    // Check if there are no chunks or the last chunk does not have enough space
    if (m_Suballocations.empty() ||
        m_CurrentSuballocationOffset + Count > m_Suballocations.back().GetNumHandles())
    {
        // Request a new chunk from the parent GPU descriptor heap
        auto SuballocationSize = std::max(m_DynamicChunkSize, Count);
        auto NewDynamicSubAllocation = m_ParentGPUHeap.AllocateDynamic(SuballocationSize);
        if (NewDynamicSubAllocation.IsNull())
            return DescriptorHeapAllocation();
        m_Suballocations.emplace_back(std::move(NewDynamicSubAllocation));
        m_CurrentSuballocationOffset = 0;

        m_CurrSuballocationsTotalSize += SuballocationSize;
        m_PeakSuballocationsTotalSize = std::max(m_PeakSuballocationsTotalSize, m_CurrSuballocationsTotalSize);
    }

    // Perform suballocation from the last chunk
    auto& CurrentSuballocation = m_Suballocations.back();

    auto ManagerId = CurrentSuballocation.GetAllocationManagerId();
    assert(ManagerId < std::numeric_limits<uint16_t>::max() && "ManagerID exceed allowed limit");
    DescriptorHeapAllocation Allocation(*this,
        CurrentSuballocation.GetDescriptorHeap(),
        CurrentSuballocation.GetCpuHandle(m_CurrentSuballocationOffset),
        CurrentSuballocation.GetGpuHandle(m_CurrentSuballocationOffset),
        Count,
        static_cast<uint16_t>(ManagerId));
    m_CurrentSuballocationOffset += Count;
    m_CurrDescriptorCount += Count;
    m_PeakDescriptorCount = std::max(m_PeakDescriptorCount, m_CurrDescriptorCount);

    return Allocation;
}