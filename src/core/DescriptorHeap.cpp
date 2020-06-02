#include <UDX12/DescriptorHeap.h>

using namespace Ubpa;

HRESULT DX12::DescriptorHeap::Create(
    ID3D12Device* pDevice,
    D3D12_DESCRIPTOR_HEAP_TYPE Type,
    UINT NumDescriptors,
    bool bShaderVisible)
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type = Type;
    Desc.NumDescriptors = NumDescriptors;
    Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    Desc.NodeMask = 0;

    HRESULT hr = pDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(raw.GetAddressOf()));
    if (FAILED(hr)) return hr;

    hCPUHeapStart = raw->GetCPUDescriptorHandleForHeapStart();
    hGPUHeapStart = raw->GetGPUDescriptorHandleForHeapStart();

    HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
    return hr;
}
