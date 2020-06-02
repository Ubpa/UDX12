Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/UDX12"
  INC
    "include"
  LIB_INTERFACE
    d3dcompiler.lib
	D3D12.lib
	dxgi.lib
    dxguid.lib
  DEFINE
    UNICODE
	_UNICODE
  L_OPTION_INTERFACE
    /SUBSYSTEM:WINDOWS
)
