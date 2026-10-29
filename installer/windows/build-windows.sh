#!/usr/bin/env bash
# Cross-compile Sunset Delay for 64-bit Windows and build the installer.
# Run this on Linux. On Windows, build with CMake + Visual Studio, then run build-windows.cmd.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD="${ROOT}/build-windows"
JUCE_SRC="${JUCE_DIR:-/tmp/JUCE}"
XWIN_DIR="${XWIN_DIR:-/tmp/xwin-sdk}"
JOBS="${JOBS:-4}"

if [[ ! -d "${XWIN_DIR}/crt/include" || ! -d "${XWIN_DIR}/sdk/include/um" ]]; then
    echo "Windows SDK not found at ${XWIN_DIR}." >&2
    echo "JUCE 9 does not build with MinGW. Fetch the SDK with xwin, then rerun:" >&2
    echo "  xwin --accept-license splat --output ${XWIN_DIR}" >&2
    exit 1
fi

# lld-link matches defaultlib names case-sensitively. xwin's splat covers most
# MSVC casings, but these four JUCE imports are still missing on a Linux fs.
UM_LIB="${XWIN_DIR}/sdk/lib/um/x86_64"
link_sdk_alias() {
    local want="$1"
    local alt
    [[ -e "${UM_LIB}/${want}" ]] && return 0
    for alt in "${@:2}"; do
        if [[ -e "${UM_LIB}/${alt}" ]]; then
            ln -sfn "${alt}" "${UM_LIB}/${want}"
            return 0
        fi
    done
    echo "Missing Windows import library for ${want} in ${UM_LIB}" >&2
    exit 1
}
link_sdk_alias DbgHelp.lib dbghelp.lib DbgHelp.Lib DBGHELP.lib
link_sdk_alias Dwrite.lib dwrite.lib DWRITE.lib
link_sdk_alias D2d1.lib d2d1.lib D2D1.lib
link_sdk_alias DComp.lib dcomp.lib DCOMP.lib

# JUCE builds juceaide for the host during configure. /usr/bin/c++ may be Clang
# and fail to link libstdc++, so prefer the host GCC for that helper only.
mkdir -p /tmp/host-cxx
ln -sfn /usr/bin/gcc /tmp/host-cxx/cc
ln -sfn /usr/bin/gcc /tmp/host-cxx/gcc
ln -sfn /usr/bin/g++ /tmp/host-cxx/c++
ln -sfn /usr/bin/g++ /tmp/host-cxx/g++
# clang-cl is the clang driver selected by argv[0]. A symlink is resolved away
# by CMake, so this is a real wrapper that keeps the name.
rm -f /tmp/host-cxx/clang-cl
cat > /tmp/host-cxx/clang-cl << 'EOF'
#!/bin/bash
exec -a clang-cl /usr/bin/clang-18 "$@"
EOF
chmod +x /tmp/host-cxx/clang-cl
# Ubuntu packages these as lld-link-18, llvm-lib-18, llvm-rc-18, llvm-mt-18.
link_host_tool() {
    local name="$1"
    if command -v "${name}" >/dev/null 2>&1; then
        return 0
    fi
    if [[ -x "/usr/bin/${name}-18" ]]; then
        ln -sfn "/usr/bin/${name}-18" "/tmp/host-cxx/${name}"
    fi
}
link_host_tool lld-link
link_host_tool llvm-lib
link_host_tool llvm-rc
link_host_tool llvm-mt
export PATH="/tmp/host-cxx:${PATH}"

CMAKE_ARGS=(
    -S "${ROOT}"
    -B "${BUILD}"
    -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/windows-clang-cl.cmake"
    -DXWIN_DIR="${XWIN_DIR}"
)
if [[ -d "${JUCE_SRC}/modules" ]]; then
    CMAKE_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_JUCE=${JUCE_SRC}")
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "${BUILD}" --target package_windows -j "${JOBS}"
