#pragma once

#include "Util.h"

namespace Ubpa::UDX12 {
	// ref: http://nikvoss.com/2013/03/implementing-id3dinclude/
	// TODO: multi system dir
	class D3DInclude : public ID3DInclude {
	public:
		D3DInclude(std::string localDir, std::string systemDir)
			: localDir(std::move(localDir)), systemDir(std::move(systemDir)) {}

		HRESULT __stdcall Open(
			D3D_INCLUDE_TYPE IncludeType,
			LPCSTR pFileName,
			LPCVOID pParentData,
			LPCVOID* ppData,
			UINT* pBytes
		);

		HRESULT __stdcall Close(LPCVOID pData);
	private:
		std::string localDir;
		std::string systemDir;
	};
}
