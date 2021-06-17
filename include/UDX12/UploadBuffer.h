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
		void* GetMappedData() const noexcept { return mappedData; }
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
		void* mappedData;
		UINT64 size;
	};

	class DynamicUploadBuffer {
	public:
		DynamicUploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		ID3D12Resource* GetResource() const noexcept;
		UINT64 Size() const noexcept;
		bool Valid() const noexcept { return (bool)buffer; }
		void* GetMappedData() const noexcept;

		// retain original data when resizing
		// [sync]
		// - (maybe) construct resized upload buffer
		// - (maybe) orignal upload buffer -> new upload buffer
		void Reserve(size_t size);

		// not retain original data when resizing
		// [sync]
		// - (maybe) construct resized upload buffer
		void FastReserve(size_t size);

		void Resize(size_t size);

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

	class DynamicUploadVector {
	public:
		DynamicUploadVector(ID3D12Device* device, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE)
			: buffer(device, 0, flag), size{ 0 } {}

		void* Data() noexcept { return buffer.GetMappedData(); }
		const void* Data() const noexcept { return buffer.GetMappedData(); }
		UINT64 Capacity() const noexcept { return buffer.Size(); }
		bool Empty() const noexcept { return size == 0; }
		UINT64 Size() const noexcept { return size; }

		// offset + data_size < Size()
		void Set(size_t offset, const void* data, UINT64 data_size) {
			assert(offset + data_size < Size());
			buffer.Set(offset, data, data_size);
		}

		void Pushback(const void* data, UINT64 data_size) {
			if (size + data_size < Capacity())
				Reserve(2 * (size + data_size));
			buffer.Set(size, data, data_size);
			size += data_size;
		}

		void Reserve(UINT64 capacity) { buffer.Reserve(capacity); }
		void FastReserve(UINT64 capacity) { buffer.FastReserve(capacity); }

		void Popback(UINT64 data_size) { assert(size > data_size); size -= data_size; }

		void Clear() noexcept { size = 0; }

		void Resize(UINT64 s) {
			if (s > Capacity())
				buffer.Resize(s);
			size = s;
		}

		ID3D12Resource* GetResource() const noexcept { return buffer.GetResource(); }
	private:
		DynamicUploadBuffer buffer;
		UINT64 size;
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

	template<typename T>
	class VectorUploadBuffer : public DynamicUploadBuffer {
	public:
		VectorUploadBuffer(ID3D12Device* device, bool isConstantBuffer, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
		bool IsConstantBuffer() const noexcept { return isConstantBuffer; }

		void* Data() noexcept { return DynamicUploadBuffer::GetMappedData(); }
		const void* Data() const noexcept { return DynamicUploadBuffer::GetMappedData(); }

		T& At(size_t idx);
		const T& At(size_t idx) const;

		T& operator[](size_t idx) noexcept;
		const T& operator[](size_t idx) const noexcept;

		bool Empty() const noexcept { return size == 0; }
		size_t Size() const noexcept { return size; }

		void Reserve(size_t n) { DynamicUploadBuffer::Reserve(n * ElementSize<T>(isConstantBuffer)); }
		size_t Capacity() const noexcept { DynamicUploadBuffer::Size() / ElementSize<T>(isConstantBuffer); }
		
		void Clear() noexcept { size = 0; }
		
		void Pushback(const T& ele);
		void Popback() noexcept { assert(size > 0); --size; }

		D3D12_GPU_VIRTUAL_ADDRESS GpuAdressAt(size_t idx) const;
		
	private:
		static constexpr UINT ElementSize(bool isConstantBuffer);
		bool isConstantBuffer;
		size_t size;
	};
}

#include "details/UploadBuffer.inl"
