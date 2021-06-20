#pragma once

#include "../Desc.h"

#include <variant>
#include <unordered_map>
#include <functional>
#include <UTemplate/Type.hpp>

namespace Ubpa::UDX12::FG::detail {
	template<typename T>
	void hash_combine(size_t& s, const T& v) {
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	template<typename T>
	size_t hash_of(const T& v) {
		using U =
			std::conditional_t<alignof(T) == 1,
				std::uint8_t,
				std::conditional_t<alignof(T) == 2,
					std::uint16_t,
					std::conditional_t<alignof(T) == 4,
						std::uint32_t,
						std::uint64_t
					>
				>
			>;

		size_t rst = Ubpa::TypeID_of<T>.GetValue();
		for (size_t i = 0; i < sizeof(T) / sizeof(U); i++)
			hash_combine(rst, reinterpret_cast<const U*>(&v)[i]);

		return rst;
	}

	template<typename T>
	struct always_false : std::false_type {};
	template<typename T>
	static constexpr bool always_false_v = always_false<T>::value;
	template<typename T>
	bool bitwise_equal(const T& lhs, const T& rhs) noexcept {
		using U =
			std::conditional_t<alignof(T) == 1,
				std::uint8_t,
				std::conditional_t<alignof(T) == 2,
					std::uint16_t,
					std::conditional_t<alignof(T) == 4,
						std::uint32_t,
						std::uint64_t
					>
				>
			>;

		const U* u_lhs = reinterpret_cast<const U*>(&lhs);
		const U* u_rhs = reinterpret_cast<const U*>(&rhs);
		for (size_t i = 0; i < sizeof(T) / sizeof(U); i++, u_lhs++, u_rhs++) {
			if (*u_lhs != *u_rhs)
				return false;
		}

		return true;
	}
}

inline bool operator==(const D3D12_CONSTANT_BUFFER_VIEW_DESC& lhs, const D3D12_CONSTANT_BUFFER_VIEW_DESC& rhs) noexcept {
	return lhs.BufferLocation == rhs.BufferLocation && lhs.SizeInBytes == rhs.SizeInBytes;
}

inline bool operator==(const D3D12_SHADER_RESOURCE_VIEW_DESC& lhs, const D3D12_SHADER_RESOURCE_VIEW_DESC& rhs) noexcept {
	if (lhs.Format == rhs.Format && lhs.ViewDimension == rhs.ViewDimension && lhs.Shader4ComponentMapping == rhs.Shader4ComponentMapping) {
		switch (lhs.ViewDimension)
		{
		case D3D12_SRV_DIMENSION_UNKNOWN:
			return true;
		case D3D12_SRV_DIMENSION_BUFFER:
			return lhs.Buffer.FirstElement == rhs.Buffer.FirstElement
				&& lhs.Buffer.Flags == rhs.Buffer.Flags
				&& lhs.Buffer.NumElements == rhs.Buffer.NumElements
				&& lhs.Buffer.StructureByteStride == rhs.Buffer.StructureByteStride;
		case D3D12_SRV_DIMENSION_TEXTURE1D:
			return lhs.Texture1D.MipLevels == rhs.Texture1D.MipLevels
				&& lhs.Texture1D.MostDetailedMip == rhs.Texture1D.MostDetailedMip
				&& lhs.Texture1D.ResourceMinLODClamp == rhs.Texture1D.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
			return lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize
				&& lhs.Texture1DArray.MipLevels == rhs.Texture1DArray.MipLevels
				&& lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice
				&& lhs.Texture1DArray.MostDetailedMip == rhs.Texture1DArray.MostDetailedMip
				&& lhs.Texture1DArray.ResourceMinLODClamp == rhs.Texture1DArray.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURE2D:
			return lhs.Texture2D.MipLevels == rhs.Texture2D.MipLevels
				&& lhs.Texture2D.MostDetailedMip == rhs.Texture2D.MostDetailedMip
				&& lhs.Texture2D.ResourceMinLODClamp == rhs.Texture2D.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
			return lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize
				&& lhs.Texture2DArray.MipLevels == rhs.Texture2DArray.MipLevels
				&& lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice
				&& lhs.Texture2DArray.MostDetailedMip == rhs.Texture2DArray.MostDetailedMip
				&& lhs.Texture2DArray.ResourceMinLODClamp == rhs.Texture2DArray.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURE2DMS:
			return true;
			//return lhs.Texture2DMS.UnusedField_NothingToDefine == rhs.Texture2DMS.UnusedField_NothingToDefine;
		case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return lhs.Texture2DMSArray.ArraySize == rhs.Texture2DMSArray.ArraySize
				&& lhs.Texture2DMSArray.FirstArraySlice == rhs.Texture2DMSArray.FirstArraySlice;
		case D3D12_SRV_DIMENSION_TEXTURE3D:
			return lhs.Texture3D.MipLevels == rhs.Texture3D.MipLevels
				&& lhs.Texture3D.MostDetailedMip == rhs.Texture3D.MostDetailedMip
				&& lhs.Texture3D.ResourceMinLODClamp == rhs.Texture3D.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURECUBE:
			return lhs.TextureCube.MipLevels == rhs.TextureCube.MipLevels
				&& lhs.TextureCube.MostDetailedMip == rhs.TextureCube.MostDetailedMip
				&& lhs.TextureCube.ResourceMinLODClamp == rhs.TextureCube.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
			return lhs.TextureCubeArray.First2DArrayFace == rhs.TextureCubeArray.First2DArrayFace
				&& lhs.TextureCubeArray.MipLevels == rhs.TextureCubeArray.MipLevels
				&& lhs.TextureCubeArray.MostDetailedMip == rhs.TextureCubeArray.MostDetailedMip
				&& lhs.TextureCubeArray.NumCubes == rhs.TextureCubeArray.NumCubes
				&& lhs.TextureCubeArray.ResourceMinLODClamp == rhs.TextureCubeArray.ResourceMinLODClamp;
		case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
			return lhs.RaytracingAccelerationStructure.Location == rhs.RaytracingAccelerationStructure.Location;
		default:
			assert(false);
			return false;
		}
	}
	else
		return false;
}

inline bool operator==(const D3D12_UNORDERED_ACCESS_VIEW_DESC& lhs, const D3D12_UNORDERED_ACCESS_VIEW_DESC& rhs) noexcept {
	if (lhs.Format == rhs.Format && lhs.ViewDimension == rhs.ViewDimension) {
		switch (lhs.ViewDimension)
		{
		case D3D12_UAV_DIMENSION_UNKNOWN:
			return true;
		case D3D12_UAV_DIMENSION_BUFFER:
			return lhs.Buffer.CounterOffsetInBytes == rhs.Buffer.CounterOffsetInBytes
				&& lhs.Buffer.FirstElement == rhs.Buffer.FirstElement
				&& lhs.Buffer.Flags == rhs.Buffer.Flags
				&& lhs.Buffer.NumElements == rhs.Buffer.NumElements
				&& lhs.Buffer.StructureByteStride == rhs.Buffer.StructureByteStride;
		case D3D12_UAV_DIMENSION_TEXTURE1D:
			return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;
		case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
			return lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize
				&& lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice
				&& lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice;
		case D3D12_UAV_DIMENSION_TEXTURE2D:
			return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice
				&& lhs.Texture2D.PlaneSlice == rhs.Texture2D.PlaneSlice;
		case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
			return lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize
				&& lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice
				&& lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice
				&& lhs.Texture2DArray.PlaneSlice == rhs.Texture2DArray.PlaneSlice;
		case D3D12_UAV_DIMENSION_TEXTURE3D:
			return lhs.Texture3D.FirstWSlice == rhs.Texture3D.FirstWSlice
				&& lhs.Texture3D.MipSlice == rhs.Texture3D.MipSlice
				&& lhs.Texture3D.WSize == rhs.Texture3D.WSize;
		default:
			assert(false);
			return false;
		}
	}
	else
		return false;
}

inline bool operator==(const D3D12_RENDER_TARGET_VIEW_DESC& lhs, const D3D12_RENDER_TARGET_VIEW_DESC& rhs) noexcept {
	if (lhs.Format == rhs.Format && lhs.ViewDimension == rhs.ViewDimension) {
		switch (lhs.ViewDimension)
		{
		case D3D12_RTV_DIMENSION_UNKNOWN:
			return true;
		case D3D12_RTV_DIMENSION_BUFFER:
			return lhs.Buffer.FirstElement == rhs.Buffer.FirstElement
				&& lhs.Buffer.NumElements == rhs.Buffer.NumElements;
		case D3D12_RTV_DIMENSION_TEXTURE1D:
			return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;
		case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
			return lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize
				&& lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice
				&& lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice;
		case D3D12_RTV_DIMENSION_TEXTURE2D:
			return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice
				&& lhs.Texture2D.PlaneSlice == rhs.Texture2D.PlaneSlice;
		case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
			return lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize
				&& lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice
				&& lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice
				&& lhs.Texture2DArray.PlaneSlice == rhs.Texture2DArray.PlaneSlice;
		case D3D12_RTV_DIMENSION_TEXTURE2DMS:
			return true;
			//return lhs.Texture2DMS.UnusedField_NothingToDefine == rhs.Texture2DMS.UnusedField_NothingToDefine;
		case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
			return lhs.Texture2DMSArray.ArraySize == rhs.Texture2DMSArray.ArraySize
				&& lhs.Texture2DMSArray.FirstArraySlice == rhs.Texture2DMSArray.FirstArraySlice;
		case D3D12_RTV_DIMENSION_TEXTURE3D:
			return lhs.Texture3D.FirstWSlice == rhs.Texture3D.FirstWSlice
				&& lhs.Texture3D.MipSlice == rhs.Texture3D.MipSlice
				&& lhs.Texture3D.WSize == rhs.Texture3D.WSize;
		default:
			assert(false);
			return false;
		}
	}
	else
		return false;
}

inline bool operator==(const D3D12_DEPTH_STENCIL_VIEW_DESC& lhs, const D3D12_DEPTH_STENCIL_VIEW_DESC& rhs) noexcept {
	if (lhs.Format == rhs.Format && lhs.ViewDimension == rhs.ViewDimension && lhs.Flags == rhs.Flags) {
		switch (lhs.ViewDimension)
		{
		case D3D12_DSV_DIMENSION_UNKNOWN:
			return true;
		case D3D12_DSV_DIMENSION_TEXTURE1D:
			return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;
		case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
			return lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize
				&& lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice
				&& lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice;
		case D3D12_DSV_DIMENSION_TEXTURE2D:
			return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice;
		case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
			return lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize
				&& lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice
				&& lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice;
		case D3D12_DSV_DIMENSION_TEXTURE2DMS:
			return true;
			//return lhs.Texture2DMS.UnusedField_NothingToDefine == rhs.Texture2DMS.UnusedField_NothingToDefine;
		case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
			return lhs.Texture2DMSArray.ArraySize == rhs.Texture2DMSArray.ArraySize
				&& lhs.Texture2DMSArray.FirstArraySlice == rhs.Texture2DMSArray.FirstArraySlice;
		default:
			assert(false);
			return false;
		}
	}
	else
		return false;
}

namespace Ubpa::UDX12::FG {
	using Rsrc = ID3D12Resource;
	using RsrcPtr = ComPtr<Rsrc>;
	using RsrcState = D3D12_RESOURCE_STATES;
	struct RsrcImplDesc_SRV_NULL {};
	struct RsrcImplDesc_UAV_NULL {};
	struct RsrcImplDesc_RTV_Null {};
	struct RsrcImplDesc_DSV_Null {};
	using RsrcImplDesc = std::variant<
		D3D12_CONSTANT_BUFFER_VIEW_DESC,
		D3D12_SHADER_RESOURCE_VIEW_DESC,
		D3D12_UNORDERED_ACCESS_VIEW_DESC,
		D3D12_RENDER_TARGET_VIEW_DESC,
		D3D12_DEPTH_STENCIL_VIEW_DESC,
		RsrcImplDesc_SRV_NULL,
		RsrcImplDesc_UAV_NULL,
		RsrcImplDesc_RTV_Null,
		RsrcImplDesc_DSV_Null>;
	struct SRsrcView {
		Rsrc* pRsrc;
		RsrcState state;
	};
}

namespace std {
	template<>
	struct hash<D3D12_RESOURCE_DESC> {
		size_t operator()(const D3D12_RESOURCE_DESC type) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_RESOURCE_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Dimension);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Alignment);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Width);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Height);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.DepthOrArraySize);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.MipLevels);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Format);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.SampleDesc.Count);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.SampleDesc.Quality);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Layout);
			Ubpa::UDX12::FG::detail::hash_combine(rst, type.Flags);
			return rst;
		}
	};

	template<>
	struct hash<D3D12_CONSTANT_BUFFER_VIEW_DESC> {
		size_t operator()(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_CONSTANT_BUFFER_VIEW_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.BufferLocation);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.SizeInBytes);
			return rst;
		}
	};

	template<>
	struct hash<D3D12_SHADER_RESOURCE_VIEW_DESC> {
		size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_SHADER_RESOURCE_VIEW_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Format);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.ViewDimension);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Shader4ComponentMapping);
			switch (desc.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_UNKNOWN:
				break;
			case D3D12_SRV_DIMENSION_BUFFER:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.FirstElement);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.Flags);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.NumElements);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.StructureByteStride);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.PlaneSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.PlaneSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMS:
				//Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMS.UnusedField_NothingToDefine);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.FirstArraySlice);
				break;
			case D3D12_SRV_DIMENSION_TEXTURE3D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCube.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCube.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCube.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCubeArray.First2DArrayFace);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCubeArray.MipLevels);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCubeArray.MostDetailedMip);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCubeArray.NumCubes);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.TextureCubeArray.ResourceMinLODClamp);
				break;
			case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.RaytracingAccelerationStructure.Location);
				break;
			default:
				assert(false);
				break;
			}
			return rst;
		}
	};

	template<>
	struct hash<D3D12_UNORDERED_ACCESS_VIEW_DESC> {
		size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_UNORDERED_ACCESS_VIEW_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Format);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.ViewDimension);
			switch (desc.ViewDimension)
			{
			case D3D12_UAV_DIMENSION_UNKNOWN:
				break;
			case D3D12_UAV_DIMENSION_BUFFER:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.CounterOffsetInBytes);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.FirstElement);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.Flags);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.NumElements);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.StructureByteStride);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.MipSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.MipSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.MipSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.MipSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.PlaneSlice);
				break;
			case D3D12_UAV_DIMENSION_TEXTURE3D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.FirstWSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.FirstWSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.WSize);
				break;
			default:
				assert(false);
				break;
			}
			return rst;
		}
	};

	template<>
	struct hash<D3D12_RENDER_TARGET_VIEW_DESC> {
		size_t operator()(const D3D12_RENDER_TARGET_VIEW_DESC& desc) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_RENDER_TARGET_VIEW_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Format);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.ViewDimension);
			switch (desc.ViewDimension)
			{
			case D3D12_RTV_DIMENSION_UNKNOWN:
				break;
			case D3D12_RTV_DIMENSION_BUFFER:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.FirstElement);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Buffer.NumElements);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE1D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.MipSlice);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.MipSlice);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE2D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.MipSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.PlaneSlice);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.MipSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.PlaneSlice);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE2DMS:
				//Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMS.UnusedField_NothingToDefine);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.FirstArraySlice);
				break;
			case D3D12_RTV_DIMENSION_TEXTURE3D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.FirstWSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.MipSlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture3D.WSize);
				break;
			default:
				assert(false);
				break;
			}
			return rst;
		}
	};

	template<>
	struct hash<D3D12_DEPTH_STENCIL_VIEW_DESC> {
		size_t operator()(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc) const noexcept {
			std::size_t rst = Ubpa::TypeID_of<D3D12_DEPTH_STENCIL_DESC>.GetValue();
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Format);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.ViewDimension);
			Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Flags);
			switch (desc.ViewDimension)
			{
			case D3D12_DSV_DIMENSION_UNKNOWN:
				break;
			case D3D12_DSV_DIMENSION_TEXTURE1D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1D.MipSlice);
				break;
			case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture1DArray.MipSlice);
				break;
			case D3D12_DSV_DIMENSION_TEXTURE2D:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2D.MipSlice);
				break;
			case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.FirstArraySlice);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DArray.MipSlice);
				break;
			case D3D12_DSV_DIMENSION_TEXTURE2DMS:
				//Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMS.UnusedField_NothingToDefine);
				break;
			case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.ArraySize);
				Ubpa::UDX12::FG::detail::hash_combine(rst, desc.Texture2DMSArray.FirstArraySlice);
				break;
			default:
				assert(false);
			}
			return rst;
		}
	};
}

