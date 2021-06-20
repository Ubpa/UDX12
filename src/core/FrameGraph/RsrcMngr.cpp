#include <UDX12/FrameGraph/RsrcMngr.h>

#include <UFG/FrameGraph.hpp>

#include <UDX12/_deps/DirectXTK12/DirectXHelpers.h>

using namespace Ubpa::UDX12::FG;
using namespace Ubpa::UDX12;
using namespace Ubpa;
using namespace std;

#include <DirectXColors.h>

RsrcMngr::RsrcMngr(ID3D12Device* device) : device{ device } {
	csuDynamicDH = new DynamicSuballocMngr{
		DescriptorHeapMngr::Instance().GetCSUGpuDH(),
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
	for (auto& [type, rsrcs] : pool) {
		rsrcs.erase(std::remove_if(rsrcs.begin(), rsrcs.end(), [&](const auto& rsrc) {
			if (!usedRsrcs.contains(rsrc.pRsrc)) {
				rsrcKeeper.erase(rsrc.pRsrc);
				return true;
			}
			else
				return false;
		}), rsrcs.end());
	}
	{ // clear empty type -> rsrc vector
		auto iter = pool.begin();
		while (iter != pool.end()) {
			if (iter->second.empty())
				iter = pool.erase(iter);
			else
				++iter;
		}
	}

	importeds.clear();
	temporals.clear();
	passNodeIdx2rsrcMap.clear();
	actives.clear();
	usedRsrcs.clear();

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
					// DSV
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

void RsrcMngr::Construct(size_t rsrcNodeIdx) {
	SRsrcView view;

	if (IsImported(rsrcNodeIdx))
		view = importeds.at(rsrcNodeIdx);
	else {
		const auto& type = temporals[rsrcNodeIdx];
		auto& frees = pool[type];
		if (frees.empty()) {
			view.state = D3D12_RESOURCE_STATE_COMMON;
			RsrcPtr ptr;
			const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&type.desc,
				view.state,
				type.contain_claervalue ? &type.clearvalue : nullptr,
				IID_PPV_ARGS(ptr.GetAddressOf())));
			rsrcKeeper.emplace(ptr.Get(), ptr);
			view.pRsrc = ptr.Get();
		}
		else {
			view = frees.back();
			frees.pop_back();
		}
		usedRsrcs.insert(view.pRsrc);
	}
	actives[rsrcNodeIdx] = view;
}

void RsrcMngr::DestructCPU(size_t rsrcNodeIdx) {
	auto view = actives.at(rsrcNodeIdx);
	if (!IsImported(rsrcNodeIdx))
		pool[temporals.at(rsrcNodeIdx)].push_back(view);
	/* // run in GPU timeline
	else {
		auto orig_state = importeds.at(rsrcNodeIdx).state;
		if (view.state != orig_state)
			DirectX::TransitionResource(cmdList, view.pRsrc, view.state, orig_state);
	}
	actives.erase(rsrcNodeIdx);
	*/
}

void RsrcMngr::DestructGPU(ID3D12GraphicsCommandList* cmdList, size_t rsrcNodeIdx) {
	auto view = actives.at(rsrcNodeIdx);
	if (IsImported(rsrcNodeIdx)) {
		auto orig_state = importeds.at(rsrcNodeIdx).state;
		if (view.state != orig_state)
			DirectX::TransitionResource(cmdList, view.pRsrc, view.state, orig_state);
	}
	actives.erase(rsrcNodeIdx);
}

void RsrcMngr::Move(size_t dstRsrcNodeIdx, size_t srcRsrcNodeIdx) {
	assert(dstRsrcNodeIdx != srcRsrcNodeIdx);
	assert(actives.find(dstRsrcNodeIdx) == actives.end());
	assert(actives.find(srcRsrcNodeIdx) != actives.end());

	actives.emplace(dstRsrcNodeIdx, actives.at(srcRsrcNodeIdx));
	actives.erase(srcRsrcNodeIdx);

	if (IsImported(srcRsrcNodeIdx)) {
		auto [iter, success] = importeds.emplace(dstRsrcNodeIdx, importeds.at(srcRsrcNodeIdx));
		assert(success);
		importeds.erase(srcRsrcNodeIdx);
	}
	else {
		auto [iter, success] = temporals.emplace(dstRsrcNodeIdx, temporals.at(srcRsrcNodeIdx));
		assert(success);
		temporals.erase(srcRsrcNodeIdx);
	}
}

RsrcMngr& RsrcMngr::RegisterTemporalRsrc(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC desc) {
	auto [iter, success] = temporals.emplace(rsrcNodeIdx, RsrcType{ desc, false });
	assert(success);
	return *this;
}

