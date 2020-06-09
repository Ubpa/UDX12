#include <UDX12/Desc.h>

using namespace Ubpa;

D3D12_SHADER_RESOURCE_VIEW_DESC DX12::Desc::SRC::Tex2D(ID3D12Resource* pResource) {
    auto rsrcDesc = pResource->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = rsrcDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = rsrcDesc.MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    return srvDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC DX12::Desc::PSO::Basic(
    ID3D12RootSignature* rootSig,
    D3D12_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements,
    ID3DBlob* VS,
    ID3DBlob* PS,
    DXGI_FORMAT rtvFormat,
    DXGI_FORMAT dsvFormat)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;

	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { pInputElementDescs, NumElements };
	psoDesc.pRootSignature = rootSig;
	psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = rtvFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	return psoDesc;
}