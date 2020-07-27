#pragma once

#include "Util.h"
#include <functional>
#include <map>
#include <unordered_map>

namespace Ubpa::UDX12 {
	class FrameResource {
	public:
		FrameResource(ID3D12Fence* gpuFence) : gpuFence{ gpuFence } {}
		~FrameResource();

		void Signal(ID3D12CommandQueue* cmdQueue, UINT64 cpuFence);
		void Wait();

		bool HaveResource(std::string_view name) const { return resourceMap.find(name) != resourceMap.end(); }

		FrameResource& RegisterResource(std::string name, void* pResource, std::function<void(void*)> deletor);
		// use T::~T() as deletor
		template<typename T>
		FrameResource& RegisterResource(std::string name, T* pResource);
		FrameResource& RegisterTemporalResource(std::string name, void* pResource, std::function<void(void*)> deletor);

		FrameResource& UnregisterResource(std::string_view name);
		FrameResource& DelayUnregisterResource(std::string name);

		FrameResource& DelayUpdateResource(std::string name, std::function<void(void*)> updator);

		template<typename T>
		T* GetResource(std::string_view name) const;

	private:
		ID3D12Fence* gpuFence;
		UINT64 cpuFence{ 0 };
		std::map<std::string, void*, std::less<>> resourceMap;
		std::unordered_map<void*, std::function<void(void*)>> deletorMap;
		std::vector<std::string> delayUnregisterResources;
		std::vector<std::tuple<std::string, std::function<void(void*)>>> delayUpdateResources;
	};
}

#include "details/FrameResource.inl"
