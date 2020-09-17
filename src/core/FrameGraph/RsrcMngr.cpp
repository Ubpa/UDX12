#include <UDX12/FrameGraph/RsrcMngr.h>

#include <UFG/FrameGraph.h>

using namespace Ubpa::UDX12::FG;
using namespace Ubpa::UDX12;
using namespace Ubpa;
using namespace std;

#include <DirectXColors.h>

RsrcMngr::RsrcMngr() {
	csuDynamicDH = new DynamicSuballocMngr{
		*DescriptorHeapMngr::Instance().GetCSUGpuDH(),
		256,
		"RsrcMngr::csuDynamicDH" };
}

RsrcMngr::~RsrcMngr() {
	if (!csuDH.IsNull())
		DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(csuDH));
	if (!rtvDH.IsNull())
		DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(move(rtvDH));
	if (!dsvDH.IsNull())
		DescriptorHeapMngr::Instance().GetDSVCpuDH()->Free(move(dsvDH));
	csuDynamicDH->ReleaseAllocations();
	delete csuDynamicDH;
}

void RsrcMngr::NewFrame() {
	importeds.clear();
	temporals.clear();
	passNodeIdx2rsrcMap.clear();
	actives.clear();

	for (const auto& idx : csuDHused)
		csuDHfree.push_back(idx);
	for (const auto& idx : rtvDHused)
		rtvDHfree.push_back(idx);
	for (const auto& idx : dsvDHused)
		dsvDHfree.push_back(idx);

	csuDHused.clear();
	rtvDHused.clear();
	dsvDHused.clear();

	if(csuDynamicDH)
		csuDynamicDH->ReleaseAllocations();
	typeinfoMap.clear();
}

void RsrcMngr::Clear() {
	NewFrame();
	rsrcKeeper.clear();
	pool.clear();
}

void RsrcMngr::CSUDHReserve(UINT num) {
	assert(csuDHused.empty());

	UINT origSize = csuDH.GetNumHandles();
	if (origSize >= num)
		return;

	if (!csuDH.IsNull())
		DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(csuDH));
	csuDH = DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(num);

	for (UINT i = origSize; i < num; i++)
		csuDHfree.push_back(i);
}

void RsrcMngr::RtvDHReserve(UINT num) {
	assert(rtvDHused.empty());

	UINT origSize = rtvDH.GetNumHandles();
	if (origSize >= num)
		return;

	if (!rtvDH.IsNull())
		DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(move(rtvDH));
	rtvDH = DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(num);

	for (UINT i = origSize; i < num; i++)
		rtvDHfree.push_back(i);
}

void RsrcMngr::DsvDHReserve(UINT num) {
	assert(dsvDHused.empty());

	UINT origSize = dsvDH.GetNumHandles();
	if (dsvDH.GetNumHandles() >= num)
		return;

	if (!dsvDH.IsNull())
		DescriptorHeapMngr::Instance().GetDSVCpuDH()->Free(move(dsvDH));
	dsvDH = DescriptorHeapMngr::Instance().GetDSVCpuDH()->Allocate(num);

	for (UINT i = origSize; i < num; i++)
		dsvDHfree.push_back(i);
}

