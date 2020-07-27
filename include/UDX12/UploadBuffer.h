#pragma once

#include "Util.h"

namespace Ubpa::UDX12 {
	// create a buffer for uploading
	// we will map the gpu buffer to a cpu pointer
	// so it act like a cpu buffer
	class UploadBuffer {
	public:
		UploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		ID3D12Resource* GetResource() const noexcept { return resource.Get(); }

		void Set(UINT64 offset, const void* data, UINT64 size);

	private:
		ComPtr<ID3D12Resource> resource;
		BYTE* mappedData{ nullptr };
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
