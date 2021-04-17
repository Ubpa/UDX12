#pragma once

#include "Util.h"

#include <UFunction.hpp>

namespace Ubpa::UDX12 {
	class ResourceDeleteBatch {
	public:
		ResourceDeleteBatch() = default;
		ResourceDeleteBatch(const ResourceDeleteBatch&) = delete;
		ResourceDeleteBatch(ResourceDeleteBatch&&) noexcept = default;
		ResourceDeleteBatch& operator=(const ResourceDeleteBatch&) = delete;
		ResourceDeleteBatch& operator=(ResourceDeleteBatch&&) noexcept = default;
		~ResourceDeleteBatch();
		void Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
		void AddCallback(unique_function<void()> callback);
		void Release();
	private:
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
		std::vector<unique_function<void()>> callbacks;
	};
}
