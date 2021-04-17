#include <UDX12/FrameResourceMngr.h>

using namespace Ubpa;

UDX12::FrameResourceMngr::FrameResourceMngr(size_t numFrame, ID3D12Device* device) {
	assert(numFrame > 0 && device);
	ThrowIfFailed(device->CreateFence(cpuFence, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&gpuFence)));
	for (size_t i = 0; i < numFrame; i++)
		frameResources.push_back(std::make_unique<FrameResource>(cpuFence, gpuFence.Get()));
	eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
}

UDX12::FrameResourceMngr::~FrameResourceMngr() {
	CloseHandle(eventHandle);
}

UDX12::FrameResource* UDX12::FrameResourceMngr::GetCurrentFrameResource() noexcept {
	return frameResources[cpuFence % frameResources.size()].get();
}

void UDX12::FrameResourceMngr::BeginFrame() {
	GetCurrentFrameResource()->BeginFrame(eventHandle);
}

void UDX12::FrameResourceMngr::EndFrame(ID3D12CommandQueue* cmdQueue) {
	GetCurrentFrameResource()->Signal(cmdQueue, ++cpuFence);
}
