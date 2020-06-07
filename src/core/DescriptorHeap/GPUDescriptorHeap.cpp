#include <UDX12/DescriptorHeap/GPUDescriptorHeap.h>

using namespace Ubpa;

DX12::GPUDescriptorHeap::GPUDescriptorHeap(
    ID3D12Device*               Device,
    uint32_t                    NumDescriptorsInHeap,
    uint32_t                    NumDynamicDescriptors,
    D3D12_DESCRIPTOR_HEAP_TYPE  Type,
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
    :
    // clang-format off
    m_Device{Device},
    m_HeapDesc
    {
        Type,
        NumDescriptorsInHeap + NumDynamicDescriptors,
        Flags,
        1 // UINT NodeMask;
    },
    m_pd3d12DescriptorHeap
    {
        [&]
        {
              CComPtr<ID3D12DescriptorHeap> pHeap;
              Device->CreateDescriptorHeap(&m_HeapDesc, __uuidof(pHeap), reinterpret_cast<void**>(&pHeap));
              return pHeap;
        }()
    },
    m_DescriptorSize           {Device->GetDescriptorHandleIncrementSize(Type)},
    m_HeapAllocationManager    {Device, *this, 0, m_pd3d12DescriptorHeap, 0, NumDescriptorsInHeap},
    m_DynamicAllocationsManager{Device, *this, 1, m_pd3d12DescriptorHeap, NumDescriptorsInHeap, NumDynamicDescriptors}
// clang-format on
{
}

DX12::GPUDescriptorHeap::~GPUDescriptorHeap()
{
    auto TotalStaticSize  = m_HeapAllocationManager.GetMaxDescriptors();
    auto TotalDynamicSize = m_DynamicAllocationsManager.GetMaxDescriptors();
    auto MaxStaticSize    = m_HeapAllocationManager.GetMaxAllocatedSize();
    auto MaxDynamicSize   = m_DynamicAllocationsManager.GetMaxAllocatedSize();
}

void DX12::GPUDescriptorHeap::Free(DescriptorHeapAllocation&& Allocation)
{
    auto MgrId = Allocation.GetAllocationManagerId();
    assert(MgrId == 0 || MgrId == 1 && "Unexpected allocation manager ID");

    if (MgrId == 0)
    {
        m_HeapAllocationManager.FreeAllocation(std::move(Allocation));
    }
    else
    {
        m_DynamicAllocationsManager.FreeAllocation(std::move(Allocation));
    }
}