void RsrcMngr::DHReserve() {
	UINT numCSU = 0;
	UINT numRTV = 0;
	UINT numDSV = 0;
	
	struct DHRecord {
		std::unordered_set<D3D12_CONSTANT_BUFFER_VIEW_DESC>  descs_cbv;
		std::unordered_set<D3D12_SHADER_RESOURCE_VIEW_DESC>  descs_srv;
		std::unordered_set<D3D12_UNORDERED_ACCESS_VIEW_DESC> descs_uav;
		std::unordered_set<D3D12_RENDER_TARGET_VIEW_DESC>    descs_rtv;
		std::unordered_set<D3D12_DEPTH_STENCIL_VIEW_DESC>    descs_dsv;

		bool null_cbv{ false };
		bool null_srv{ false };
		bool null_uav{ false };
		bool null_rtv{ false };
		bool null_dsv{ false };
	};
	std::unordered_map<size_t, DHRecord> rsrc2record;
	for (const auto& [passNodeIdx, rsrcs] : passNodeIdx2rsrcMap) {
		for (const auto& [rsrcNodeIdx, state_descs] : rsrcs) {
			auto& record = rsrc2record[rsrcNodeIdx];
			const auto& [state, descs] = state_descs;
			for (const auto& desc : descs) {
				std::visit([&](const auto& desc) {
					using T = std::decay_t<decltype(desc)>;
					// CBV
					if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>) {
						if (record.descs_cbv.find(desc) != record.descs_cbv.end())
							return;
						record.descs_cbv.insert(desc);
						numCSU++;
					}
					// SRV
					else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>) {
						if (record.descs_srv.find(desc) != record.descs_srv.end())
							return;
						record.descs_srv.insert(desc);
						numCSU++;
					}
					// UAV
					else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>) {
						if (record.descs_uav.find(desc) != record.descs_uav.end())
							return;
						record.descs_uav.insert(desc);
						numCSU++;
					}
					// RTV
					else if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>) {
						if (record.descs_rtv.find(desc) != record.descs_rtv.end())
							return;
						record.descs_rtv.insert(desc);
						numRTV++;
					}
					// DTV
					else if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>) {
						if (record.descs_dsv.find(desc) != record.descs_dsv.end())
							return;
						record.descs_dsv.insert(desc);
						numDSV++;
					}
					// SRV null
					else if constexpr (std::is_same_v<T, RsrcImplDesc_SRV_NULL>) {
						if (record.null_srv)
							return;
						record.null_srv = true;
						numCSU++;
					}
					// UAV null
					else if constexpr (std::is_same_v<T, RsrcImplDesc_UAV_NULL>) {
						if (record.null_uav)
							return;
						record.null_uav = true;
						numCSU++;
					}
					// RTV null
					else if constexpr (std::is_same_v<T, RsrcImplDesc_RTV_Null>) {
						if (record.null_rtv)
							return;
						record.null_rtv = true;
						numRTV++;
					}
					// DSV null
					else if constexpr (std::is_same_v<T, RsrcImplDesc_DSV_Null>) {
						if (record.null_dsv)
							return;
						record.null_dsv = true;
						numDSV++;
					}
					else
						static_assert(always_false_v<T>, "non-exhaustive visitor!");
				}, desc);
			}
		}
	}

	CSUDHReserve(numCSU);
	RtvDHReserve(numRTV);
	DsvDHReserve(numDSV);
}

void RsrcMngr::Construct(ID3D12Device* device, size_t rsrcNodeIdx) {
	SRsrcView view;

	if (IsImported(rsrcNodeIdx))
		view = importeds.find(rsrcNodeIdx)->second;
	else {
		const auto& type = temporals[rsrcNodeIdx];
		auto& typefrees = pool[type];
		if (typefrees.empty()) {
			view.state = D3D12_RESOURCE_STATE_COMMON;
			RsrcPtr ptr;
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&type.desc,
				view.state,
				&type.clearValue,
				IID_PPV_ARGS(ptr.GetAddressOf())));
			rsrcKeeper.push_back(ptr);
			view.pRsrc = ptr.Get();
		}
		else {
			view = typefrees.back();
			typefrees.pop_back();
		}
	}
	actives[rsrcNodeIdx] = view;
}

void RsrcMngr::Destruct(ID3D12GraphicsCommandList* cmdList, size_t rsrcNodeIdx) {
	auto view = actives[rsrcNodeIdx];
	if (!IsImported(rsrcNodeIdx))
		pool[temporals[rsrcNodeIdx]].push_back(view);
	else {
		auto orig_state = importeds[rsrcNodeIdx].state;
		if (view.state != orig_state) {
			cmdList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					view.pRsrc,
					view.state, orig_state
				)
			);
		}
	}
	actives.erase(rsrcNodeIdx);
}

void RsrcMngr::Move(size_t dstRsrcNodeIdx, size_t srcRsrcNodeIdx) {
	assert(dstRsrcNodeIdx != srcRsrcNodeIdx);
	assert(actives.find(dstRsrcNodeIdx) == actives.end());
	assert(actives.find(srcRsrcNodeIdx) != actives.end());

	actives.emplace(dstRsrcNodeIdx, actives[srcRsrcNodeIdx]);
	actives.erase(srcRsrcNodeIdx);

	if (IsImported(srcRsrcNodeIdx)) {
		importeds.emplace(dstRsrcNodeIdx, importeds[srcRsrcNodeIdx]);
		importeds.erase(srcRsrcNodeIdx);
	}
}

RsrcMngr& RsrcMngr::RegisterPassRsrcState(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcState state) {
	std::get<RsrcState>(passNodeIdx2rsrcMap[passNodeIdx][rsrcNodeIdx]) = state;
	return *this;
}

