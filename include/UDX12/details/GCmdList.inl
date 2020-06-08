#pragma once

namespace Ubpa::DX12 {
    template<typename... Heaps, typename>
    void GCmdList::SetDescriptorHeaps(Heaps*... heaps){
        constexpr size_t N = sizeof...(Heaps);
        ID3D12DescriptorHeap* arr[N] = { heaps... };

#ifndef NDEBUG
        for (SIZE_T i = 0; i < N; i++) {
            ID3D12DescriptorHeap* heapi = arr[i];
            auto typei = heapi->GetDesc().Type;
            assert(typei == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || typei == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            for (SIZE_T j = 1; j < N; j++) {
                ID3D12DescriptorHeap* heapj = arr[j];
                auto typej = heapj->GetDesc().Type;
                assert(typei != typej);
            }
        }
#endif // !NDEBUG

        raw->SetDescriptorHeaps(N, arr);
    }
}
