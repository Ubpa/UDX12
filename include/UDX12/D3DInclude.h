#pragma once

#include "Util.h"
#include <fstream>
#include <filesystem>

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

	class __declspec(uuid("22cbd10c-cf07-4d0a-81d6-54654278f83b"))
	DxcInclude : public IDxcIncludeHandler {
	public:
		static DxcInclude* New(ComPtr<IDxcLibrary> lib, std::string dir) {
			return new DxcInclude(lib, std::move(dir));
		}

		virtual HRESULT __stdcall LoadSource(
			LPCWSTR pFilename,                                   // Candidate filename.
			IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		) override;

		HRESULT QueryInterface(REFIID riid, LPVOID* ppvObj) override;

		virtual ULONG STDMETHODCALLTYPE AddRef(void) override;

		virtual ULONG STDMETHODCALLTYPE Release(void) override;

		void Clear();
	private:
		DxcInclude(ComPtr<IDxcLibrary> lib, std::string dir) : lib{ lib }, dir(std::move(dir)) {}
		~DxcInclude() { Clear(); }

		std::atomic<ULONG> m_cRef{ 1 };
		ComPtr<IDxcLibrary> lib;
		std::string dir;
		std::vector<char*> buffers;
	};
}
