#pragma once

#include "Util.h"

namespace Ubpa::DX12 {
    // ref: https://docs.microsoft.com/en-us/windows/win32/direct3d12/creating-descriptor-heaps
    struct DescriptorHeap : ComPtrHolder<ID3D12DescriptorHeap>
    {
        DescriptorHeap() { memset(this, 0, sizeof(*this)); }

        HRESULT Create(
            ID3D12Device* pDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE Type,
            UINT NumDescriptors,
            bool bShaderVisible = false);

        operator ID3D12DescriptorHeap* () { return raw.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index)
        {
            return { hCPUHeapStart.ptr + index * HandleIncrementSize };
        }

        D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index)
        {
            assert(Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
            return { hGPUHeapStart.ptr + index * HandleIncrementSize };
        }

        D3D12_DESCRIPTOR_HEAP_DESC Desc;
        D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
        D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
        UINT HandleIncrementSize;
    };
}
