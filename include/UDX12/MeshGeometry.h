#pragma once

#include "Util.h"

#include "Blob.h"

//#include <DirectXCollision.h>

#include "_deps/DirectXTK12/ResourceUploadBatch.h"

#include <unordered_map>

namespace Ubpa::UDX12 {
	// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
	// geometries are stored in one vertex and index buffer.  It provides the offsets
	// and data needed to draw a subset of geometry stores in the vertex and index 
	// buffers so that we can implement the technique described by Figure 6.3.
	struct SubmeshGeometry
	{
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		INT BaseVertexLocation = 0;

		// Bounding box of the geometry defined by this submesh. 
		// This is used in later chapters of the book.
		//DirectX::BoundingBox Bounds;
	};

	struct MeshGeometry
	{
		// Give it a name so we can look it up by name.
		std::string Name;

		// System memory copies.  Use Blobs because the vertex/index format can be generic.
		// It is up to the client to cast appropriately.  
		Blob VertexBufferCPU{ nullptr };
		Blob IndexBufferCPU{ nullptr };

		ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
		BYTE* VertexBufferMappedData{ nullptr }; // only useful for dynamic
		BYTE* IndexBufferMappedData{ nullptr }; // only useful for dynamic

		// Data about the buffers.
		UINT VertexByteStride = 0; // per vertex data size in bytes
		UINT VertexBufferByteSize = 0; // vertex buffer total size in bytes
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT; // DXGI_FORMAT_R16_UINT / DXGI_FORMAT_R32_UINT
		UINT IndexBufferByteSize = 0; // index buffer total size in bytes

		// A MeshGeometry may store multiple geometries in one vertex/index buffer.
		// Use this container to define the Submesh geometries so we can draw
		// the Submeshes individually.
		std::unordered_map<std::string, SubmeshGeometry> submeshGeometries;

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

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
			vbv.StrideInBytes = VertexByteStride;
			vbv.SizeInBytes = VertexBufferByteSize;

			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
		{
			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			ibv.Format = IndexFormat;
			ibv.SizeInBytes = IndexBufferByteSize;

			return ibv;
		}
	};
}
