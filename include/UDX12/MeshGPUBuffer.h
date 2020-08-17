#pragma once

#include "Util.h"

#include "_deps/DirectXTK12/ResourceUploadBatch.h"

namespace Ubpa::UDX12 {
	struct MeshGPUBuffer {
		ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
		BYTE* VertexBufferMappedData{ nullptr }; // only useful for dynamic
		BYTE* IndexBufferMappedData{ nullptr }; // only useful for dynamic

		// Data about the buffers.
		UINT VertexByteStride = 0; // per vertex data size in bytes
		UINT VertexBufferByteSize = 0; // vertex buffer total size in bytes
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT
		UINT IndexBufferByteSize = 0; // index buffer total size in bytes

		// static
		// use DirectX::CreateStaticBuffer
		// idxFormat   : DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT
		// after state : D3D12_RESOURCE_STATE_GENERIC_READ
		void InitBuffer(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format);

		// dynamic
		// upload + map
		void InitBuffer(ID3D12Device* device,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format);

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
	};
}
