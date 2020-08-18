#include <UDX12/D3DInclude.h>

#include <fstream>

HRESULT __stdcall Ubpa::UDX12::D3DInclude::Open(
	D3D_INCLUDE_TYPE IncludeType,
	LPCSTR pFileName,
	LPCVOID pParentData,
	LPCVOID* ppData,
	UINT* pBytes
) {
	try {
		std::string finalPath;
		switch (IncludeType) {
		case D3D_INCLUDE_LOCAL:
			finalPath = localDir + "/" + pFileName;
				break;
		case D3D_INCLUDE_SYSTEM:
			finalPath = systemDir + "/" + pFileName;
				break;
		default:
			assert(0);
		}

		std::ifstream includeFile(finalPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

		if (includeFile.is_open()) {
			auto fileSize = includeFile.tellg();
			char* buf = new char[fileSize];
			includeFile.seekg(0, std::ios::beg);
			includeFile.read(buf, fileSize);
			includeFile.close();
			*ppData = buf;
			*pBytes = static_cast<UINT>(fileSize);
		}
		else {
			return E_FAIL;
		}
		return S_OK;
	}
	catch (...) {
		return E_FAIL;
	}
}

HRESULT __stdcall Ubpa::UDX12::D3DInclude::Close(LPCVOID pData) {
	char* buf = (char*)pData;
	delete[] buf;
	return S_OK;
}
