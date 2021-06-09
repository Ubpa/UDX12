#pragma once

#include "_deps/d3dx12.h"
#include "_deps/DirectXTK12/ResourceUploadBatch.h"

#include <d3dcompiler.h>
#include <wrl.h>
#include <atlcomcli.h>

#include <string>

#include <cassert>

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                               \
{                                                                                      \
    HRESULT hr__ = (x);                                                                \
    std::wstring wfn = Ubpa::UDX12::Util::AnsiToWString(__FILE__);                     \
    if(FAILED(hr__)) { throw Ubpa::UDX12::Util::Exception(hr__, L#x, wfn, __LINE__); } \
}
#endif

namespace Ubpa::UDX12 {
    using Microsoft::WRL::ComPtr;
    using ATL::CComPtr;
    class D3DInclude;
}

namespace Ubpa::UDX12::Util {
    template<typename T>
    void ReleaseCom(T* & p) {
        if (p) {
            p->Release();
            p = nullptr;
        }
    }

    std::wstring AnsiToWString(const std::string& str);

    template<typename T>
    struct ComPtrHolder {
        ComPtrHolder(ComPtr<T> ptr = {}) : raw{ ptr } {}
        ComPtr<T> raw;
        T* operator->() noexcept { return raw.Get(); }
        const T* operator->() const noexcept { return raw.Get(); }
        bool IsNull() const noexcept { return raw.Get() == nullptr; }
        T* Get() const noexcept { return raw.Get(); }
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

    constexpr UINT CalcConstantBufferByteSize(UINT byteSize)
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
    ComPtr<ID3DBlob> CompileShaderFromFile(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target
    );

	ComPtr<ID3DBlob> CompileShader(
		std::string_view source,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target,
        D3DInclude* pInclude,
        LPCSTR pSourceName = nullptr
	);

	HRESULT __cdecl CreateTexture2DArrayFromMemory(_In_ ID3D12Device* device,
		DirectX::ResourceUploadBatch& resourceUpload,
		size_t width, size_t height, size_t arraySize,
		DXGI_FORMAT format,
		const D3D12_SUBRESOURCE_DATA* subResources,
		_COM_Outptr_ ID3D12Resource** texture,
		bool generateMips = false,
		D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE) noexcept;

    ComPtr<ID3DBlob> CompileLibrary(LPCVOID pText, UINT32 size, LPCWSTR pSourceName);
}
