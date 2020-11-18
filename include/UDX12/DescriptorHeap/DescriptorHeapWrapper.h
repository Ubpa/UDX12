#pragma once

#include "../Util.h"

namespace Ubpa::UDX12 {
    // ref: https://docs.microsoft.com/en-us/windows/win32/direct3d12/creating-descriptor-heaps
    class DescriptorHeapWrapper : Util::ComPtrHolder<ID3D12DescriptorHeap> {
    public:
        DescriptorHeapWrapper() { memset(this, 0, sizeof(DescriptorHeapWrapper)); }

        HRESULT Create(
            ID3D12Device* pDevice,
            const D3D12_DESCRIPTOR_HEAP_DESC* desc);

        HRESULT Create(
            ID3D12Device* pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE Type,
            UINT NumDescriptors,
            bool bShaderVisible = false);

        void Clear() {
            raw.Reset();
            memset(this, 0, sizeof(DescriptorHeapWrapper));
        }

        UINT Size() const noexcept {
            if (IsNull())
                return 0;

            return Desc.NumDescriptors;
        }

        bool Empty() const noexcept { return Size() == 0; }

        operator ID3D12DescriptorHeap* () const noexcept { return raw.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index) const noexcept
        { return { hCPUHeapStart.ptr + index * HandleIncrementSize }; }

        D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index) const noexcept {
            assert(Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            return { hGPUHeapStart.ptr + index * HandleIncrementSize };
        }

    private:
        D3D12_DESCRIPTOR_HEAP_DESC Desc;
        D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
        D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
        UINT HandleIncrementSize;
    };
}
