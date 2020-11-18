#include <UDX12/DescriptorHeap/CPUDescriptorHeap.h>

#include <UDX12/DescriptorHeap/DescriptorHeapAllocation.h>

using namespace Ubpa;

UDX12::CPUDescriptorHeap::CPUDescriptorHeap(
    ID3D12Device*               pDevice,
    uint32_t                    NumDescriptorsInHeap,
    D3D12_DESCRIPTOR_HEAP_TYPE  Type,
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
    :
    m_pDevice{ pDevice },
    m_HeapDesc
    {
        Type,
        NumDescriptorsInHeap,
        Flags,
        1   // NodeMask
    },
    m_DescriptorSize{ pDevice->GetDescriptorHandleIncrementSize(Type) }
{
    // Create one pool
    m_HeapPool.emplace_back(m_pDevice, *this, 0, m_HeapDesc);
    m_AvailableHeaps.insert(0);
}

UDX12::CPUDescriptorHeap::~CPUDescriptorHeap() {
    assert(m_CurrentSize == 0 && "Not all allocations released");

    assert(m_AvailableHeaps.size() == m_HeapPool.size() && "Not all descriptor heap pools are released");

#ifndef NDEBUG
    for (const auto& Heap : m_HeapPool)
        assert(Heap.GetNumAvailableDescriptors() == Heap.GetMaxDescriptors() && "Not all descriptors in the descriptor pool are released");
#endif // !NDEBUG
}

UDX12::DescriptorHeapAllocation UDX12::CPUDescriptorHeap::Allocate(uint32_t count) {
    std::lock_guard<std::mutex> lockGuard(m_HeapPoolMutex);
    // Note that every DescriptorHeapAllocationManager object instance is itslef
    // thread-safe. Nested mutexes cannot cause a deadlock

    DescriptorHeapAllocation allocation;
    // Go through all descriptor heap managers that have free descriptors
    auto availableHeapIter = m_AvailableHeaps.begin();
    while (availableHeapIter != m_AvailableHeaps.end()) {
        auto nextIter = availableHeapIter;
        ++nextIter;
        // Try to allocate descriptor using the current descriptor heap manager
        allocation = m_HeapPool[*availableHeapIter].Allocate(count);
        // Remove the manager from the pool if it has no more available descriptors
        if (m_HeapPool[*availableHeapIter].GetNumAvailableDescriptors() == 0)
            m_AvailableHeaps.erase(*availableHeapIter);

        // Terminate the loop if descriptor was successfully allocated, otherwise
        // go to the next manager
        if (!allocation.IsNull())
            break;
        availableHeapIter = nextIter;
    }

    // If there were no available descriptor heap managers or no manager was able
    // to suffice the allocation request, create a new manager
    if (allocation.IsNull()) {
        // Make sure the heap is large enough to accomodate the requested number of descriptors
        m_HeapDesc.NumDescriptors = std::max(m_HeapDesc.NumDescriptors, static_cast<UINT>(count));
        // Create a new descriptor heap manager. Note that this constructor creates a new D3D12 descriptor
        // heap and references the entire heap. Pool index is used as manager ID
        m_HeapPool.emplace_back(m_pDevice, *this, m_HeapPool.size(), m_HeapDesc);
        auto newHeapIter = m_AvailableHeaps.insert(m_HeapPool.size() - 1);
        assert(newHeapIter.second);

        // Use the new manager to allocate descriptor handles
        allocation = m_HeapPool[*newHeapIter.first].Allocate(count);
    }

    m_CurrentSize += static_cast<uint32_t>(allocation.GetNumHandles());
    m_MaxSize = std::max(m_MaxSize, m_CurrentSize);

    return allocation;
}

void UDX12::CPUDescriptorHeap::Free(DescriptorHeapAllocation&& allocation) {
    std::lock_guard<std::mutex> lockGuard(m_HeapPoolMutex);
    auto                        managerId = allocation.GetAllocationManagerId();
    m_CurrentSize -= static_cast<uint32_t>(allocation.GetNumHandles());
    m_HeapPool[managerId].FreeAllocation(std::move(allocation));
    // Return the manager to the pool of available managers
    assert(m_HeapPool[managerId].GetNumAvailableDescriptors() > 0);
    m_AvailableHeaps.insert(managerId);
}
