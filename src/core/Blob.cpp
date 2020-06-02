#include <UDX12/Blob.h>

using namespace Ubpa;

void DX12::Blob::Create(SIZE_T size) {
    ThrowIfFailed(D3DCreateBlob(size, &raw));
}

void DX12::Blob::Copy(const void* data, SIZE_T size) {
    CopyMemory(raw->GetBufferPointer(), data, size);
}

void DX12::Blob::Create(const void* data, SIZE_T size) {
    Create(size);
    Copy(data, size);
}
