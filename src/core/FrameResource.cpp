#include <UDX12/FrameResource.h>

using namespace Ubpa;

void UDX12::FrameResource::Signal(ID3D12CommandQueue* cmdQueue, UINT64 cpuFence) {
	this->cpuFence = cpuFence;
	cmdQueue->Signal(gpuFence, cpuFence);
}

void UDX12::FrameResource::BeginFrame(HANDLE sharedEventHandle) {
	if (gpuFence->GetCompletedValue() < cpuFence) {
		ThrowIfFailed(gpuFence->SetEventOnCompletion(cpuFence, sharedEventHandle));
		WaitForSingleObject(sharedEventHandle, INFINITE);
    }

    for (const auto& name : delayUnregisterResources)
        UnregisterResource(name);
    delayUnregisterResources.clear();

    for (const auto& [name, updator] : delayUpdatorMap)
        updator(resourceMap.at(name));
    delayUpdatorMap.clear();
}

UDX12::FrameResource& UDX12::FrameResource::UnregisterResource(std::string_view name) {
    assert(HaveResource(name));

    resourceMap.erase(resourceMap.find(name));

    return *this;
}

UDX12::FrameResource& UDX12::FrameResource::DelayUnregisterResource(std::string name) {
    assert(HaveResource(name));
    delayUnregisterResources.emplace_back(std::move(name));
    return *this;
}
