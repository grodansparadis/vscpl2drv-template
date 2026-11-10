# Build the driver from source

The driver is built with CMake and builds on Linux (including Raspberry Pi),
Windows and macOS. All third party dependencies (the VSCP common code,
[spdlog](https://github.com/gabime/spdlog) and
[nlohmann/json](https://github.com/nlohmann/json)) are fetched automatically
by CMake with `FetchContent` — you only need a compiler, CMake and OpenSSL.

## Linux / macOS

Install the prerequisites (Debian/Ubuntu shown):

```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev pandoc
```

(pandoc is optional and only used to generate the man page.)

Then build:

```bash
git clone https://github.com/grodansparadis/vscpl2drv-template.git
cd vscpl2drv-template
./build-unix.sh
```

or manually:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The driver ends up as `build/libvscpl2drv-template.so`
(`.dylib` on macOS).

Install with:

```bash
sudo make install
```

## Build packages

```bash
cd build
cpack
```

builds Debian/RPM/TGZ/ZIP packages on Linux.

## Windows

The Windows build uses [vcpkg](https://github.com/microsoft/vcpkg) for
dependencies. Install the dependencies:

```cmd
vcpkg install pthreads:x64-windows openssl:x64-windows
```

Then adjust the vcpkg paths in `build-windows.bat` and run it, or use the
CMake presets:

```cmd
cmake --preset windows-release
cmake --build --preset windows-release
```

## CMake presets

The project ships `CMakePresets.json` with the presets `linux-debug`,
`linux-release`, `windows-debug` and `windows-release`:

```bash
cmake --preset linux-release
cmake --build build/linux-release
```

[filename](./bottom-copyright.md ':include')
