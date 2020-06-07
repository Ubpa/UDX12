#pragma once

#define NOMINMAX

#include "_deps/d3dx12.h"

#include <d3dcompiler.h>
#include <wrl.h>
#include <atlcomcli.h>

#include <string>

#include <cassert>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                              \
{                                                                                     \
    HRESULT hr__ = (x);                                                               \
    std::wstring wfn = Ubpa::DX12::Util::AnsiToWString(__FILE__);                     \
    if(FAILED(hr__)) { throw Ubpa::DX12::Util::Exception(hr__, L#x, wfn, __LINE__); } \
}
#endif

namespace Ubpa::DX12 {
    using Microsoft::WRL::ComPtr;
    using ATL::CComPtr;
}

namespace Ubpa::DX12::Util {

    std::wstring AnsiToWString(const std::string& str);

    template<typename T>
    struct ComPtrHolder {
        ComPtr<T> raw;
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

    // vkeyCode : virtual key code
    // ref: https://docs.microsoft.com/zh-cn/windows/win32/inputdev/virtual-key-codes 
    bool IsKeyDown(int vkeyCode);

    // 32bit in hex, start with '0x'
    // e.g. 0x88888888
    std::string HRstToString(HRESULT hr);

    inline UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    // release uploadBuffer after coping
    ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        ComPtr<ID3D12Resource>& uploadBuffer);

    // [summary]
    // compile shader file to bytecode
    // [arguments]
    // - defines: marco array, end with {NULL, NULL}
    // - - e.g. #define zero 0 <-> D3D_SHADER_MACRO Shader_Macros[] = { "zero", "0", NULL, NULL };
    // - entrypoint: begin function name, like 'main'
    // - target: e.g. cs/ds/gs/hs/ps/vs + _5_ + 0/1
    // [ref] https://docs.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompilefromfile
    ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target);
}
