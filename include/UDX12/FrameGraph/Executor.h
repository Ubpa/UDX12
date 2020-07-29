#pragma once

#include "Rsrc.h"

#include <UFG/Compiler.h>

#include <functional>

namespace Ubpa::UDX12::FG {
	class RsrcMngr;

	class Executor {
	public:
		Executor& RegisterPassFunc(size_t passNodeIdx, std::function<void(const PassRsrcs&)> func) {
			passFuncs[passNodeIdx] = std::move(func);
			return *this;
		}

		void NewFrame() {
			passFuncs.clear();
		}

		void Execute(const UFG::Compiler::Result& crst, RsrcMngr& rsrcMngr);

	private:
		std::unordered_map<size_t, std::function<void(const PassRsrcs&)>> passFuncs;
	};
}
