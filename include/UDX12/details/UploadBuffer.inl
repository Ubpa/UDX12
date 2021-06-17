#pragma once

namespace Ubpa::UDX12 {
	template<typename T>
	constexpr UINT ArrayUploadBuffer<T>::ElementSize(bool isConstantBuffer) {
		return isConstantBuffer ? Util::CalcConstantBufferByteSize(sizeof(T)) : sizeof(T);
	}

	template<typename T>
	ArrayUploadBuffer<T>::ArrayUploadBuffer(ID3D12Device* device, UINT64 numElement, bool isConstantBuffer,
		D3D12_RESOURCE_FLAGS flag)
		: UploadBuffer{device, numElement * ElementSize(isConstantBuffer), flag },
		isConstantBuffer{ isConstantBuffer },
		numElement{ numElement }
	{
	}

	template<typename T>
	void ArrayUploadBuffer<T>::Set(UINT64 index, const T* data, UINT64 numElement) {
		assert(index < this->numElement);
		auto elementSize = ElementSize(isConstantBuffer);
		UploadBuffer::Set(index * elementSize, data, numElement * elementSize);
	}

	template<typename T>
	void ArrayUploadBuffer<T>::Set(UINT64 index, const T& element) {
		assert(index < numElement);
		auto elementSize = ElementSize(isConstantBuffer);
		UploadBuffer::Set(index * elementSize, &element, elementSize);
	}

	// ===

	template<typename T>
	constexpr UINT VectorUploadBuffer<T>::ElementSize(bool isConstantBuffer) {
		return isConstantBuffer ? Util::CalcConstantBufferByteSize(sizeof(T)) : sizeof(T);
	}

	template<typename T>
	VectorUploadBuffer<T>::VectorUploadBuffer(ID3D12Device* device, bool isConstantBuffer,
		D3D12_RESOURCE_FLAGS flag)
		: DynamicUploadBuffer{ device, 0, flag },
		isConstantBuffer{ isConstantBuffer },
		size{ 0 }
	{
	}

	template<typename T>
	void VectorUploadBuffer<T>::Pushback(const T& ele) {
		if (size == Capacity())
			Reserve(size == 0 ? 1 : 2 * size);
		Set<T>(size * ElementSize<T>(isConstantBuffer), &ele);
	}

	template<typename T>
	T& VectorUploadBuffer<T>::At(size_t idx) {
		if (idx >= size)
			throw std::out_of_range{ "VectorUploadBuffer::At out of range" };
		return *(Data() + idx);
	}

	template<typename T>
	const T& VectorUploadBuffer<T>::At(size_t idx) const {
		return const_cast<VectorUploadBuffer*>(this)->At(idx);
	}

	template<typename T>
	T& VectorUploadBuffer<T>::operator[](size_t idx) noexcept {
		assert(idx < size);
		return *(Data() + idx);
	}

	template<typename T>
	const T& VectorUploadBuffer<T>::operator[](size_t idx) const noexcept {
		return const_cast<VectorUploadBuffer*>(this)->operator[](idx);
	}

	template<typename T>
	D3D12_GPU_VIRTUAL_ADDRESS VectorUploadBuffer<T>::GpuAdressAt(size_t idx) const {
		if (idx >= size)
			throw std::out_of_range{ "VectorUploadBuffer::GpuAdressAt out of range" };

		return DynamicUploadBuffer::GetResource()->GetGPUVirtualAddress() + idx * ElementSize<T>(isConstantBuffer);
	}
}
