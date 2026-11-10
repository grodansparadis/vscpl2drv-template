
## Documentation for the vscpl2drv-template driver

**Document version:** ${/var/document-version} - ${/var/creation-time}
[HISTORY](./history.md)

This is a template for a VSCP level II driver. It is not a driver that is useful by itself. Instead it is a fully working skeleton that is intended to be used as a starting point when developing new VSCP level II drivers.

The template implements everything a level II driver needs:

* The complete [VSCP level II driver API](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces) (`VSCPOpen`, `VSCPClose`, `VSCPWrite`, `VSCPRead`, `VSCPGetVersion`, `VSCPGetVendorString`).
* JSON based configuration from file or inline configuration string.
* Logging to console and rotating log files using [spdlog](https://github.com/gabime/spdlog).
* Thread safe send and receive queues with semaphore signaling.
* A worker thread skeleton where the actual driver functionality should be implemented.
* CMake based cross platform build (Linux, Windows, macOS, Raspberry Pi).
* Packaging (Debian/RPM/TGZ/ZIP/NSIS) with CPack.
* Man page and docsify based documentation skeletons.

As delivered the template just loops back events written to it, which makes it easy to verify that the build and the host integration works before you start adding your own functionality.

**Start here: [Making a driver from the template](./creating-a-driver.md)** describes step by step how to turn this template into a finished driver.

The level II driver API is [described here](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces). With the simple interface API the VSCP level II driver uses it is also easy to use a driver with other software as a component.

* [Repository for the module](https://github.com/grodansparadis/${/var/driver-name})
* This manual is available [here](https://grodansparadis.github.io/${/var/driver-name})


## VSCP - the Very Simple Control Protocol (framework)

![VSCP logo](./images/logo_100.png)

VSCP is a free and open automation protocol for IoT and m2m devices. Visit [the VSCP site](https://www.vscp.org) for more information.

**VSCP is free.** Placed in the **public domain**. Free to use. Free to change. Free to do whatever you want to do with it. VSCP is not owned by anyone. VSCP will stay free and gratis forever.

The specification for the VSCP protocol is [here](https://grodansparadis.github.io/vscp-doc-spec/#/)

VSCP documentation for various parts of the protocol/framework can be found [here](https://docs.vscp.org/).

If you use VSCP please consider contributing resources or time to the project ([https://github.com/sponsors/grodansparadis](https://github.com/sponsors/grodansparadis)).


## Document license

This document is licensed under [Creative Commons BY 4.0](https://creativecommons.org/licenses/by/4.0/) and can be freely copied, redistributed, remixed, transformed, built upon as long as you give credits to the author.


[filename](./bottom-copyright.md ':include')