RsrcMngr& RsrcMngr::RegisterPassRsrcImplDesc(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcImplDesc desc) {
	auto& m = std::get<1>(passNodeIdx2rsrcMap[passNodeIdx][rsrcNodeIdx]);
	m.push_back(desc);
	return *this;
}

RsrcMngr& RsrcMngr::RegisterPassRsrc(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcState state, RsrcImplDesc desc) {
	auto& [s, descs] = passNodeIdx2rsrcMap[passNodeIdx][rsrcNodeIdx];
	s = state;
	descs.push_back(desc);
	return *this;
}

RsrcMngr& RsrcMngr::RegisterRsrcHandle(
	size_t rsrcNodeIdx,
	RsrcImplDesc desc,
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	auto& typeinfo = typeinfoMap[rsrcNodeIdx];
	std::visit([&](const auto& desc) {
		using T = std::decay_t<decltype(desc)>;
		// CBV
		if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_cbv[desc] = { cpuHandle, gpuHandle, true };
		}
		// SRV
		else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_srv[desc] = { cpuHandle, gpuHandle, true };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_SRV_NULL>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.null_info_srv = { cpuHandle, gpuHandle, true };
		}
		// UAV
		else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_uav[desc] = { cpuHandle, gpuHandle, true };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_UAV_NULL>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.null_info_uav = { cpuHandle, gpuHandle, true };
		}
		// RTV
		else if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.desc2info_rtv[desc] = { cpuHandle,true };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_RTV_Null>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.null_info_rtv = { cpuHandle,true };
		}
		// DSV
		else if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.desc2info_dsv[desc] = { cpuHandle,true };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_DSV_Null>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.null_info_dsv = { cpuHandle,true };
		}
		else
			static_assert(always_false_v<T>, "non-exhaustive visitor!");
		}, desc);
	return *this;
}

RsrcMngr& RsrcMngr::RegisterRsrcTable(const std::vector<std::tuple<size_t, RsrcImplDesc>>& rsrcNodeIndices) {
	auto allocation = csuDynamicDH->Allocate(static_cast<uint32_t>(rsrcNodeIndices.size()));
	for (uint32_t i = 0; i < rsrcNodeIndices.size(); i++) {
		const auto& [rsrcNodeIdx, desc] = rsrcNodeIndices[i];
		auto& typeinfo = typeinfoMap[rsrcNodeIdx];
		auto cpuHandle = allocation.GetCpuHandle(i);
		auto gpuHandle = allocation.GetGpuHandle(i);
		std::visit([&](const auto& desc) {
			using T = std::decay_t<decltype(desc)>;
			// CBV
			if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>) {
				typeinfo.desc2info_cbv[desc].cpuHandle = cpuHandle;
				typeinfo.desc2info_cbv[desc].gpuHandle = gpuHandle;
			}
			// SRV
			else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>) {
				typeinfo.desc2info_srv[desc].cpuHandle = cpuHandle;
				typeinfo.desc2info_srv[desc].gpuHandle = gpuHandle;
			}
			// SRV null
			else if constexpr (std::is_same_v<T, RsrcImplDesc_SRV_NULL>) {
				typeinfo.null_info_srv.cpuHandle = cpuHandle;
				typeinfo.null_info_srv.gpuHandle = gpuHandle;
			}
			// UAV
			else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>) {
				typeinfo.desc2info_uav[desc].cpuHandle = cpuHandle;
				typeinfo.desc2info_uav[desc].gpuHandle = gpuHandle;
			}
			// UAV null
			else if constexpr (std::is_same_v<T, RsrcImplDesc_UAV_NULL>) {
				typeinfo.null_info_uav.cpuHandle = cpuHandle;
				typeinfo.null_info_uav.gpuHandle = gpuHandle;
			}
			else
				assert("CBV, SRV, UAV");
		}, desc);
	}
	return *this;
}

