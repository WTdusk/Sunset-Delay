#!/usr/bin/env bash
# Stage a Windows Sunset Delay build, compile the installer, and zip the VST3.
# Prefers Inno Setup (ISCC). Falls back to NSIS (makensis), which runs on Linux.
# Refuses a bundle whose moduleinfo still lists an Instrument subcategory.
set -euo pipefail

SOURCE=${GUITAR_DELAY_SOURCE:?Set GUITAR_DELAY_SOURCE to the repository root}
BUILD=${GUITAR_DELAY_BUILD:?Set GUITAR_DELAY_BUILD to the Windows CMake build directory}
STAGE="${BUILD}/windows-stage"
DIST="${BUILD}/dist"
ISS="${SOURCE}/installer/windows/GuitarDelay.iss"
NSI="${SOURCE}/installer/windows/GuitarDelay.nsi"
VERSION=$(sed -n 's/^project(GuitarDelay VERSION \([0-9.]*\).*/\1/p' "${SOURCE}/CMakeLists.txt" | head -1)
VERSION=${VERSION:-1.2.0}
SETUP_NAME="SunsetDelay-Setup-${VERSION}.exe"
VST3_ZIP="SunsetDelay-VST3-${VERSION}.zip"

rm -rf "${STAGE}"
mkdir -p "${STAGE}" "${DIST}"

VST3=$(find "${BUILD}" -type d -name 'Sunset Delay.vst3' -path '*VST3*' | head -1 || true)
if [[ -z "${VST3}" ]]; then
    echo "No VST3 bundle under ${BUILD}. Build GuitarDelay_VST3 first." >&2
    exit 1
fi

BIN=$(find "${VST3}" -type f -name 'Sunset Delay.vst3' | head -1 || true)
if [[ -z "${BIN}" ]]; then
    echo "VST3 bundle has no binary: ${VST3}" >&2
    exit 1
fi

if ! file "${BIN}" | grep -q 'PE32+'; then
    echo "This build is not a Windows plugin (${BIN})." >&2
    echo "Configure with cmake/toolchains/windows-clang-cl.cmake, or build on Windows." >&2
    file "${BIN}" >&2
    exit 1
fi

echo "Staging ${VST3}"
cp -a "${VST3}" "${STAGE}/Sunset Delay.vst3"

if [[ ! -f "${STAGE}/Sunset Delay.vst3/Contents/Resources/moduleinfo.json" ]]; then
    cmake -DBUNDLE_DIR="${STAGE}/Sunset Delay.vst3" -DVERSION="${VERSION}" \
        -P "${SOURCE}/installer/windows/write_moduleinfo.cmake"
fi

EXE=$(find "${BUILD}" -type f -path '*Standalone/Sunset Delay.exe' | head -1 || true)
if [[ -n "${EXE}" ]]; then
    if file "${EXE}" | grep -q 'PE32+'; then
        cp -a "${EXE}" "${STAGE}/Sunset Delay.exe"
    else
        echo "Skipping non-Windows standalone: ${EXE}" >&2
    fi
fi

INFO_JSON="${STAGE}/Sunset Delay.vst3/Contents/Resources/moduleinfo.json"
if [[ ! -f "${INFO_JSON}" ]]; then
    echo "Staged VST3 is missing moduleinfo.json" >&2
    exit 1
fi
if grep -q 'Instrument' "${INFO_JSON}"; then
    echo "VST3 moduleinfo must be Fx|Delay. An Instrument subcategory silences DAW inserts." >&2
    exit 1
fi
if ! grep -q '"Fx"' "${INFO_JSON}" || ! grep -q '"Delay"' "${INFO_JSON}"; then
    echo "VST3 moduleinfo is missing the Fx and Delay subcategories." >&2
    exit 1
fi
if strings "${BIN}" | grep -q 'Fx|Instrument|Delay'; then
    echo "Plugin binary still advertises Fx|Instrument|Delay." >&2
    exit 1
fi