namespace Ubpa::UDX12::FG {
	struct RsrcDescInfo {
		struct CpuGpuInfo {
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ 0 };
			bool init{ false };
		};

		struct CpuInfo {
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
			bool init{ false };
		};

		std::unordered_map<D3D12_CONSTANT_BUFFER_VIEW_DESC, CpuGpuInfo>  desc2info_cbv;
		std::unordered_map<D3D12_SHADER_RESOURCE_VIEW_DESC, CpuGpuInfo>  desc2info_srv;
		std::unordered_map<D3D12_UNORDERED_ACCESS_VIEW_DESC, CpuGpuInfo> desc2info_uav;

		std::unordered_map<D3D12_RENDER_TARGET_VIEW_DESC, CpuInfo>       desc2info_rtv;
		std::unordered_map<D3D12_DEPTH_STENCIL_VIEW_DESC, CpuInfo>       desc2info_dsv;

		CpuGpuInfo null_info_srv;
		CpuGpuInfo null_info_uav;

		CpuInfo    null_info_dsv;
		CpuInfo    null_info_rtv;

		bool HaveNullSrv() const { return null_info_srv.cpuHandle.ptr != 0; }
		bool HaveNullUav() const { return null_info_uav.cpuHandle.ptr != 0; }
		bool HaveNullDsv() const { return null_info_dsv.cpuHandle.ptr != 0; }
		bool HaveNullRtv() const { return null_info_rtv.cpuHandle.ptr != 0; }
	};
	struct RsrcImpl {
		ID3D12Resource* resource;
		const RsrcDescInfo* info;
	};
	using PassRsrcs = std::unordered_map<size_t, RsrcImpl>;
}
