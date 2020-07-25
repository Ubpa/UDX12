#pragma once

#include "Util.h"

namespace Ubpa::UDX12::Desc {
    namespace SRV {
        D3D12_SHADER_RESOURCE_VIEW_DESC Tex2D(DXGI_FORMAT format);
        D3D12_SHADER_RESOURCE_VIEW_DESC TexCube(DXGI_FORMAT format);
    }

    namespace DSV {
        D3D12_DEPTH_STENCIL_VIEW_DESC Basic(DXGI_FORMAT format);
    }

    namespace PSO {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC MRT(
            ID3D12RootSignature* rootSig,
            D3D12_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements,
            ID3DBlob* VS,
            ID3DBlob* PS,
            UINT rtNum,
            DXGI_FORMAT rtvFormat,
            DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC Basic(
            ID3D12RootSignature* rootSig,
            D3D12_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements,
            ID3DBlob* VS,
            ID3DBlob* PS,
            DXGI_FORMAT rtvFormat,
            DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);
    }

    namespace RSRC {
        D3D12_RESOURCE_DESC Basic(
            D3D12_RESOURCE_DIMENSION dimension,
            UINT64 Width,
            UINT Height,
            DXGI_FORMAT format,
            D3D12_RESOURCE_FLAGS flags);

        D3D12_RESOURCE_DESC RT2D(
            UINT64 Width,
            UINT Height,
            DXGI_FORMAT format);
    }
}
