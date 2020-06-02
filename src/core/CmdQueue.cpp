#include <UDX12/CmdQueue.h>

using namespace Ubpa;

void DX12::CmdQueue::Execute(ID3D12GraphicsCommandList* list) {
	raw->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&list));
}
