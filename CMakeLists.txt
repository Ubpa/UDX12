cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(UDX12 VERSION 0.0.5)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake()

Ubpa_InitProject()

Ubpa_DownloadZip(
  https://ubpa.github.io/UData/UDX12/DirectXTK12/DirectXTK12_20200603.zip
  DirectXTK12.zip
  SHA256 7AD7FF6131B72D223945BA66AC610230DD492D39D58AF94E839CED5CC7CA3085
)

Ubpa_AddDep(UContainer 0.0.6)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(DIRECTORIES "include")
