#include <UDX12/ResourceDeleteBatch.h>

#include <DirectXHelpers.h>

using namespace Ubpa;

std::future<void> UDX12::ResourceDeleteBatch::Commit(ID3D12Device* device, ID3D12CommandQueue* cmdQueue) {
	// Set an event so we get notified when the GPU has completed all its work
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.GetAddressOf())));

	HANDLE gpuCompletedEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	if (!gpuCompletedEvent)
		throw std::exception("CreateEventEx");

	ThrowIfFailed(cmdQueue->Signal(fence.Get(), 1ULL));
	ThrowIfFailed(fence->SetEventOnCompletion(1ULL, gpuCompletedEvent));

	// Kick off a thread that waits for the upload to complete on the GPU timeline.
	// Let the thread run autonomously, but provide a future the user can wait on.
	std::future<void> future = std::async(
		std::launch::async,
		[gpuCompletedEvent, deleteBatch = std::move(*this)]() {
			// Wait on the GPU-complete notification
			DWORD wr = WaitForSingleObject(gpuCompletedEvent, INFINITE);
			if (wr != WAIT_OBJECT_0)
			{
				if (wr == WAIT_FAILED)
				{
					ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
				}
				else
				{
					throw std::exception("WaitForSingleObject");
				}
			}

			// Delete the batch
			for (auto rsrc : deleteBatch.resources)
				rsrc->Release();
		}
	);

	// Reset our state
	resources.clear();

	return future;
}
