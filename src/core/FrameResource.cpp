#include <UDX12/FrameResource.h>

using namespace Ubpa;

DX12::FrameResource::~FrameResource() {
    for (const auto& [name, ptr] : resourceMap)
        deletorMap[ptr](ptr);
}

void DX12::FrameResource::Signal(ID3D12CommandQueue* cmdQueue, UINT64 cpuFence) {
	this->cpuFence = cpuFence;
	cmdQueue->Signal(gpuFence, cpuFence);
}

void DX12::FrameResource::Wait() {
    if (cpuFence == 0 || gpuFence->GetCompletedValue() >= cpuFence)
        return;

    HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(gpuFence->SetEventOnCompletion(cpuFence, eventHandle));
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);

    for (const auto& name : delayUnregisterResources)
        UnregisterResource(name);
    delayUnregisterResources.clear();

    for (const auto& [name, updator] : delayUpdateResources)
        updator(resourceMap[name]);
    delayUpdateResources.clear();
}

DX12::FrameResource& DX12::FrameResource::RegisterResource(
    std::string name,
    void* pResource,
    std::function<void(void*)> deletor)
{
    assert(!HaveResource(name));
    resourceMap.emplace(std::move(name), pResource);
    deletorMap[pResource] = std::move(deletor);
    return *this;
}

DX12::FrameResource& DX12::FrameResource::RegisterTemporalResource(
    std::string name,
    void* pResource,
    std::function<void(void*)> deletor)
{
    assert(!HaveResource(name));
    RegisterResource(name, pResource, std::move(deletor));
    DelayUnregisterResource(std::move(name));
    return *this;
}

DX12::FrameResource& DX12::FrameResource::UnregisterResource(const std::string& name) {
    assert(HaveResource(name));
    auto ptr = resourceMap[name];
    deletorMap[ptr](ptr);
    deletorMap.erase(ptr);
    resourceMap.erase(name);
    return *this;
}

DX12::FrameResource& DX12::FrameResource::DelayUnregisterResource(std::string name) {
    assert(HaveResource(name));
    delayUnregisterResources.emplace_back(std::move(name));
    return *this;
}

DX12::FrameResource& DX12::FrameResource::DelayUpdateResource(std::string name, std::function<void(void*)> updator) {
    assert(HaveResource(name));
    delayUpdateResources.emplace_back(std::move(name), std::move(updator));
    return *this;
}
