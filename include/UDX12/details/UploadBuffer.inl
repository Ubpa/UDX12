#pragma once

namespace Ubpa::DX12 {
	template<typename T>
	constexpr UINT ArrayUploadBuffer<T>::ElementSize(bool isConstantBuffer) {
		return isConstantBuffer ? Util::CalcConstantBufferByteSize(sizeof(T)) : sizeof(T);
	}

	template<typename T>
	ArrayUploadBuffer<T>::ArrayUploadBuffer(ID3D12Device* device, UINT64 numElement, bool isConstantBuffer,
		D3D12_RESOURCE_FLAGS flag)
		: UploadBuffer{device, numElement * ElementSize(isConstantBuffer), flag },
		isConstantBuffer{ isConstantBuffer }
	{
	}

	template<typename T>
	void ArrayUploadBuffer<T>::Set(UINT64 index, const T* data, UINT64 numElement) {
		auto elementSize = ElementSize(isConstantBuffer);
		UploadBuffer::Set(index * elementSize, data, numElement * elementSize);
	}

	template<typename T>
	void ArrayUploadBuffer<T>::Set(UINT64 index, const T& element) {
		auto elementSize = ElementSize(isConstantBuffer);
		UploadBuffer::Set(index * elementSize, &element, elementSize);
	}
}
