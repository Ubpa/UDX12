#pragma once

#include "Util.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <any>

namespace Ubpa::UDX12 {
	// frame resource mngr
	class FrameRsrcMngr {
	public:
		FrameRsrcMngr(UINT64 cpuFence, ID3D12Fence* gpuFence) : cpuFence{ cpuFence }, gpuFence { gpuFence } {}

		// cpu at frame [cpuFence] use the resources
		// when the frame complete (GPU instructions complete), gpuFence <- cpuFence
		void Signal(ID3D12CommandQueue* cmdQueue, UINT64 cpuFence);

		// at the cpu frame beginning, you should wait the frame complete
		// block cpu
		// run delay updator and unregister
		void Wait();

		bool HaveResource(std::string_view name) const { return resourceMap.find(name) != resourceMap.end(); }

		template<typename T>
		FrameRsrcMngr& RegisterResource(std::string name, T&& resource);

		FrameRsrcMngr& UnregisterResource(std::string_view name);
		FrameRsrcMngr& DelayUnregisterResource(std::string name);
		template<typename Func>
		FrameRsrcMngr& DelayUpdateResource(std::string name, Func&& updator);

		template<typename T>
		T& GetResource(std::string_view name);
		template<typename T>
		const T& GetResource(std::string_view name) const;

	private:
		ID3D12Fence* gpuFence;
		UINT64 cpuFence;
		std::map<std::string, std::any, std::less<>> resourceMap;
		std::vector<std::string> delayUnregisterResources;
		std::vector<std::tuple<std::string, std::function<void(std::any&)>>> delayUpdatorMap;
	};
}

#include "details/FrameRsrcMngr.inl"
