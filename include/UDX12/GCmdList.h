#pragma once

#include "Util.h"

namespace Ubpa::UDX12 {
    // raw : Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
    // .   : simple API
    // ->  : raw API
	struct GCmdList : Util::ComPtrHolder<ID3D12GraphicsCommandList> {
		using Util::ComPtrHolder<ID3D12GraphicsCommandList>::ComPtrHolder;

        void Reset(ID3D12CommandAllocator* pAllocator);

        void ResourceBarrierTransition(
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES from,
            D3D12_RESOURCE_STATES to
        );

        // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        template<typename... Heaps,
            typename = std::enable_if_t<
            sizeof...(Heaps) < static_cast<SIZE_T>(2) &&
            (std::is_same_v<Heaps, ID3D12DescriptorHeap>&&...) >>
        void SetDescriptorHeaps(Heaps*... heaps);

        void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT color[4]);
        void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView);

        void RSSetViewport(D3D12_VIEWPORT viewport);
        void RSSetScissorRect(D3D12_RECT rect);

        // simple version
        // pro: OMSetRenderTargets
        void OMSetRenderTarget(
            D3D12_CPU_DESCRIPTOR_HANDLE rendertarget,
            D3D12_CPU_DESCRIPTOR_HANDLE depthstencil);

        // one instance
        void DrawIndexed(
            UINT IndexCount,
            UINT StartIndexLocation,
            INT BaseVertexLocation);
    };
}

#include "details/GCmdList.inl"
