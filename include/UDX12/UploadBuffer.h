#pragma once

#include "Util.h"

#include "ResourceDeleteBatch.h"
#include "_deps/DirectXTK12/ResourceUploadBatch.h"

#include <memory>

namespace Ubpa::UDX12 {
	// resource in upload heap
	// upload heap : cpu and gpu shared memory
	// sync : run in cpu timeline
	// async : record in command list, run in gpu timeline
	class UploadBuffer {
	public:
		// create default buffer
		// [sync]
		// - construct
		UploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
		~UploadBuffer();

		ID3D12Resource* GetResource() const noexcept { return resource.Get(); }
		UINT64 Size() const noexcept { return size; }
		BYTE* GetMappedData() const noexcept { return mappedData; }
		bool Valid() const noexcept { return resource.Get() != nullptr; }

		// copy cpu buffer to upload buffer
		// [sync]
		// - cpu buffer -> upload buffer
		void Set(UINT64 offset, const void* data, UINT64 size);
		template<typename T>
		void Set(UINT64 offset, const T* data) { Set(offset, data, sizeof(T)); }

		// create default buffer resource
		// [sync]
		// - construct default buffer
		// [async]
		// - upload buffer -> default buffer
		void CopyConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		// copy upload buffer to dst
		// [async]
		// - upload buffer -> dst
		void CopyAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);

		// move resource to deleteBatch
		void Delete(ResourceDeleteBatch& deleteBatch);
	private:
		ComPtr<ID3D12Resource> resource;
		BYTE* mappedData;
		UINT64 size;
	};

	class DynamicUploadBuffer {
	public:
		DynamicUploadBuffer(ID3D12Device* device, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
		DynamicUploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		ID3D12Resource* GetResource() const noexcept;
		UINT64 Size() const noexcept;
		bool Valid() const noexcept { return (bool)buffer; }

		// retain original data when resizing
		// [sync]
		// - (maybe) construct resized upload buffer
		// - (maybe) orignal upload buffer -> new upload buffer
		void Reserve(size_t size);

		// not retain original data when resizing
		// [sync]
		// - (maybe) construct resized upload buffer
		void FastReserve(size_t size);

		// copy cpu buffer to upload buffer
		// [sync]
		// - cpu buffer -> upload buffer
		void Set(UINT64 offset, const void* data, UINT64 size);
		template<typename T>
		void Set(UINT64 offset, const T* data) { Set(offset, data, sizeof(T)); }

		// same with UploadBuffer::CopyConstruct
		void CopyConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		// same with UploadBuffer::CopyAssign
		void CopyAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);

		// move resource to deleteBatch
		void Delete(ResourceDeleteBatch& deleteBatch);
	private:
		ID3D12Device* device;
		D3D12_RESOURCE_FLAGS flag;

		std::unique_ptr<UploadBuffer> buffer;
	};

	// a wrappper of the upload buffer to treat the buffer as an fix-size array
	template<typename T>
	class ArrayUploadBuffer : public UploadBuffer {
	public:
		ArrayUploadBuffer(ID3D12Device* device, UINT64 numElement, bool isConstantBuffer,
			D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		void Set(UINT64 index, const T* data, UINT64 numElement);
		void Set(UINT64 index, const T& element);
		bool IsConstantBuffer() const noexcept { return isConstantBuffer; }
		UINT64 NumElement() const noexcept { return numElement; }
	private:
		static constexpr UINT ElementSize(bool isConstantBuffer);
		UINT64 numElement;
		bool isConstantBuffer;
	};
}

#include "details/UploadBuffer.inl"
