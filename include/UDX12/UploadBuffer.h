#pragma once

#include "Util.h"

#include "ResourceDeleteBatch.h"
#include "_deps/DirectXTK12/ResourceUploadBatch.h"

#include <memory>

namespace Ubpa::UDX12 {
	// create a buffer for uploading
	// we will map the gpu buffer to a cpu pointer
	// so it act like a cpu buffer
	class UploadBuffer {
	public:
		UploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
		~UploadBuffer();

		ID3D12Resource* GetResource() const noexcept { return resource.Get(); }
		UINT64 Size() const noexcept { return size; }
		BYTE* GetMappedData() const noexcept { return mappedData; }

		void Set(UINT64 offset, const void* data, UINT64 size);

		void CopyConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		void MoveConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ResourceDeleteBatch& deleteBatch,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		void CopyAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);

		void MoveAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ResourceDeleteBatch& deleteBatch,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);
	private:
		void Delete(ResourceDeleteBatch& deleteBatch);

		ComPtr<ID3D12Resource> resource;
		BYTE* mappedData;
		UINT64 size;
	};

	class DynamicUploadBuffer {
	public:
		DynamicUploadBuffer(ID3D12Device* device, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		ID3D12Resource* GetResource() const noexcept;
		UINT64 Size() const noexcept;

		// retain original data when resizing
		void Reserve(size_t size);
		// not retain original data when resizing
		void FastReserve(size_t size);
		void Set(UINT64 offset, const void* data, UINT64 size);

		void CopyConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		void MoveConstruct(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ResourceDeleteBatch& deleteBatch,
			ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			D3D12_RESOURCE_STATES afterState,
			ID3D12Resource** pBuffer, // out com ptr
			D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE
		);

		void CopyAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);

		void MoveAssign(
			size_t dstOffset, size_t srcOffset, size_t numBytes,
			ResourceDeleteBatch& deleteBatch,
			ID3D12GraphicsCommandList* cmdList,
			ID3D12Resource* dst,
			D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
		);
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
