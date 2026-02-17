#!/bin/bash
# build-windows.sh — Build Windows binary from WSL using MSVC
#
# Both sources and build output are mirrored to the Windows filesystem
# to avoid the slow WSL 9p filesystem bridge during compilation.
# The MSVC environment is cached to avoid running vcvarsall.bat every time.
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONFIG="Release"

# Parse arguments
for arg in "$@"; do
    case "$arg" in
        --debug) CONFIG="Debug" ;;
        --clean) CLEAN=1 ;;
    esac
done

echo "============================================"
echo " Drem Soundscape - Windows Build (from WSL)"
echo "============================================"
echo ""
echo "Configuration: $CONFIG"

# --- Locate Visual Studio via vswhere.exe ---
VSWHERE="/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
if [[ ! -f "$VSWHERE" ]]; then
    echo "ERROR: vswhere.exe not found. Is Visual Studio installed?"
    exit 1
fi

VS_INSTALL=$("$VSWHERE" -latest -property installationPath 2>/dev/null | tr -d '\r\n')
if [[ -z "$VS_INSTALL" ]]; then
    echo "ERROR: No Visual Studio installation found."
    exit 1
fi
echo "Visual Studio: $VS_INSTALL"

# --- Set up directories on the Windows filesystem ---
# MSVC tools are extremely slow reading from WSL's 9p filesystem,
# so we mirror both source and build to a native Windows drive.
WIN_TEMP=$(cmd.exe /c "echo %TEMP%" 2>/dev/null | tr -d '\r\n')
WSL_TEMP=$(wslpath "$WIN_TEMP")
BASE_DIR="$WSL_TEMP/drem-soundscape"
SOURCE_DIR="$BASE_DIR/src"
BUILD_DIR="$BASE_DIR/build"
ENV_CACHE="$BASE_DIR/_msvc_env.bat"

if [[ "${CLEAN:-}" == "1" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR" "$SOURCE_DIR"
fi

mkdir -p "$SOURCE_DIR" "$BUILD_DIR"

# --- Mirror source tree to Windows filesystem ---
# JUCE is large and never changes between builds, so we only sync it
# on the first run or --clean. Project files are always synced.
if [[ ! -d "$SOURCE_DIR/JUCE" ]]; then
    echo "Syncing full source tree (including JUCE)..."
    rsync -a --delete \
        --exclude='build/' \
        --exclude='build-windows/' \
        --exclude='.git/' \
        "$PROJECT_DIR/" "$SOURCE_DIR/"
else
    echo "Syncing project files..."
    rsync -a \
        --include='src/***' \
        --include='resources/***' \
        --include='CMakeLists.txt' \
        --exclude='*' \
        "$PROJECT_DIR/" "$SOURCE_DIR/"
fi

# --- Convert paths to Windows format ---
WIN_PROJECT=$(wslpath -w "$SOURCE_DIR")
WIN_BUILD=$(wslpath -w "$BUILD_DIR")
WIN_VS_INSTALL=$(echo "$VS_INSTALL" | tr '/' '\\')
WIN_ENV_CACHE=$(wslpath -w "$ENV_CACHE")

echo "Source:    $WIN_PROJECT"
echo "Build dir: $WIN_BUILD"
echo ""

# --- Cache MSVC environment variables ---
# vcvarsall.bat is very slow (~60s). We run it once, snapshot the
# environment, and replay it on subsequent builds.
if [[ ! -f "$ENV_CACHE" ]]; then
    echo "Caching MSVC environment (one-time)..."
    SNAPSHOT_BAT="$BASE_DIR/_snapshot_env.bat"
    cat > "$SNAPSHOT_BAT" <<SNAPEOF
@echo off
call "$WIN_VS_INSTALL\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64 >nul
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize MSVC toolchain.
    exit /b 1
)
set > "$WIN_ENV_CACHE"
SNAPEOF
    WIN_SNAPSHOT_BAT=$(wslpath -w "$SNAPSHOT_BAT")
    cmd.exe /c "$WIN_SNAPSHOT_BAT"
    rm -f "$SNAPSHOT_BAT"
    if [[ ! -f "$ENV_CACHE" ]]; then
        echo "ERROR: Failed to cache MSVC environment."
        exit 1
    fi
    echo "MSVC environment cached."
fi

# --- Generate the build batch file ---
BATCH_FILE="$BUILD_DIR/_build_helper.bat"
cat > "$BATCH_FILE" <<BATCH
@echo off

REM Restore cached MSVC environment
for /f "usebackq tokens=1,* delims==" %%A in ("$WIN_ENV_CACHE") do (
    set "%%A=%%B"
)

if not exist "$WIN_BUILD\\CMakeCache.txt" (
    echo [1/2] Configuring...
    cmake -B "$WIN_BUILD" -S "$WIN_PROJECT" -G Ninja -DCMAKE_BUILD_TYPE=$CONFIG
    if %errorlevel% neq 0 (
        echo ERROR: CMake configure failed.
        exit /b 1
    )
) else (
    echo [1/2] Already configured — skipping. Use --clean to reconfigure.
)

echo.
echo [2/2] Building $CONFIG...
cmake --build "$WIN_BUILD" --parallel
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    exit /b 1
)

echo.
echo ============================================
echo  Build succeeded!
echo  Output: $WIN_BUILD\\DremSoundscape_artefacts\\
echo ============================================
BATCH

WIN_BATCH=$(wslpath -w "$BATCH_FILE")

# --- Run the build via cmd.exe ---
cmd.exe /c "cd /d $WIN_BUILD && $WIN_BATCH"

# --- Copy artifacts back to project directory ---
ARTIFACT_SRC="$BUILD_DIR/DremSoundscape_artefacts"
ARTIFACT_DST="$PROJECT_DIR/build-windows"

if [[ -d "$ARTIFACT_SRC" ]]; then
    mkdir -p "$ARTIFACT_DST"
    rsync -a --delete "$ARTIFACT_SRC/" "$ARTIFACT_DST/DremSoundscape_artefacts/"
    echo ""
    echo "Artifacts copied to: $ARTIFACT_DST/DremSoundscape_artefacts/"
fi
