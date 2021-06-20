#include <UDX12/FrameGraph/Executor.h>

#include <UDX12/FrameGraph/RsrcMngr.h>

using namespace Ubpa::UDX12::FG;
using namespace Ubpa::UDX12;
using namespace Ubpa;

void Executor::Execute(
	ID3D12CommandQueue* cmdQueue,
	const UFG::Compiler::Result& crst,
	RsrcMngr& rsrcMngr
) {
	rsrcMngr.DHReserve();
	rsrcMngr.AllocateHandle();

	const size_t cmdlist_num = crst.sorted_passes.size();
	if (cmdlist_num == 0)
		return;
	
	std::vector<ID3D12GraphicsCommandList*> cmdlists(cmdlist_num, nullptr);
	std::mutex mutex_cnt;
	size_t cnt = 0;
	std::condition_variable cv_cnt;

	std::mutex mutex_rsrcMngr;

	for (size_t i = 0; i < cmdlist_num; ++i) {
		if (free_allocators.empty()) {
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
			ThrowIfFailed(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&allocator)));
			free_allocators.push_back(std::move(allocator));
		}

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator = free_allocators.back();
		free_allocators.pop_back();
		used_allocators.push_back(allocator);

		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			allocator.Get(),           // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(&cmdlists[i])));
	}

	if (auto target = crst.pass2info.find(static_cast<size_t>(-1)); target != crst.pass2info.end()) {
		const auto& info = target->second;

		for (auto rsrc : info.construct_resources)
			rsrcMngr.Construct(rsrc);

		for (auto rsrc : info.move_resources) {
			auto src = rsrc;
			auto dst = crst.moves_src2dst.at(src);
			rsrcMngr.Move(dst, src);
		}

		for (auto rsrc : info.destruct_resources){
			rsrcMngr.DestructCPU(rsrc);
			rsrcMngr.DestructGPU(cmdlists.front(), rsrc);
		}
	}

	{
		// let worker threads wait for the main thread
		std::lock_guard<std::mutex> guard(mutex_rsrcMngr);
		for (auto pass : crst.sorted_passes) {
			const auto& passInfo = crst.pass2info.at(pass);
			for (auto rsrc : passInfo.construct_resources)
				rsrcMngr.Construct(rsrc);

			auto passRsrcs = rsrcMngr.RequestPassRsrcs(cmdlists[pass], pass);

			for (auto rsrc : passInfo.move_resources) {
				auto src = rsrc;
				auto dst = crst.moves_src2dst.at(src);
				rsrcMngr.Move(dst, src);
			}

			for (auto rsrc : passInfo.destruct_resources)
				rsrcMngr.DestructCPU(rsrc);

			if (auto target = passFuncs.find(pass); target != passFuncs.end()) {
				threadpool.Enqueue(
					[
						rsrcs = std::move(passRsrcs), cmdlist = cmdlists[pass], func = std::move(target->second), pass, cmdlist_num,
						&crst, &mutex_cnt, &cnt, &cv_cnt, &rsrcMngr, &mutex_rsrcMngr
					]
					() {
						func(cmdlist, rsrcs);

						const auto& passinfo = crst.pass2info.at(pass);
						if (!passinfo.destruct_resources.empty()) {
							// 1. wait main thread
							// 2. avoid data race with other worker threads
							std::lock_guard<std::mutex> guard(mutex_rsrcMngr);
							for (auto rsrc : passinfo.destruct_resources)
								rsrcMngr.DestructGPU(cmdlist, rsrc);
						}

						cmdlist->Close();

						{ // add cnt
							std::lock_guard<std::mutex> lk(mutex_cnt);
							++cnt;
							if (cnt == cmdlist_num)
								cv_cnt.notify_one();
						}
					}
				);
			}
		}
	}
	

	{
		std::unique_lock<std::mutex> lk(mutex_cnt);
		if (cnt != cmdlist_num) {
			cv_cnt.wait(lk);
			assert(cnt == cmdlist_num);
		}
	}
	cmdQueue->ExecuteCommandLists(cmdlists.size(), (ID3D12CommandList* const*)cmdlists.data());
	for (auto* cmdlist : cmdlists)
		cmdlist->Release();
}
