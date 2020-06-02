Ubpa_GetTargetName(common "${PROJECT_SOURCE_DIR}/src/test/common")
Ubpa_AddTarget(
  TEST
  MODE EXE
  LIB ${common}
)
