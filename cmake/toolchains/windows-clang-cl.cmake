# Cross-compile Sunset Delay for 64-bit Windows with clang-cl.
# JUCE 9 rejects MinGW, so this toolchain uses the MSVC-compatible clang driver
# plus a Windows SDK + CRT laid out by xwin (https://github.com/Jake-Shadle/xwin).
#
#   xwin --accept-license splat --output /tmp/xwin-sdk
#   cmake -S . -B build-windows -G Ninja \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-clang-cl.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT DEFINED XWIN_DIR)
    set(XWIN_DIR "/tmp/xwin-sdk" CACHE PATH "xwin splat output (CRT + Windows SDK)")
endif()

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)

set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

set(_xwin_includes
    "/imsvc${XWIN_DIR}/crt/include"
    "/imsvc${XWIN_DIR}/sdk/include/ucrt"
    "/imsvc${XWIN_DIR}/sdk/include/um"
    "/imsvc${XWIN_DIR}/sdk/include/shared"
)
string(JOIN " " _xwin_include_flags ${_xwin_includes})

set(_xwin_libs
    "/libpath:${XWIN_DIR}/crt/lib/x86_64"
    "/libpath:${XWIN_DIR}/sdk/lib/ucrt/x86_64"
    "/libpath:${XWIN_DIR}/sdk/lib/um/x86_64"
)
string(JOIN " " _xwin_lib_flags ${_xwin_libs})

# Static CRT so the plugin does not need a separate VC redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
# The CRT xwin downloads may be newer than the distro Clang. MSVC's STL
# allows that combination when this macro is set.
set(_stl_mismatch "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH")
set(CMAKE_C_FLAGS_INIT "${_xwin_include_flags} /EHsc ${_stl_mismatch}")
set(CMAKE_CXX_FLAGS_INIT "${_xwin_include_flags} /EHsc /bigobj ${_stl_mismatch}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_xwin_lib_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_xwin_lib_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_xwin_lib_flags}")
set(CMAKE_RC_FLAGS "-I${XWIN_DIR}/crt/include -I${XWIN_DIR}/sdk/include/ucrt -I${XWIN_DIR}/sdk/include/um -I${XWIN_DIR}/sdk/include/shared")

set(CMAKE_FIND_ROOT_PATH "${XWIN_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
