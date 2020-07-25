#include <UDX12/UploadBuffer.h>

using namespace Ubpa;

UDX12::UploadBuffer::UploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag) {
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size, flag),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource)));

    ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
}

void UDX12::UploadBuffer::Set(UINT64 offset, const void* data, UINT64 size) {
    memcpy(mappedData + offset, data, size);
}
