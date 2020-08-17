#pragma once 

#include "Util.h"

namespace Ubpa::UDX12 {
    // raw : Microsoft::WRL::ComPtr<ID3D12Device>
    // .   : simple API
    // ->  : raw API
	struct Device : Util::ComPtrHolder<ID3D12Device> {
		using Util::ComPtrHolder<ID3D12Device>::ComPtrHolder;

        void CreateCommittedResource(
            D3D12_HEAP_TYPE heap_type,
            SIZE_T size,
            ID3D12Resource** resources);

        void CreateDescriptorHeap(UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type,
            ID3D12DescriptorHeap** pHeap);

        void CreateSRV_Tex2D(
            ID3D12Resource* pResource,
            D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);
    };
}
