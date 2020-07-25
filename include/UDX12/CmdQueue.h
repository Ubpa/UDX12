#pragma once

#include "Util.h"

namespace Ubpa::UDX12 {
	struct CmdQueue : Util::ComPtrHolder<ID3D12CommandQueue> {
		void Execute(ID3D12GraphicsCommandList* list);
	};
}
