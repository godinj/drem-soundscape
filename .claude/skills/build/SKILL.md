---
name: build
description: Build the drem-soundscape JUCE application with CMake. Use when compiling the project or troubleshooting build issues.
allowed-tools: Bash, Read, Grep
argument-hint: [Debug|Release] [--windows]
user-invocable: true
---

# Build drem-soundscape

Build the application using CMake. Supports both Linux and Windows targets.

## Argument parsing

- `$0` may contain a build type (`Debug` or `Release`) and/or `--windows`.
- Default build type: `Debug`
- Default target: Linux (native)
- If `--windows` is present, build a Windows executable via `build-windows.sh`.

## Steps

1. Ensure the JUCE submodule is initialized:
   ```bash
   git -C /home/jgodin/dev/drem-soundscape submodule update --init --recursive
   ```

2. **If `--windows` is in the arguments**, run the Windows cross-build script:
   ```bash
   /home/jgodin/dev/drem-soundscape/build-windows.sh [--debug if Debug]
   ```
   This builds a Windows `.exe` from WSL using the host's MSVC toolchain.
   The binary is output to `build-windows/DremSoundscape_artefacts/Release/` (or `Debug`).
   Skip to step 4.

3. **Otherwise (Linux)**, create the build directory, configure, and build:
   ```bash
   cmake -S /home/jgodin/dev/drem-soundscape -B /home/jgodin/dev/drem-soundscape/build -DCMAKE_BUILD_TYPE=<type>
   cmake --build /home/jgodin/dev/drem-soundscape/build --config <type>
   ```

4. Report build result — success or failure with relevant error output.

## Troubleshooting

If the build fails:
- Check that JUCE submodule is initialized: `git submodule status`
- Verify CMake version: `cmake --version` (need 3.15+)
- On Linux, check for JUCE dependencies: `sudo apt-get install -y libasound2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev`
- For Windows builds (`--windows`): requires Visual Studio 2022 with C++ workload installed on the Windows host
- Review the full error output and suggest fixes
