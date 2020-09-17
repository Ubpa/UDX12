#pragma once

#include <UTemplate/Func.h>

namespace Ubpa::UDX12 {
	template<typename T>
	T& FrameResource::GetResource(std::string_view name) {
		static_assert(!std::is_reference_v<T>);
		assert(HaveResource(name));
		return std::any_cast<T&>(resourceMap.find(name)->second);
	}

	template<typename T>
	const T& FrameResource::GetResource(std::string_view name) const {
		return const_cast<FrameResource*>(this)->GetResource<T>();
	}

	template<typename T>
	FrameResource& FrameResource::RegisterResource(std::string name, T&& resource) {
		assert(!HaveResource(name));
		resourceMap.emplace(std::move(name), std::any{ std::forward<T>(resource) });
		return *this;
	}

	template<typename Func>
	FrameResource& FrameResource::DelayUpdateResource(std::string name, Func&& updator) {
		using ArgList = FuncTraits_ArgList<std::decay_t<Func>>;
		static_assert(Length_v<ArgList> == 1);
		using Arg = Front_t<ArgList>;
		delayUpdatorMap.emplace_back(std::tuple{ std::move(name), std::function{
			[updator = std::forward<Func>(updator)] (std::any& resource) mutable {
				updator(std::any_cast<Arg>(resource));
			}
		}});
		return *this;
	}
}
