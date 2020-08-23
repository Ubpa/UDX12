#pragma once

#include "Util.h"

#include <future>

namespace Ubpa::UDX12 {
	class ResourceDeleteBatch {
	public:
		void Add(ID3D12Resource* resource) { resources.push_back(resource); }
		std::future<void> Commit(ID3D12Device* device, ID3D12CommandQueue*);
	private:
		std::vector<ID3D12Resource*> resources;
	};
}