void RsrcMngr::AllocateHandle() {
	for (const auto& [passNodeIdx, rsrcs] : passNodeIdx2rsrcMap) {
		for (const auto& [rsrcNodeIdx, state_descs] : rsrcs) {
			const auto& [state, descs] = state_descs;
			auto& typeinfo = typeinfoMap[rsrcNodeIdx];
			for (const auto& desc : descs) {
				std::visit([&](const auto& desc) {
					using T = std::decay_t<decltype(desc)>;
					// CBV
					if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>) {
						if (typeinfo.desc2info_cbv.find(desc) != typeinfo.desc2info_cbv.end())
							return;
						auto idx = csuDHfree.back();
						csuDHfree.pop_back();
						csuDHused.insert(idx);
						typeinfo.desc2info_cbv[desc] = { csuDH.GetCpuHandle(idx), csuDH.GetGpuHandle(idx), false };
					}
					// SRV
					else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>
						|| std::is_same_v<T, RsrcImplDesc_SRV_NULL>)
					{
						if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>) {
							if (typeinfo.desc2info_srv.find(desc) != typeinfo.desc2info_srv.end())
								return;
						}
						else { // std::is_same_v<T, RsrcImplDesc_SRV_NULL>
							if (typeinfo.HaveNullSrv())
								return;
						}
						auto idx = csuDHfree.back();
						csuDHfree.pop_back();
						csuDHused.insert(idx);
						if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>)
							typeinfo.desc2info_srv[desc] = { csuDH.GetCpuHandle(idx), csuDH.GetGpuHandle(idx), false };
						else
							typeinfo.null_info_srv = { csuDH.GetCpuHandle(idx), csuDH.GetGpuHandle(idx), false };
					}
					// UAV
					else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>
						|| std::is_same_v<T, RsrcImplDesc_UAV_NULL>)
					{
						if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>) {
							if (typeinfo.desc2info_uav.find(desc) != typeinfo.desc2info_uav.end())
								return;
						}
						else { // std::is_same_v<T, RsrcImplDesc_UAV_NULL>
							if (typeinfo.HaveNullUav())
								return;
						}
						auto idx = csuDHfree.back();
						csuDHfree.pop_back();
						csuDHused.insert(idx);
						if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>)
							typeinfo.desc2info_uav[desc] = { csuDH.GetCpuHandle(idx), csuDH.GetGpuHandle(idx), false };
						else
							typeinfo.null_info_uav = { csuDH.GetCpuHandle(idx), csuDH.GetGpuHandle(idx), false };
					}
					// RTV
					else if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>
						|| std::is_same_v<T, RsrcImplDesc_RTV_Null>)
					{
						if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>) {
							if (typeinfo.desc2info_rtv.find(desc) != typeinfo.desc2info_rtv.end())
								return;
						}
						else { // std::is_same_v<T, RsrcImplDesc_RTV_Null>
							if (typeinfo.HaveNullRtv())
								return;
						}
						auto idx = rtvDHfree.back();
						rtvDHfree.pop_back();
						rtvDHused.insert(idx);
						if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>)
							typeinfo.desc2info_rtv[desc] = { rtvDH.GetCpuHandle(idx), false };
						else
							typeinfo.null_info_rtv = { rtvDH.GetCpuHandle(idx), false };
					}
					// DSV
					else if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>
						|| std::is_same_v<T, RsrcImplDesc_DSV_Null>)
					{
						if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>) {
							if (typeinfo.desc2info_dsv.find(desc) != typeinfo.desc2info_dsv.end())
								return;
						}
						else { // std::is_same_v<T, RsrcImplDesc_DSV_Null>
							if (typeinfo.HaveNullDsv())
								return;
						}
						auto idx = dsvDHfree.back();
						dsvDHfree.pop_back();
						dsvDHused.insert(idx);
						if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>)
							typeinfo.desc2info_dsv[desc] = { dsvDH.GetCpuHandle(idx), false };
						else
							typeinfo.null_info_dsv = { dsvDH.GetCpuHandle(idx), false };
					}
					else
						static_assert(always_false_v<T>, "non-exhaustive visitor!");
				}, desc);
			}
		}
	}
}

