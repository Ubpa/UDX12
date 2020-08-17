#include <UDX12/MeshGPUBuffer.h>

#include <UDX12/_deps/DirectXTK12/BufferHelpers.h>

using namespace Ubpa;

void UDX12::MeshGPUBuffer::InitBuffer(
	ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload,
	const void* vb_data, UINT vb_count, UINT vb_stride,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format)
{
	assert(ib_format == DXGI_FORMAT_R16_UINT || ib_format == DXGI_FORMAT_R32_UINT);

	UINT ib_stride = ib_format == DXGI_FORMAT_R16_UINT ? 2 : 4;

	UINT vb_size = vb_count * vb_stride;
	UINT ib_size = ib_count * ib_stride;

	DirectX::CreateStaticBuffer(device, resourceUpload, vb_data, vb_count, vb_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		VertexBufferGPU.GetAddressOf());

	DirectX::CreateStaticBuffer(device, resourceUpload, ib_data, ib_count, ib_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		IndexBufferGPU.GetAddressOf());

	VertexByteStride = vb_stride;
	VertexBufferByteSize = vb_size;
	IndexFormat = ib_format;
	IndexBufferByteSize = ib_size;
}

void UDX12::MeshGPUBuffer::InitBuffer(ID3D12Device* device,
	const void* vb_data, UINT vb_count, UINT vb_stride,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format)
{
	assert(ib_format == DXGI_FORMAT_R16_UINT || ib_format == DXGI_FORMAT_R32_UINT);

	UINT ib_stride = ib_format == DXGI_FORMAT_R16_UINT ? 2 : 4;

	UINT vb_size = vb_count * vb_stride;
	UINT ib_size = ib_count * ib_stride;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vb_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&VertexBufferGPU)));

	ThrowIfFailed(VertexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&VertexBufferMappedData)));
	memcpy(VertexBufferMappedData, vb_data, vb_size);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ib_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&IndexBufferGPU)));

	ThrowIfFailed(IndexBufferGPU->Map(0, nullptr, reinterpret_cast<void**>(&IndexBufferMappedData)));
	memcpy(IndexBufferMappedData, ib_data, ib_size);

	VertexByteStride = vb_stride;
	VertexBufferByteSize = vb_size;
	IndexFormat = ib_format;
	IndexBufferByteSize = ib_size;
}

D3D12_VERTEX_BUFFER_VIEW UDX12::MeshGPUBuffer::VertexBufferView() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW UDX12::MeshGPUBuffer::IndexBufferView() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;

	return ibv;
}
