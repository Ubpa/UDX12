#pragma once

#include "FrameResource.h"

namespace Ubpa::UDX12 {
	class FrameResourceMngr {
	public:
		FrameResourceMngr(size_t numFrame, ID3D12Device*);
		~FrameResourceMngr();
		size_t GetNumFrame() const noexcept { return frameResources.size(); }
		FrameResource* GetCurrentFrameResource() noexcept;
		const std::vector<std::unique_ptr<FrameResource>>& GetFrameResources() const noexcept { return frameResources; }
		size_t GetCurrentCpuFence() const noexcept { return cpuFence; };

		void BeginFrame();
		void EndFrame(ID3D12CommandQueue*);
	private:
		HANDLE eventHandle;
		std::vector<std::unique_ptr<FrameResource>> frameResources;
		size_t cpuFence{ 0 };
		Microsoft::WRL::ComPtr<ID3D12Fence> gpuFence;
	};
}
