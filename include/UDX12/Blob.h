#pragma once 

#include "Util.h"

namespace Ubpa::DX12 {
    // CPU buffer
    // raw : Microsoft::WRL::ComPtr<ID3DBlob>
    // .   : simple API
    // ->  : raw API
    struct Blob : ComPtrHolder<ID3DBlob> {
        void Create(SIZE_T size);
        void Copy(const void* data, SIZE_T size);
        void Create(const void* data, SIZE_T size);
    };
}
