#pragma once

#include "Util.h"

#include "_deps/DirectXTK12/ResourceUploadBatch.h"

#include "UploadBuffer.h"

namespace Ubpa::UDX12 {
	// static / dynamic mesh GPU buffer
	class MeshGPUBuffer {
	public:
		// [[ static ]]
		// default buffer
		// ib_format   : DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT
		// after state : D3D12_RESOURCE_STATE_GENERIC_READ
		// [sync]
		// - construct upload, default buffer
		// - cpu buffer -> upload buffer
		// [async]
		// - upload buffer -> default buffer
		// - delete upload buffer
		MeshGPUBuffer(
			ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
		);
		
		// [[ dynamic ]]
		// upload buffer + default buffer
		MeshGPUBuffer(
			ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
		);

		bool IsStatic() const noexcept { return isStatic; }

		// view of default buffer
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

		// view of default buffer
		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

		// [[ Update Strategy ]]
		// 
		// # [FOUR] cases to update a [dynamic] mesh
		//   1. non-editable + non-dirty : ConvertToStatic
		//   2. non-editable +     dirty : Update + ConvertToStatic
		//   3.     editable + non-dirty : do nothing
		//   4.     editable +     dirty : Update
		// 
		// # [FOUR] cases to update a [static] mesh
		//   1. non-editable + non-dirty : do nothing
		//   2. non-editable +     dirty : ConvertToDynamic + Update + ConvertToStatic
		//   3.     editable + non-dirty : ConvertToDynamic
		//   4.     editable +     dirty : ConvertToDynamic + Update

		// update dynamic mesh data
		// [sync]
		// - (maybe) construct resized upload buffer
		// - (maybe) construct resized default buffer
		// - cpu buffer -> upload buffer
		// [async]
		// - upload buffer -> default buffer
		void Update(
			ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format
		);

		// set static, delete upload buffer
		// [async]
		// - delete upload buffer
		void ConvertToStatic(ResourceDeleteBatch& deleteBatch);

		// convert static to dynamic
		// [sync]
		// - construct upload buffer
		void ConvertToDynamic(ID3D12Device* device);

		// delete all buffers
		// [async]
		// - delete vertexUploadBuffer, indexUploadBuffer, staticVertexBuffer, staticIndexBuffer
		void Delete(ResourceDeleteBatch& deleteBatch);

	private:
		bool isStatic{ true };

		ComPtr<ID3D12Resource> staticVertexBuffer;
		ComPtr<ID3D12Resource> staticIndexBuffer;

		std::unique_ptr<DynamicUploadBuffer> vertexUploadBuffer;
		std::unique_ptr<DynamicUploadBuffer> indexUploadBuffer;

		UINT vertexByteStride; // per vertex data size in bytes
		UINT vertexBufferByteSize; // vertex buffer total size in bytes

		DXGI_FORMAT indexFormat; // DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT
		UINT indexBufferByteSize; // index buffer total size in bytes
	};
}
