% VSCPL2DRV-TEMPLATE(1) VSCP Level II driver
% Åke Hedman, Grodans Paradis AB
% November 2026

# NAME

vscpl2drv-template - template for VSCP Level II drivers

# SYNOPSIS

**libvscpl2drv-template.so** (Linux), **vscpl2drv-template.dll** (Windows), **libvscpl2drv-template.dylib** (macOS)

A VSCP Level II driver. It is loaded by the VSCP daemon (**vscpd**(8)), VSCP Works or any other program that uses the VSCP Level II driver API, and is configured with the JSON configuration file described below. The path of the configuration file is given to the driver when it is opened (**path-config** in the daemon's driver configuration).

# DESCRIPTION

This is a **template for VSCP level II drivers**. It is a fully working driver skeleton, modeled after [vscpl2drv-websocksrv](https://github.com/grodansparadis/vscpl2drv-websocksrv), that is intended to be used as a starting point when developing new VSCP level II drivers.

The template implements everything a level II driver needs:

* The complete [VSCP level II driver API](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces) (`VSCPOpen`, `VSCPClose`, `VSCPWrite`, `VSCPRead`, `VSCPGetVersion`, `VSCPGetVendorString`).
* JSON based configuration from file or inline configuration string.
* Logging to console and rotating log files using [spdlog](https://github.com/gabime/spdlog).
* Thread safe send and receive queues with semaphore signaling.
* A worker thread skeleton where the actual driver functionality is implemented. Search for `TODO:` in src/template.cpp to find the places to fill in.
* Cross platform CMake build (Linux, Windows, macOS, Raspberry Pi) that fetches all dependencies automatically.
* Packaging (Debian/RPM/TGZ/ZIP/NSIS) with CPack.
* Man page and [docsify](https://docsify.js.org) based documentation skeletons.
* GitHub Actions build workflow.

As delivered the template just **loops back** events written to it, which makes it easy to verify that the build and host integration work before you add your own functionality.

# CONFIGURATION

The driver is configured with a JSON file (or a JSON string handed to the
driver as the configuration path). A sample configuration file is available
in the `resources/` folder of the repository and is installed to
*/usr/share/vscpl2drv-template/* by the Linux packages.

## Adding the driver to the VSCP daemon

Add the driver to the level II driver section of the VSCP daemon
configuration file (default */etc/vscp/vscpd.json*):

```json
"drivers" : {
  "level2" : [
    {
      "enable" : true,
      "name" : "template",
      "path-driver" : "/var/lib/vscp/drivers/level2/libvscpl2drv-template.so",
      "path-config" : "/etc/vscp/template.json",
      "guid" : "FF:FF:FF:FF:FF:FF:FF:FC:88:99:AA:BB:CC:DD:EE:FF"
    }
  ]
}
```

| Option | Description |
|--------|-------------|
| `enable` | Driver is loaded if `true`. |
| `name` | A name of your choice for the driver instance. |
| `path-driver` | Path to the driver shared library. |
| `path-config` | Path to the driver configuration file (described below). |
| `guid` | GUID for the driver instance. |

## Driver configuration file

```json
{
  "debug" : false,
  "write" : false,
  "rx-filter" : "0,0,0,-,0,0,0,-",
  "max-queue-size" : 32000,
  "logging" : {
    "console-enable-log" : false,
    "console-log-level" : "info",
    "console-pattern" : "[vscpl2drv-template %c] [%^%l%$] %v",
    "file-enable-log" : true,
    "file-log-level" : "info",
    "file-pattern" : "[vscpl2drv-template %c] [%^%l%$] %v",
    "file-path" : "/var/log/vscp/vscpl2drv-template.log",
    "file-max-size" : 5242880,
    "file-max-files" : 7
  }
}
```

### debug

Set to `true` to get extra debug output from the driver. Default: `false`.

### write

Set to `true` to allow the configuration to be written back by the driver
(remote configuration). Default: `false`.

### rx-filter

Filter for incoming events on the form

```
priority,class,type,GUID,filter-priority,filter-class,filter-type,filter-GUID
```

written as `"filter,mask"` pairs: `"0,0,0,-,0,0,0,-"` accepts all events.
See the [VSCP specification](https://grodansparadis.github.io/vscp-doc-spec/#/)
for a description of VSCP filters.

### max-queue-size

Maximum number of events held in the send/receive queues before events are
dropped. Default: `32000`.

### Logging

| Option | Description | Default |
|--------|-------------|---------|
| `console-enable-log` | Enable logging to the console. | `true` |
| `console-log-level` | One of `off`, `critical`, `error`, `warn`, `info`, `debug`, `trace`. | `info` |
| `console-pattern` | [spdlog pattern](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting) for console log lines. | see above |
| `file-enable-log` | Enable logging to a rotating log file. | `false` |
| `file-log-level` | One of `off`, `critical`, `error`, `warn`, `info`, `debug`, `trace`. | `info` |
| `file-pattern` | spdlog pattern for file log lines. | see above |
| `file-path` | Path to the log file. | `/tmp/vscpl2drv-template.log` |
| `file-max-size` | Maximum size of a log file before it is rotated (bytes). | `5242880` |
| `file-max-files` | Maximum number of rotated log files to keep. | `7` |

<!-- TODO: Document your own configuration options here -->

filename

# FILES

*/var/lib/vscp/drivers/level2/libvscpl2drv-template.so*
:   The driver (Linux).

*resources/linux/template.json*
:   Sample configuration in the source repository (installed with the driver).

# SEE ALSO

**vscpd**(8), **vscp-dump**(1), **vscp-send**(1), **vscpl2drv-socketcan**(1), **vscpl2drv-tcpipsrv**(1)

The source code and the full documentation: <https://github.com/grodansparadis/vscpl2drv-template>

The VSCP specification: <https://grodansparadis.github.io/vscp-doc-spec>

# COPYRIGHT

Copyright 2000-2026 Åke Hedman, Grodans Paradis AB. See the LICENSE file of the driver.
