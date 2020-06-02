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