RsrcMngr& RsrcMngr::RegisterTemporalRsrcAutoClear(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC desc) {
	bool contain_clearvalue;
	D3D12_CLEAR_VALUE clearvalue;
	const float black[4] = { 0,0,0,1 };
	if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
		&& (desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0)
	{
		switch (desc.Format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC7_TYPELESS:
			contain_clearvalue = false;
			break;
		default:
			contain_clearvalue = true;
			if(desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
				clearvalue = CD3DX12_CLEAR_VALUE(desc.Format, 1.f, 0);
			else
				clearvalue = CD3DX12_CLEAR_VALUE(desc.Format, black);
			break;
		}
	}
	else
		contain_clearvalue = false;

	if (contain_clearvalue) {
		auto [iter, success] = temporals.emplace(rsrcNodeIdx, RsrcType{ desc, true, clearvalue });
		assert(success);
	}
	else {
		auto [iter, success] = temporals.emplace(rsrcNodeIdx, RsrcType{ desc, false });
		assert(success);
	}

	return *this;
}

RsrcMngr& RsrcMngr::RegisterTemporalRsrc(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC desc, D3D12_CLEAR_VALUE clearvalue) {
	auto [iter, success] = temporals.emplace(rsrcNodeIdx, RsrcType{ desc, true, clearvalue });
	assert(success);
	return *this;
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
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	bool inited)
{
	auto& typeinfo = typeinfoMap[rsrcNodeIdx];
	std::visit([&](const auto& desc) {
		using T = std::decay_t<decltype(desc)>;
		// CBV
		if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_cbv[desc] = { cpuHandle, gpuHandle, inited };
		}
		// SRV
		else if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_srv[desc] = { cpuHandle, gpuHandle, inited };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_SRV_NULL>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.null_info_srv = { cpuHandle, gpuHandle, inited };
		}
		// UAV
		else if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.desc2info_uav[desc] = { cpuHandle, gpuHandle, inited };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_UAV_NULL>) {
			assert(gpuHandle.ptr != 0);
			typeinfo.null_info_uav = { cpuHandle, gpuHandle, inited };
		}
		// RTV
		else if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.desc2info_rtv[desc] = { cpuHandle,inited };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_RTV_Null>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.null_info_rtv = { cpuHandle,inited };
		}
		// DSV
		else if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.desc2info_dsv[desc] = { cpuHandle,inited };
		}
		else if constexpr (std::is_same_v<T, RsrcImplDesc_DSV_Null>) {
			assert(gpuHandle.ptr == 0);
			typeinfo.null_info_dsv = { cpuHandle,inited };
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

PassRsrcs RsrcMngr::RequestPassRsrcs(ID3D12GraphicsCommandList* cmdList, size_t passNodeIdx) {
	PassRsrcs passRsrc;
	const auto& rsrcMap = passNodeIdx2rsrcMap.at(passNodeIdx);
	for (const auto& [rsrcNodeIdx, state_descs] : rsrcMap) {
		const auto& [state, descs] = state_descs;
		auto& view = actives.at(rsrcNodeIdx);
		auto& typeinfo = typeinfoMap.at(rsrcNodeIdx);

		if (view.state != state) {
			DirectX::TransitionResource(cmdList, view.pRsrc, view.state, state);
			view.state = state;
		}

		for (const auto& desc : descs) {
			std::visit([&, rsrcNodeIdx = rsrcNodeIdx](const auto& desc) {
				using T = std::decay_t<decltype(desc)>;

				if constexpr (std::is_same_v<T, D3D12_CONSTANT_BUFFER_VIEW_DESC>)
				{
					auto& info = typeinfo.desc2info_cbv.at(desc);

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
					RsrcDescInfo::CpuGpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_SHADER_RESOURCE_VIEW_DESC>)
						info = &typeinfo.desc2info_srv.at(desc);
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
					RsrcDescInfo::CpuGpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_UNORDERED_ACCESS_VIEW_DESC>)
						info = &typeinfo.desc2info_uav.at(desc);
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
					RsrcDescInfo::CpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_RENDER_TARGET_VIEW_DESC>)
						info = &typeinfo.desc2info_rtv.at(desc);
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
					RsrcDescInfo::CpuInfo* info;
					if constexpr (std::is_same_v<T, D3D12_DEPTH_STENCIL_VIEW_DESC>)
						info = &typeinfo.desc2info_dsv.at(desc);
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
		passRsrc.emplace(rsrcNodeIdx, RsrcImpl{ view.pRsrc, &typeinfo });
	}
	return passRsrc;
}

bool RsrcMngr::CheckComplete(const UFG::FrameGraph& fg) {
	size_t rsrcNodeNum = fg.GetResourceNodes().size();
	size_t passNodeNum = fg.GetPassNodes().size();

	for (size_t i = 0; i < rsrcNodeNum; i++) {
		if (!importeds.contains(i) && !temporals.contains(i) && !fg.IsMovedIn(i))
			return false;
	}

	for (size_t i = 0; i < passNodeNum; i++) {
		auto target = passNodeIdx2rsrcMap.find(i);
		if (target == passNodeIdx2rsrcMap.end())
			return false;
		const auto& rsrcMap = target->second;
		const auto& passNode = fg.GetPassNodes()[i];
		for (auto rsrcNodeIdx : passNode.Inputs()) {
			if (!rsrcMap.contains(rsrcNodeIdx))
				return false;
		}
		for (auto rsrcNodeIdx : passNode.Outputs()) {
			if (!rsrcMap.contains(rsrcNodeIdx))
				return false;
		}
	}

	return true;
}
