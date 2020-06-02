#pragma once

namespace Ubpa::DX12 {
    template<typename... Heaps, typename>
    void GCmdList::SetDescriptorHeaps(Heaps*... heaps){
        const std::array<ID3D12DescriptorHeap*, sizeof...(Heaps)> arr{ heaps... };

#ifndef NDEBUG
        for (SIZE_T i = 0; i < arr.size(); i++) {
            ID3D12DescriptorHeap* heapi = arr[i];
            auto typei = heapi->GetDesc().Type;
            for (SIZE_T j = 1; j < arr.size(); j++) {
                ID3D12DescriptorHeap* heapj = arr[j];
                auto typej = heapj->GetDesc().Type;
                assert(typei != typej);
            }
        }
#endif // !NDEBUG

        raw->SetDescriptorHeaps(static_cast<UINT>(arr.size()), arr.data());
    }
}
