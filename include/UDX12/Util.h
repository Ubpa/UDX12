#pragma once

#include "_deps/d3dx12.h"

#include <d3dcompiler.h>
#include <wrl.h>

#include <string>

#include <cassert>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = Ubpa::DX12::AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw Ubpa::DX12::Exception(hr__, L#x, wfn, __LINE__); } \
}
#endif

namespace Ubpa::DX12 {
    using namespace Microsoft::WRL;

    std::wstring AnsiToWString(const std::string& str);

    template<typename T>
    struct ComPtrHolder {
        Microsoft::WRL::ComPtr<T> raw;
        T* operator->() noexcept { return raw.Get(); }
        const T* operator->() const noexcept { return raw.Get(); }
        bool IsNull() const noexcept { return raw.Get() == nullptr; }
    };

    class Exception {
    public:
        Exception() = default;
        Exception(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

        std::wstring ToString() const;

        HRESULT ErrorCode = S_OK;
        std::wstring FunctionName;
        std::wstring Filename;
        int LineNumber = -1;
    };
}
