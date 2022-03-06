cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(UDX12 VERSION 0.2.1)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake(VERSION 0.6.4)

Ubpa_InitProject()

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/DirectXTK12_20200603.zip
  DirectXTK12.zip
  SHA256 7AD7FF6131B72D223945BA66AC610230DD492D39D58AF94E839CED5CC7CA3085
)

Ubpa_DownloadZip_Pro(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/DXCompiler_20210512.zip
  DXCompiler_20210512.zip
  ${CMAKE_CURRENT_SOURCE_DIR}/bin
  SHA256 86FC3DBD3086A0B2C5B0D1B9A79BCDE5D2D302FCA6441559BA3145FF5922EA6E
)

Ubpa_AddDep(UTemplate 0.7.2)
Ubpa_AddDep(UFG 0.6.0)
Ubpa_AddDep(UThreadPool 0.1.0)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(
  TARGET
  DIRECTORIES
    "include"
)

install(
  FILES
    "bin/dxcompiler.dll"
    "bin/dxil.dll"
  DESTINATION "bin"
)
