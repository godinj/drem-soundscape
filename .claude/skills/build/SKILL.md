---
name: build
description: Build the drem-soundscape JUCE application with CMake. Use when compiling the project or troubleshooting build issues.
allowed-tools: Bash, Read, Grep
argument-hint: [Debug|Release]
user-invocable: true
---

# Build drem-soundscape

Build the application using CMake.

## Steps

1. Ensure the JUCE submodule is initialized:
   ```bash
   git -C /home/godinj/dev/drem-soundscape submodule update --init --recursive
   ```

2. Create the build directory and configure:
   ```bash
   cmake -S /home/godinj/dev/drem-soundscape -B /home/godinj/dev/drem-soundscape/build -DCMAKE_BUILD_TYPE=$0
   ```
   Use `$0` as the build type. Default to `Debug` if no argument given.

3. Build:
   ```bash
   cmake --build /home/godinj/dev/drem-soundscape/build --config $0
   ```

4. Report build result — success or failure with relevant error output.

## Troubleshooting

If the build fails:
- Check that JUCE submodule is initialized: `git submodule status`
- Verify CMake version: `cmake --version` (need 3.15+)
- On Linux, check for JUCE dependencies: `sudo apt-get install -y libasound2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev`
- Review the full error output and suggest fixes
