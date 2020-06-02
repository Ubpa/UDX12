#pragma once

#include "Util.h"

namespace Ubpa::DX12 {
	struct CmdQueue : ComPtrHolder<ID3D12CommandQueue> {
		void Execute(ID3D12GraphicsCommandList* list);
	};
}
