#include <UDX12/D3DInclude.h>

#include <fstream>
#include <filesystem>

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

HRESULT Ubpa::UDX12::DxcInclude::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	// Always set out parameter to NULL, validating it first.
	if (!ppvObj)
		return E_INVALIDARG;
	*ppvObj = NULL;
	if (riid == IID_IUnknown || riid == __uuidof(IDxcIncludeHandler) ||
		riid == __uuidof(DxcInclude))
	{
		// Increment the reference count and return the pointer.
		*ppvObj = (LPVOID)this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Ubpa::UDX12::DxcInclude::AddRef(void) {
	++m_cRef;
	return m_cRef;
}

ULONG STDMETHODCALLTYPE Ubpa::UDX12::DxcInclude::Release(void) {
	assert(m_cRef > 0);
	ULONG n = --m_cRef;
	if (0 == n)
		delete this;
	return n;
}

void Ubpa::UDX12::DxcInclude::Clear() {
	for (char* buffer : buffers)
		delete[] buffer;
	buffers.clear();
}

HRESULT __stdcall Ubpa::UDX12::DxcInclude::LoadSource(
	LPCWSTR pFilename,                                   // Candidate filename.
	IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
) {
	try {
		std::filesystem::path finalPath(dir);
		finalPath += LR"(\)";
		finalPath += std::filesystem::path(pFilename);

		std::ifstream includeFile(finalPath.string().c_str(), std::ios::in | std::ios::binary | std::ios::ate);

		if (includeFile.is_open()) {
			auto fileSize = includeFile.tellg();
			char* buf = new char[fileSize];
			includeFile.seekg(0, std::ios::beg);
			includeFile.read(buf, fileSize);
			includeFile.close();

			IDxcBlobEncoding* blob = nullptr;
			lib->CreateBlobWithEncodingFromPinned(buf, fileSize, 0, &blob);
			*ppIncludeSource = blob;
			buffers.push_back(buf);
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