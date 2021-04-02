#pragma once

#include "Util.h"

#include <future>

namespace Ubpa::UDX12 {
	class ResourceDeleteBatch {
	public:
		void Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource) { resources.push_back(std::move(resource)); }
		void AddCallback(std::function<void()> callback) { callbacks.push_back(std::move(callback)); }

		std::function<void()> Pack();
	private:
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
		std::vector<std::function<void()>> callbacks;
	};
}
