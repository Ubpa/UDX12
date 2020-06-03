#include <UDX12/MeshGeometry.h>

#include <UDX12/_deps/DirectXTK12/BufferHelpers.h>

using namespace Ubpa;

void DX12::MeshGeometry::InitBuffer(
	ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload,
	const void* vb_data, UINT vb_count, UINT vb_stride, bool vb_static,
	const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format, bool ib_static
) {
	assert(ib_format == DXGI_FORMAT_R16_UINT || ib_format == DXGI_FORMAT_R32_UINT);

	UINT ib_stride = ib_format == DXGI_FORMAT_R16_UINT ? 2 : 4;

	UINT vb_size = vb_count * vb_stride;
	UINT ib_size = ib_count * ib_stride;

	VertexBufferCPU.Create(vb_data, vb_size);
	IndexBufferCPU.Create(vb_data, ib_size);

	DirectX::CreateStaticBuffer(device, resourceUpload, vb_data, vb_count, vb_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		VertexBufferGPU.GetAddressOf());

	DirectX::CreateStaticBuffer(device, resourceUpload, ib_data, ib_count, ib_stride, D3D12_RESOURCE_STATE_GENERIC_READ,
		IndexBufferGPU.GetAddressOf());

	VertexByteStride = vb_stride;
	VertexBufferByteSize = vb_size;
	IndexFormat = ib_format;
	IndexBufferByteSize = ib_size;
}
