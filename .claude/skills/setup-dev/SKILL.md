---
name: setup-dev
description: Set up the development environment for drem-soundscape including submodules, dependencies, and build configuration.
allowed-tools: Bash, Read
disable-model-invocation: true
user-invocable: true
---

# Setup Development Environment

Initialize the full development environment for drem-soundscape.

## Steps

1. **Initialize JUCE submodule:**
   ```bash
   cd /home/godinj/dev/drem-soundscape && git submodule update --init --recursive
   ```

2. **Check system dependencies:**

   On Linux (Debian/Ubuntu), JUCE needs:
   ```bash
   sudo apt-get install -y build-essential cmake pkg-config \
     libasound2-dev libcurl4-openssl-dev libfreetype6-dev \
     libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
     libxinerama-dev libxrandr-dev libxrender-dev \
     libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev
   ```

   On macOS, Xcode command line tools are needed:
   ```bash
   xcode-select --install
   ```

   On Windows, Visual Studio with C++ workload is needed.

   Detect the current platform and check/install the appropriate dependencies.

3. **Verify toolchain:**
   ```bash
   cmake --version
   c++ --version
   ```
   CMake 3.15+ and a C++17-capable compiler are required.

4. **Configure the build:**
   ```bash
   cmake -S /home/godinj/dev/drem-soundscape -B /home/godinj/dev/drem-soundscape/build -DCMAKE_BUILD_TYPE=Debug
   ```

5. **Report status** — confirm what succeeded and flag anything that needs manual attention.
