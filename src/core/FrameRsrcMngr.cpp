#include <UDX12/FrameRsrcMngr.h>

using namespace Ubpa;

void UDX12::FrameRsrcMngr::Signal(ID3D12CommandQueue* cmdQueue, UINT64 cpuFence) {
	this->cpuFence = cpuFence;
	cmdQueue->Signal(gpuFence, cpuFence);
}

void UDX12::FrameRsrcMngr::Wait() {
    if (cpuFence == 0 || gpuFence->GetCompletedValue() >= cpuFence)
        return;

    HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(gpuFence->SetEventOnCompletion(cpuFence, eventHandle));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);

    for (const auto& name : delayUnregisterResources)
        UnregisterResource(name);
    delayUnregisterResources.clear();

    for (const auto& [name, updator] : delayUpdatorMap)
        updator(resourceMap[name]);
    delayUpdatorMap.clear();
}

UDX12::FrameRsrcMngr& UDX12::FrameRsrcMngr::UnregisterResource(std::string_view name) {
    assert(HaveResource(name));

    resourceMap.erase(resourceMap.find(name));

    return *this;
}

UDX12::FrameRsrcMngr& UDX12::FrameRsrcMngr::DelayUnregisterResource(std::string name) {
    assert(HaveResource(name));
    delayUnregisterResources.emplace_back(std::move(name));
    return *this;
}
