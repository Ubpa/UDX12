#pragma once

#include "Util.h"

namespace Ubpa::DX12::Desc {
    namespace SRC {
        D3D12_SHADER_RESOURCE_VIEW_DESC Tex2D(ID3D12Resource* pResource);
    }
}
