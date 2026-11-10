# vscpl2drv-template

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Build](https://github.com/grodansparadis/vscpl2drv-template/actions/workflows/build.yml/badge.svg)](https://github.com/grodansparadis/vscpl2drv-template/actions/workflows/build.yml)

<img src="https://vscp.org/images/logo.png" width="100">

  * **Available for**: Linux (Including Raspberry Pi 32/64-bit), Windows, MacOS
  * **Driver Linux**: libvscpl2drv-template.so
  * **Driver Windows**: vscpl2drv-template.dll
  * **Driver MacOS**: libvscpl2drv-template.dylib

## Introduction

This is a **template for VSCP level II drivers**. It is a fully working driver skeleton, modeled after [vscpl2drv-websocksrv](https://github.com/grodansparadis/vscpl2drv-websocksrv), that is intended to be used as a starting point when developing new VSCP level II drivers.

The template implements everything a level II driver needs:

* The complete [VSCP level II driver API](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces) (`VSCPOpen`, `VSCPClose`, `VSCPWrite`, `VSCPRead`, `VSCPGetVersion`, `VSCPGetVendorString`).
* JSON based configuration from file or inline configuration string.
* Logging to console and rotating log files using [spdlog](https://github.com/gabime/spdlog).
* Thread safe send and receive queues with semaphore signaling.
* A worker thread skeleton where the actual driver functionality is implemented. Search for `TODO:` in [src/template.cpp](./src/template.cpp) to find the places to fill in.
* Cross platform CMake build (Linux, Windows, macOS, Raspberry Pi) that fetches all dependencies automatically.
* Packaging (Debian/RPM/TGZ/ZIP/NSIS) with CPack.
* Man page and [docsify](https://docsify.js.org) based documentation skeletons.
* GitHub Actions build workflow.

As delivered the template just **loops back** events written to it, which makes it easy to verify that the build and host integration work before you add your own functionality.

## How to make a driver from this template

The step by step guide is in [docs/creating-a-driver.md](./docs/creating-a-driver.md). In short:

1. Create a new repository from this template.
2. Rename the files and identifiers (`template` → your driver name).
3. Implement your functionality at the `TODO:` markers in the main driver class.
4. Update configuration, documentation and packaging.

## Build

```bash
./build-unix.sh
```

or see [docs/build.md](./docs/build.md) for details and Windows instructions.

Requirements on Linux: `build-essential cmake libssl-dev libexpat1-dev` (and optionally `pandoc` for the man page). Everything else is fetched automatically by CMake.

## Documentation

* Documentation skeleton is in the [docs](./docs) folder and can be published with GitHub Pages.
* The level II driver API is [described here](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces).

## Other sources of information

 * The VSCP site - https://www.vscp.org
 * The VSCP document site - https://docs.vscp.org/
 * VSCP discussions - https://github.com/grodansparadis/vscp/discussions

## License

The driver code is released under the [MIT license](./LICENSE). The documentation is licensed under Creative Commons BY 4.0.
