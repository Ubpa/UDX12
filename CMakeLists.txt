cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(UDX12 VERSION 0.1.0)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake(VERSION 0.6.4)

Ubpa_InitProject()

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/UDX12/DirectXTK12/DirectXTK12_20200603.zip
  DirectXTK12.zip
  SHA256 7AD7FF6131B72D223945BA66AC610230DD492D39D58AF94E839CED5CC7CA3085
)

Ubpa_AddDep(UTemplate 0.7.2)
Ubpa_AddDep(UFG 0.4.0)
Ubpa_AddDep(UFunction 0.1.0)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(
  TARGET
  DIRECTORIES
    "include"
)
