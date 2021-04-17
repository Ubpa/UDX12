#include <UDX12/ResourceDeleteBatch.h>

using namespace Ubpa;

UDX12::ResourceDeleteBatch::~ResourceDeleteBatch() { Release(); }

void UDX12::ResourceDeleteBatch::Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource) { resources.push_back(std::move(resource)); }

void UDX12::ResourceDeleteBatch::AddCallback(unique_function<void()> callback) { callbacks.push_back(std::move(callback)); }

void UDX12::ResourceDeleteBatch::Release() {
	resources.clear();
	for (auto& callback : callbacks)
		callback();
	callbacks.clear();
}