PassRsrcs RsrcMngr::RequestPassRsrcs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, size_t passNodeIdx) {
	PassRsrcs passRsrc;
	const auto& rsrcMap = passNodeIdx2rsrcMap[passNodeIdx];
	for (const auto& [rsrcNodeIdx, state_descs] : rsrcMap) {
		const auto& [state, descs] = state_descs;
		auto& view = actives[rsrcNodeIdx];
		auto& typeinfo = typeinfoMap[rsrcNodeIdx];

		if (view.state != state) {
			cmdList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					view.pRsrc,
					view.state, state
				)
			);
			view.state = state;
		}

		auto pRsrc = actives[rsrcNodeIdx].pRsrc;
		for (const auto& desc : descs) {
			std::visit([&, rsrcNodeIdx = rsrcNodeIdx](const auto& desc) {
				using T = std::decay_t<decltype(desc)>;

				if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>)
				{
					auto& info = typeinfo.desc2info_cbv[desc];

					if (!info.init) {
						assert(desc.BufferLocation == static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(0)
							|| IsImported(rsrcNodeIdx) && desc.BufferLocation == view.pRsrc->GetGPUVirtualAddress());

						D3D12_CONSTANT_BUFFER_VIEW_DESC bindDesc = desc;
						bindDesc.BufferLocation = view.pRsrc->GetGPUVirtualAddress();

						device->CreateConstantBufferView(&bindDesc, info.cpuHandle);

						info.init = true;
					}
				}
				else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>
					|| std::is_same_v<T, RsrcImplDesc_SRV_NULL>)
				{
					RsrcTypeInfo::CpuGpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>)
						info = &typeinfo.desc2info_srv[desc];
					else // std::is_same_v<T, RsrcImplDesc_SRV_NULL>
						info = &typeinfo.null_info_srv;

					if (!info->init) {
						const D3D12_SHADER_RESOURCE_VIEW_DESC* pdesc;
						if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>)
							pdesc = &desc;
						else
							pdesc = nullptr;

						device->CreateShaderResourceView(view.pRsrc, pdesc, info->cpuHandle);

						info->init = true;
					}
				}
				else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>
					|| std::is_same_v<T, RsrcImplDesc_UAV_NULL>)
				{
					RsrcTypeInfo::CpuGpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>)
						info = &typeinfo.desc2info_uav[desc];
					else // std::is_same_v<T, RsrcImplDesc_UAV_NULL>
						info = &typeinfo.null_info_uav;

					if (!info->init) {
						const D3D12_UNORDERED_ACCESS_VIEW_DESC* pdesc;
						if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>)
							pdesc = &desc;
						else
							pdesc = nullptr;

						device->CreateUnorderedAccessView(view.pRsrc, nullptr, pdesc, info->cpuHandle);

						info->init = true;
					}
				}
				else if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>
					|| std::is_same_v<T, RsrcImplDesc_RTV_Null>)
				{
					RsrcTypeInfo::CpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>)
						info = &typeinfo.desc2info_rtv[desc];
					else // std::is_same_v<T, RsrcImplDesc_RTV_NULL>
						info = &typeinfo.null_info_rtv;

					if (!info->init) {
						const D3D12_RENDER_TARGET_VIEW_DESC* pdesc;
						if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>)
							pdesc = &desc;
						else
							pdesc = nullptr;

						device->CreateRenderTargetView(view.pRsrc, pdesc, info->cpuHandle);

						info->init = true;
					}
				}
				else if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>
					|| std::is_same_v<T, RsrcImplDesc_DSV_Null>)
				{
					RsrcTypeInfo::CpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>)
						info = &typeinfo.desc2info_dsv[desc];
					else // std::is_same_v<T, RsrcImplDesc_RTV_NULL>
						info = &typeinfo.null_info_dsv;

					if (!info->init) {
						const D3D12_DEPTH_STENCIL_VIEW_DESC* pdesc;
						if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>)
							pdesc = &desc;
						else
							pdesc = nullptr;

						device->CreateDepthStencilView(view.pRsrc, pdesc, info->cpuHandle);

						info->init = true;
					}
				}
				else
					static_assert(always_false_v<T>, "non-exhaustive visitor!");
			}, desc);
		}
		passRsrc.emplace(rsrcNodeIdx, RsrcImpl{ pRsrc, typeinfo });
	}
	return passRsrc;
}

bool RsrcMngr::CheckComplete(const UFG::FrameGraph& fg) {
	size_t rsrcNodeNum = fg.GetResourceNodes().size();
	size_t passNodeNum = fg.GetPassNodes().size();

	for (size_t i = 0; i < rsrcNodeNum; i++) {
		if (importeds.find(i) == importeds.end()
			&& temporals.find(i) == temporals.end())
			return false;
	}

	for (size_t i = 0; i < passNodeNum; i++) {
		auto target = passNodeIdx2rsrcMap.find(i);
		if (target == passNodeIdx2rsrcMap.end())
			return false;
		const auto& passNode = fg.GetPassNodes().at(i);
		for (auto rsrcNodeIdx : passNode.Inputs()) {
			if (target->second.find(rsrcNodeIdx) == target->second.end())
				return false;
		}
	}

	return true;
}
