#pragma once

#include "Rsrc.h"

#include <UFG/Compiler.hpp>
#include <UThreadPool/UThreadPool.hpp>

#include <functional>

namespace Ubpa::UDX12::FG {
	class RsrcMngr;

	class Executor {
	public:
		using PassFunction = unique_function<void(ID3D12GraphicsCommandList*, const PassRsrcs&) const>;

		Executor(ID3D12Device* device, size_t num_threads = std::thread::hardware_concurrency()) :
			device{ device }, threadpool{ num_threads } {}

		Executor& RegisterPassFunc(size_t passNodeIdx, PassFunction func) {
			passFuncs[passNodeIdx] = std::move(func);
			return *this;
		}

		void NewFrame() {
			for (auto allocator : used_allocators) {
				allocator->Reset();
				free_allocators.push_back(allocator);
			}
			used_allocators.clear();
		}

		void Execute(
			ID3D12CommandQueue* cmdQueue,
			const UFG::Compiler::Result& crst,
			RsrcMngr& rsrcMngr
		);

	private:
		ThreadPool threadpool;
		ID3D12Device* device;
		std::unordered_map<size_t, PassFunction> passFuncs;
		std::vector<ComPtr<ID3D12CommandAllocator>> free_allocators;
		std::vector<ComPtr<ID3D12CommandAllocator>> used_allocators;
	};
}
