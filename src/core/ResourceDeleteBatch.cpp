#include <UDX12/ResourceDeleteBatch.h>

using namespace Ubpa;

std::function<void()> UDX12::ResourceDeleteBatch::Pack() {
	return [resources = std::move(resources), callbacks = std::move(callbacks)]() mutable {
		resources.clear();
		for (const auto& callback : callbacks)
			callback();
	};
}
