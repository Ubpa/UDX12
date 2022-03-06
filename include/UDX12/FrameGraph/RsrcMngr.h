#pragma once

#include "Rsrc.h"

#include "../GCmdList.h"
#include "../Device.h"
#include "../DescriptorHeapMngr.h"

#include <unordered_set>

namespace Ubpa::UFG {
	class FrameGraph;
}

namespace Ubpa::UDX12::FG {
	// manage per pass resources
	// CBV, SRV, UAV, RTV, DSV
	class RsrcMngr {
	public:
		RsrcMngr(ID3D12Device* device);
		~RsrcMngr();

		// clear resources
		// you can call it when resize window
		void Clear();

		// clear per frame datas
		// call it per frame before registerations
		void NewFrame();

		// descriptor heap reserve
		// call by Ubpa::UDX12::FG::Executor
		void DHReserve();

		// allocate handles in descriptor heap for each resource nodes
		// call by Ubpa::UDX12::FG::Executor
		void AllocateHandle();

		// import, create or reuse buffer for the resource node
		// call by Ubpa::UDX12::FG::Executor
		void Construct(size_t rsrcNodeIdx);

		// recycle the buffer of the resource node
		// call by Ubpa::UDX12::FG::Executor
		void DestructCPU(size_t rsrcNodeIdx);
		void DestructGPU(ID3D12GraphicsCommandList*, size_t rsrcNodeIdx);

		// move the resource view of the source resource node to the destination resource node
		// call by Ubpa::UDX12::FG::Executor
		void Move(size_t dstRsrcNodeIdx, size_t srcRsrcNodeIdx);

		// - get the resource map (resource node index -> impl resource) of the pass node
		// - we will
		//   1. change buffer state
		//   2. init handle
		// - call by Ubpa::UDX12::FG::Executor
		PassRsrcs RequestPassRsrcs(ID3D12GraphicsCommandList*, size_t passNodeIdx);

		// mark the resource node as imported
		RsrcMngr& RegisterImportedRsrc(size_t rsrcNodeIdx, SRsrcView view) {
			importeds[rsrcNodeIdx] = view;
			return *this;
		}

		// mark the resource node as temporal
		RsrcMngr& RegisterTemporalRsrc(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC type);
		RsrcMngr& RegisterTemporalRsrcAutoClear(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC type);
		RsrcMngr& RegisterTemporalRsrc(size_t rsrcNodeIdx, D3D12_RESOURCE_DESC type, D3D12_CLEAR_VALUE clearvalue);
		RsrcMngr& RegisterTemporalRsrcReusable(size_t rsrcNodeIdx, bool reusable);
		RsrcMngr& RegisterTemporalRsrcConstructState(size_t rsrcNodeIdx, D3D12_RESOURCE_STATES state);

		// provide details for the resource node of the pass node
		RsrcMngr& RegisterPassRsrcState(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcState state);
		// a resource can have several descriptions
		RsrcMngr& RegisterPassRsrcImplDesc(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcImplDesc desc);
		// a helper function to simplify register pass resource's state and impl description
		RsrcMngr& RegisterPassRsrc(size_t passNodeIdx, size_t rsrcNodeIdx, RsrcState state, RsrcImplDesc desc);

		// provide external handles for the resource node
		RsrcMngr& RegisterRsrcHandle(
			size_t rsrcNodeIdx,
			RsrcImplDesc desc,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 },
			bool inited = true
		);

		// only support CBV, SRV, UAV
		RsrcMngr& RegisterRsrcTable(const std::string& ID, const std::vector<std::tuple<size_t, RsrcImplDesc>>& rsrcNodeIndices);

		RsrcMngr& RegisterCopyPassRsrcState(size_t passNodeIdx, size_t srcRsrcNodeIdx, size_t dstRsrcNodeIdx);

		RsrcMngr& RegisterCopyPassRsrcState(size_t passNodeIdx,
			std::span<const size_t> srcRsrcNodeIndices,
			std::span<const size_t> dstRsrcNodeIndices);

		RsrcMngr& RegisterCopyPassRsrcState(const UFG::FrameGraph& fg, size_t passNodeIdx);

		DXGI_FORMAT GetResourceFormat(size_t rsrcNodeIdx) const;

		// you should
		// 1. use RegisterImportedRsrc or RegisterTemporalRsrc to mark each resource nodes
		// 2. use RegisterPassRsrcs to mark each resource nodes for every passes
		// if you do it correct, then return true, else return false
		// TODO : move
		bool CheckComplete(const UFG::FrameGraph& fg);

	private:
		bool IsImported(size_t rsrcNodeIdx) const noexcept {
			return importeds.find(rsrcNodeIdx) != importeds.end();
		}

		// CSU : CBV, CSU, UAV

		void CSUDHReserve(UINT num);
		void DsvDHReserve(UINT num);
		void RtvDHReserve(UINT num);

		struct RsrcType {
			D3D12_RESOURCE_DESC desc;
			bool containClearvalue;
			D3D12_CLEAR_VALUE clearvalue;
			bool operator==(const RsrcType& rhs) const noexcept {
				return desc == rhs.desc && containClearvalue == rhs.containClearvalue
					&& (containClearvalue?clearvalue == rhs.clearvalue:true);
			}
		};
		struct RsrcTypeHasher {
			bool operator()(const RsrcType& rsrctype) const noexcept {
				return Ubpa::UDX12::FG::detail::hash_of(rsrctype);
			}
		};

		ID3D12Device* device;

		// type -> vector<view>
		std::unordered_map<Rsrc*, RsrcPtr> rsrcKeeper;
		std::unordered_map<RsrcType, std::vector<SRsrcView>, RsrcTypeHasher> pool;
		std::vector<std::pair<RsrcType, SRsrcView>> unreusableRsrcs;
		std::unordered_set<Rsrc*> usedRsrcs;

		// rsrcNodeIdx -> view
		std::unordered_map<size_t, SRsrcView> importeds;

		// rsrcNodeIdx -> type
		std::unordered_map<size_t, RsrcType> temporals;
		std::unordered_map<size_t, bool> temporalReusable;
		std::unordered_map<size_t, D3D12_RESOURCE_STATES> temporalConstructStates;

		// passNodeIdx -> resource (state + descs) map
		std::unordered_map<size_t, std::unordered_map<size_t, std::tuple<RsrcState, std::vector<RsrcImplDesc>>>> passNodeIdx2rsrcMap;

		// rsrcNodeIdx -> view
		std::unordered_map<size_t, SRsrcView> actives;

		// rsrcNodeIdx -> typeinfo
		std::unordered_map<size_t, RsrcDescInfo> typeinfoMap;
		
		UDX12::DynamicSuballocMngr* csuDynamicDH{ nullptr };

		UDX12::DescriptorHeapAllocation csuDH;
		std::vector<UINT> csuDHfree;
		std::unordered_set<UINT> csuDHused;

		UDX12::DescriptorHeapAllocation rtvDH;
		std::vector<UINT> rtvDHfree;
		std::unordered_set<UINT> rtvDHused;

		UDX12::DescriptorHeapAllocation dsvDH;
		std::vector<UINT> dsvDHfree;
		std::unordered_set<UINT> dsvDHused;
	};
}
