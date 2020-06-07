#include <UDX12/DescriptorHeap/CPUDescriptorHeap.h>

#include <UDX12/DescriptorHeap/DescriptorHeapAllocation.h>

using namespace Ubpa;

//
// CPUDescriptorHeap implementation
//
DX12::CPUDescriptorHeap::CPUDescriptorHeap(
    ID3D12Device*               pDevice,
    uint32_t                    NumDescriptorsInHeap,
    D3D12_DESCRIPTOR_HEAP_TYPE  Type,
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
    :
    // clang-format off
    m_pDevice{ pDevice },
    m_HeapDesc
{
    Type,
    NumDescriptorsInHeap,
    Flags,
    1   // NodeMask
},
m_DescriptorSize{ pDevice->GetDescriptorHandleIncrementSize(Type) }
// clang-format on
{
    // Create one pool
    m_HeapPool.emplace_back(m_pDevice, *this, 0, m_HeapDesc);
    m_AvailableHeaps.insert(0);
}

DX12::CPUDescriptorHeap::~CPUDescriptorHeap()
{
    assert(m_CurrentSize == 0 && "Not all allocations released");

    assert(m_AvailableHeaps.size() == m_HeapPool.size() && "Not all descriptor heap pools are released");
    uint32_t TotalDescriptors = 0;
    for (auto& Heap : m_HeapPool)
    {
        assert(Heap.GetNumAvailableDescriptors() == Heap.GetMaxDescriptors() && "Not all descriptors in the descriptor pool are released");
        TotalDescriptors += Heap.GetMaxDescriptors();
    }
}

DX12::DescriptorHeapAllocation DX12::CPUDescriptorHeap::Allocate(uint32_t Count)
{
    std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
    // Note that every DescriptorHeapAllocationManager object instance is itslef
    // thread-safe. Nested mutexes cannot cause a deadlock

    DescriptorHeapAllocation Allocation;
    // Go through all descriptor heap managers that have free descriptors
    auto AvailableHeapIt = m_AvailableHeaps.begin();
    while (AvailableHeapIt != m_AvailableHeaps.end())
    {
        auto NextIt = AvailableHeapIt;
        ++NextIt;
        // Try to allocate descriptor using the current descriptor heap manager
        Allocation = m_HeapPool[*AvailableHeapIt].Allocate(Count);
        // Remove the manager from the pool if it has no more available descriptors
        if (m_HeapPool[*AvailableHeapIt].GetNumAvailableDescriptors() == 0)
            m_AvailableHeaps.erase(*AvailableHeapIt);

        // Terminate the loop if descriptor was successfully allocated, otherwise
        // go to the next manager
        if (!Allocation.IsNull())
            break;
        AvailableHeapIt = NextIt;
    }

    // If there were no available descriptor heap managers or no manager was able
    // to suffice the allocation request, create a new manager
    if (Allocation.IsNull())
    {
        // Make sure the heap is large enough to accomodate the requested number of descriptors
        m_HeapDesc.NumDescriptors = std::max(m_HeapDesc.NumDescriptors, static_cast<UINT>(Count));
        // Create a new descriptor heap manager. Note that this constructor creates a new D3D12 descriptor
        // heap and references the entire heap. Pool index is used as manager ID
        m_HeapPool.emplace_back(m_pDevice, *this, m_HeapPool.size(), m_HeapDesc);
        auto NewHeapIt = m_AvailableHeaps.insert(m_HeapPool.size() - 1);
        assert(NewHeapIt.second);

        // Use the new manager to allocate descriptor handles
        Allocation = m_HeapPool[*NewHeapIt.first].Allocate(Count);
    }

    m_CurrentSize += static_cast<uint32_t>(Allocation.GetNumHandles());
    m_MaxSize = std::max(m_MaxSize, m_CurrentSize);

    return Allocation;
}

void DX12::CPUDescriptorHeap::Free(DescriptorHeapAllocation&& Allocation)
{
    std::lock_guard<std::mutex> LockGuard(m_HeapPoolMutex);
    auto                        ManagerId = Allocation.GetAllocationManagerId();
    m_CurrentSize -= static_cast<uint32_t>(Allocation.GetNumHandles());
    m_HeapPool[ManagerId].FreeAllocation(std::move(Allocation));
    // Return the manager to the pool of available managers
    assert(m_HeapPool[ManagerId].GetNumAvailableDescriptors() > 0);
    m_AvailableHeaps.insert(ManagerId);
}
