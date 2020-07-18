Ubpa_AddTarget(
  MODE STATIC
  RET_TARGET_NAME core
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/UDX12"
    "_deps/DirectXTK12/Shaders/Compiled/SpriteEffect_SpriteVertexShader.inc"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  INC_PRIVATE
    "${PROJECT_SOURCE_DIR}/include/UDX12/_deps/DirectXTK12"
  LIB
    Ubpa::UContainer_core
  LIB_INTERFACE
    d3dcompiler.lib
	D3D12.lib
	dxgi.lib
    dxguid.lib
  DEFINE
    UNICODE
	_UNICODE
    NOMINMAX
  L_OPTION_INTERFACE
    /SUBSYSTEM:WINDOWS
)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
  target_precompile_headers(${core} PRIVATE _deps/DirectXTK12/pch.h)
endif()

set(SHADER_SOURCES
  _deps/DirectXTK12/Shaders/AlphaTestEffect.fx
  _deps/DirectXTK12/Shaders/BasicEffect.fx
  _deps/DirectXTK12/Shaders/Common.fxh
  _deps/DirectXTK12/Shaders/DebugEffect.fx
  _deps/DirectXTK12/Shaders/DualTextureEffect.fx
  _deps/DirectXTK12/Shaders/EnvironmentMapEffect.fx
  _deps/DirectXTK12/Shaders/GenerateMips.hlsl
  _deps/DirectXTK12/Shaders/Lighting.fxh
  _deps/DirectXTK12/Shaders/NormalMapEffect.fx
  _deps/DirectXTK12/Shaders/PBRCommon.fxh
  _deps/DirectXTK12/Shaders/PBREffect.fx
  _deps/DirectXTK12/Shaders/PixelPacking_Velocity.hlsli
  _deps/DirectXTK12/Shaders/PostProcess.fx
  _deps/DirectXTK12/Shaders/RootSig.fxh
  _deps/DirectXTK12/Shaders/SkinnedEffect.fx
  _deps/DirectXTK12/Shaders/SpriteEffect.fx
  _deps/DirectXTK12/Shaders/Structures.fxh
  _deps/DirectXTK12/Shaders/ToneMap.fx
  _deps/DirectXTK12/Shaders/Utilities.fxh
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/_deps/DirectXTK12/Shaders/Compiled/SpriteEffect_SpriteVertexShader.inc"
  MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/_deps/DirectXTK12/Shaders/CompileShaders.cmd"
  DEPENDS ${SHADER_SOURCES}
  COMMENT "Generating HLSL shaders..."
  COMMAND "CompileShaders.cmd"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/_deps/DirectXTK12/Shaders"
  USES_TERMINAL
)
