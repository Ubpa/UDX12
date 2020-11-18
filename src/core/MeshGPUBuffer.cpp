#include <UDX12/MeshGPUBuffer.h>

#include <UDX12/_deps/DirectXTK12/BufferHelpers.h>

using namespace Ubpa;

UDX12::MeshGPUBuffer::MeshGPUBuffer(
	ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload,
	const void* vb_data, UINT vb_count, UINT vb_stride,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
)
	:
	isStatic{ true }
{
	assert(ib_format == DXGI_FORMAT_R16_UINT || ib_format == DXGI_FORMAT_R32_UINT);

	UINT ib_stride = ib_format == DXGI_FORMAT_R16_UINT ? 2 : 4;

	UINT vb_size = vb_count * vb_stride;
	UINT ib_size = ib_count * ib_stride;

	DirectX::CreateStaticBuffer(device, resourceUpload, vb_data, vb_count, vb_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		staticVertexBuffer.GetAddressOf());

	DirectX::CreateStaticBuffer(device, resourceUpload, ib_data, ib_count, ib_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		staticIndexBuffer.GetAddressOf());

	vertexByteStride = vb_stride;
	vertexBufferByteSize = vb_size;
	indexFormat = ib_format;
	indexBufferByteSize = ib_size;
}

UDX12::MeshGPUBuffer::MeshGPUBuffer(
	ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
	const void* vb_data, UINT vb_count, UINT vb_stride,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
)
{
	ConvertToDynamic(device);
	Update(
		device, cmdList,
		vb_data, vb_count, vb_stride,
		ib_data, ib_count, ib_format
	);
}

void UDX12::MeshGPUBuffer::Update(
	ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
	const void* vb_data, UINT vb_count, UINT vb_stride,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
) {
	assert(!IsStatic());

	// 1. update upload heap buffer
	assert(ib_format == DXGI_FORMAT_R16_UINT || ib_format == DXGI_FORMAT_R32_UINT);

	UINT ib_stride = ib_format == DXGI_FORMAT_R16_UINT ? 2 : 4;

	UINT vb_size = vb_count * vb_stride;
	UINT ib_size = ib_count * ib_stride;

	vertexUploadBuffer->FastReserve(vb_size);
	indexUploadBuffer->FastReserve(ib_size);

	vertexUploadBuffer->Set(0, vb_data, vb_size);
	indexUploadBuffer->Set(0, ib_data, ib_size);

	vertexByteStride = vb_stride;
	vertexBufferByteSize = vb_size;
	indexFormat = ib_format;
	indexBufferByteSize = ib_size;

	// 2. copy upload heap buffer data to default heap buffer
	if (staticVertexBuffer && staticVertexBuffer->GetDesc().Width >= vb_size)
		vertexUploadBuffer->CopyAssign(0, 0, vb_size, cmdList, staticVertexBuffer.Get());
	else
		vertexUploadBuffer->CopyConstruct(0, 0, vb_size, device, cmdList, D3D12_RESOURCE_STATE_GENERIC_READ, &staticVertexBuffer);

	if (staticIndexBuffer && staticIndexBuffer->GetDesc().Width >= ib_size)
		indexUploadBuffer->CopyAssign(0, 0, ib_size, cmdList, staticIndexBuffer.Get());
	else
		indexUploadBuffer->CopyConstruct(0, 0, ib_size, device, cmdList, D3D12_RESOURCE_STATE_GENERIC_READ, &staticIndexBuffer);
}

void UDX12::MeshGPUBuffer::ConvertToStatic(ResourceDeleteBatch& deleteBatch) {
	assert(!IsStatic());
	vertexUploadBuffer->Delete(deleteBatch);
	indexUploadBuffer->Delete(deleteBatch);
	vertexUploadBuffer.reset();
	indexUploadBuffer.reset();
	isStatic = true;
}

void UDX12::MeshGPUBuffer::ConvertToDynamic(ID3D12Device* device) {
	assert(IsStatic());
	isStatic = false;
	vertexUploadBuffer = std::make_unique<DynamicUploadBuffer>(device);
	indexUploadBuffer = std::make_unique<DynamicUploadBuffer>(device);
}

void UDX12::MeshGPUBuffer::Delete(ResourceDeleteBatch& deleteBatch) {
	if (!IsStatic()) {
		vertexUploadBuffer->Delete(deleteBatch);
		indexUploadBuffer->Delete(deleteBatch);
		vertexUploadBuffer.reset();
		indexUploadBuffer.reset();
	}

	deleteBatch.Add(staticVertexBuffer.Get());
	deleteBatch.Add(staticIndexBuffer.Get());
	staticVertexBuffer.Detach();
	staticIndexBuffer.Detach();
}

D3D12_VERTEX_BUFFER_VIEW UDX12::MeshGPUBuffer::VertexBufferView() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = staticVertexBuffer->GetGPUVirtualAddress();
	vbv.StrideInBytes = vertexByteStride;
	vbv.SizeInBytes = vertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW UDX12::MeshGPUBuffer::IndexBufferView() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = staticIndexBuffer->GetGPUVirtualAddress();
	ibv.Format = indexFormat;
	ibv.SizeInBytes = indexBufferByteSize;

	return ibv;
}
