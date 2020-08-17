#pragma once

#include "DescriptorHeap/CPUDescriptorHeap.h"
#include "DescriptorHeap/DescriptorHeapAllocation.h"
#include "DescriptorHeap/DescriptorHeapAllocMngr.h"
#include "DescriptorHeap/DynamicSuballocMngr.h"
#include "DescriptorHeap/GPUDescriptorHeap.h"
#include "DescriptorHeap/IDescriptorAllocator.h"

namespace Ubpa::UDX12 {
	class DescriptorHeapMngr {
	public:
		static DescriptorHeapMngr& Instance() noexcept {
			static DescriptorHeapMngr instance;
			return instance;
		}

		void Init(
			ID3D12Device* device,
			uint32_t numCpuCSU,
			uint32_t numCpuRTV,
			uint32_t numCpuDSV,
			uint32_t numGpuCSU_static,
			uint32_t numGpuCSU_dynamic
		);

		CPUDescriptorHeap* GetCSUCpuDH() noexcept { assert(isInit); return CSU_CpuDH; }
		CPUDescriptorHeap* GetRTVCpuDH() noexcept { assert(isInit); return RTV_CpuDH; }
		CPUDescriptorHeap* GetDSVCpuDH() noexcept { assert(isInit); return DSV_CpuDH; }
		GPUDescriptorHeap* GetCSUGpuDH() noexcept { assert(isInit); return CSU_GpuDH; }

		void Clear();

	private:
		DescriptorHeapMngr() = default;
		~DescriptorHeapMngr();

		bool isInit{ false };
		CPUDescriptorHeap* CSU_CpuDH{ nullptr };
		CPUDescriptorHeap* RTV_CpuDH{ nullptr };
		CPUDescriptorHeap* DSV_CpuDH{ nullptr };
		GPUDescriptorHeap* CSU_GpuDH{ nullptr };
	};
}
