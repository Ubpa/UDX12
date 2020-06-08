#pragma once

#include "Util.h"

namespace Ubpa::DX12 {
	class UploadBuffer {
	public:
		UploadBuffer(ID3D12Device* device, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		ID3D12Resource* GetResource() const noexcept { return resource.Get(); }

		void Set(UINT64 offset, const void* data, UINT64 size);

	private:
		ComPtr<ID3D12Resource> resource;
		BYTE* mappedData{ nullptr };
	};

	template<typename T>
	class ArrayUploadBuffer : public UploadBuffer {
	public:
		ArrayUploadBuffer(ID3D12Device* device, UINT64 numElement, bool isConstantBuffer,
			D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

		void Set(UINT64 index, const T* data, UINT64 numElement);
		void Set(UINT64 index, const T& element);
		bool IsConstantBuffer() const noexcept { return isConstantBuffer; }
	private:
		static constexpr UINT ElementSize(bool isConstantBuffer);
		bool isConstantBuffer;
	};
}

#include "details/UploadBuffer.inl"