echo "Windows imports:"
if command -v x86_64-w64-mingw32-objdump >/dev/null 2>&1; then
    x86_64-w64-mingw32-objdump -p "${BIN}" | awk '/DLL Name/{print}'
    if x86_64-w64-mingw32-objdump -p "${BIN}" | awk '/DLL Name/{print $3}' | grep -Eiq 'libstdc|libgcc|libwinpthread|libssp'; then
        echo "The VST3 still depends on a MinGW runtime DLL. Refusing to pack it." >&2
        exit 1
    fi
fi

pack_inno() {
    local iscc="$1"
    local payload_dir="$2"
    local output_dir="$3"
    local iss_path="$4"
    if [[ "${iscc}" == *.exe ]]; then
        payload_dir=$(winepath -w "${payload_dir}")
        output_dir=$(winepath -w "${output_dir}")
        iss_path=$(winepath -w "${iss_path}")
        wine "${iscc}" "/DPayloadDir=${payload_dir}" "/DOutputDir=${output_dir}" "${iss_path}"
    else
        "${iscc}" "/DPayloadDir=${payload_dir}" "/DOutputDir=${output_dir}" "${iss_path}"
    fi
}

ISCC=""
if command -v iscc >/dev/null 2>&1; then
    ISCC=$(command -v iscc)
else
    for candidate in \
        "${HOME}/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
        "${HOME}/.wine/drive_c/Program Files/Inno Setup 6/ISCC.exe" \
        /opt/innosetup/ISCC.exe
    do
        if [[ -f "${candidate}" ]]; then
            ISCC="${candidate}"
            break
        fi
    done
fi

if [[ -n "${ISCC}" ]]; then
    echo "Compiling Inno Setup installer with ${ISCC}"
    pack_inno "${ISCC}" "${STAGE}" "${DIST}" "${ISS}"
elif command -v makensis >/dev/null 2>&1; then
    echo "Inno Setup compiler not found. Compiling the NSIS installer instead."
    makensis -INPUTCHARSET UTF8 \
        -DPAYLOAD="${STAGE}" \
        -DOUTFILE="${DIST}/${SETUP_NAME}" \
        "${NSI}"
else
    echo "Neither ISCC nor makensis is available." >&2
    exit 1
fi

if [[ ! -f "${DIST}/${SETUP_NAME}" ]]; then
    found=$(find "${DIST}" -maxdepth 1 -type f -name 'SunsetDelay-Setup*.exe' | head -1 || true)
    if [[ -n "${found}" && "${found}" != "${DIST}/${SETUP_NAME}" ]]; then
        mv "${found}" "${DIST}/${SETUP_NAME}"
    fi
fi

if [[ ! -f "${DIST}/${SETUP_NAME}" ]]; then
    echo "Installer was not written to ${DIST}/${SETUP_NAME}" >&2
    ls -la "${DIST}" >&2 || true
    exit 1
fi

rm -f "${DIST}/${VST3_ZIP}"
( cd "${STAGE}" && zip -r -X "${DIST}/${VST3_ZIP}" "Sunset Delay.vst3" )
if ! unzip -l "${DIST}/${VST3_ZIP}" | grep -q 'Sunset Delay.vst3/Contents/'; then
    echo "VST3 zip is missing the Sunset Delay.vst3 bundle." >&2
    exit 1
fi

SUM_NAME="SunsetDelay-${VERSION}.sha256"
(
    cd "${DIST}"
    sha256sum "${SETUP_NAME}" "${VST3_ZIP}" > "${SUM_NAME}"
)

echo "Installer: ${DIST}/${SETUP_NAME}"
echo "VST3 zip: ${DIST}/${VST3_ZIP}"
echo "Checksums: ${DIST}/${SUM_NAME}"
if [[ -d /opt/cursor/artifacts ]]; then
    cp -f "${DIST}/${SETUP_NAME}" "/opt/cursor/artifacts/${SETUP_NAME}"
    cp -f "${DIST}/${VST3_ZIP}" "/opt/cursor/artifacts/${VST3_ZIP}"
    cp -f "${DIST}/${SUM_NAME}" "/opt/cursor/artifacts/${SUM_NAME}"
    echo "Copied to /opt/cursor/artifacts/${SETUP_NAME}"
    echo "Copied to /opt/cursor/artifacts/${VST3_ZIP}"
    echo "Copied to /opt/cursor/artifacts/${SUM_NAME}"
fi
