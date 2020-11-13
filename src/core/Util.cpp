#include <UDX12/Util.h>

#include <UDX12/_deps/DirectXTK12/DirectXTK12.h>

#include <UDX12/D3DInclude.h>

#include <comdef.h>

#include <stringapiset.h>

#include <fstream>
#include <sstream>

using namespace Ubpa::UDX12;
using namespace std;

wstring Util::AnsiToWString(const string& str)
{
    assert(str.size() < 512);
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return wstring(buffer);
}

Util::Exception::Exception(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

std::wstring Util::Exception::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

bool Util::IsKeyDown(int vkeyCode) {
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

std::string Util::HRstToString(HRESULT hr) {
    std::stringstream ss;
    ss << "0x" << std::hex << hr << std::endl;
    return ss.str();
}

Microsoft::WRL::ComPtr<ID3DBlob> Util::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Util::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // 创建 defualt buffer
    // Create the actual default buffer resource.
    const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // 为了拷贝 CPU 数据到 default buffer，我们需要创建一个中介的 upload heap
    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // 描述数据
    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // 拷贝数据：UpdateSubresources 会将数据拷贝到中介 upload heap，再拷贝到 default buffer
    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    DirectX::TransitionResource(cmdList, defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    DirectX::TransitionResource(cmdList, defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

    // uploadBuffer 不要立即释放，拷贝完成后再释放（cmdList 只是记录命令，并未执行）
    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.


    return defaultBuffer;
}

ComPtr<ID3DBlob> Util::CompileShaderFromFile(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());

    ThrowIfFailed(hr);

    return byteCode;
}

ComPtr<ID3DBlob> Util::CompileShader(
    std::string_view source,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
	const std::string& target,
	D3DInclude* pInclude
) {
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode;
	ComPtr<ID3DBlob> errors;
    hr = D3DCompile(source.data(), source.size(), nullptr, defines, pInclude,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

namespace Ubpa::UDX12::detail {
	inline uint32_t CountMips(uint32_t width, uint32_t height) noexcept
	{
		if (width == 0 || height == 0)
			return 0;

		uint32_t count = 1;
		while (width > 1 || height > 1)
		{
			width >>= 1;
			height >>= 1;
			count++;
		}
		return count;
	}
}

_Use_decl_annotations_
HRESULT Util::CreateTexture2DArrayFromMemory(ID3D12Device* device,
    DirectX::ResourceUploadBatch& resourceUpload,
    size_t width, size_t height, size_t arraySize,
    DXGI_FORMAT format,
    const D3D12_SUBRESOURCE_DATA* subResources,
    ID3D12Resource** texture,
    bool generateMips,
    D3D12_RESOURCE_STATES afterState,
    D3D12_RESOURCE_FLAGS resFlags) noexcept
{
	if (!texture)
		return E_INVALIDARG;

	*texture = nullptr;

	if (!device || !width || !height || !arraySize)
		return E_INVALIDARG;

	static_assert(D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION <= UINT16_MAX, "Exceeded integer limits");

	if ((width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
		|| (height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
		|| (arraySize > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
	{
		//DebugTrace("ERROR: Resource dimensions too large for DirectX 12 (3D: size %zu by %zu by %zu)\n", width, height, arraySize);
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	uint16_t mipCount = 1;
	if (generateMips)
	{
		generateMips = resourceUpload.IsSupportedForGenerateMips(format);
		if (generateMips)
		{
			mipCount = static_cast<uint16_t>(detail::CountMips(static_cast<uint32_t>(width), static_cast<uint32_t>(height)));
		}
	}

	auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, static_cast<UINT64>(width), static_cast<UINT>(height),
        static_cast<UINT16>(arraySize), mipCount, 1u, 0u, resFlags);

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

	ComPtr<ID3D12Resource> res;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_GRAPHICS_PPV_ARGS(res.GetAddressOf()));
	if (FAILED(hr))
		return hr;

	try
	{
		resourceUpload.Upload(res.Get(), 0, subResources, (UINT)arraySize);

		resourceUpload.Transition(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST, afterState);
	}
	/*catch (com_exception e)
	{
		return e.get_result();
	}*/
	catch (...)
	{
		return E_FAIL;
	}

	*texture = res.Detach();

	return S_OK;
}
