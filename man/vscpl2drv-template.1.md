% vscpl2drv-template(1) VSCP Level II template driver
% Åke Hedman, the VSCP Project
% November 10, 2026

# NAME

vscpl2drv-template - VSCP Level II template driver

# SYNOPSIS

vscpl2drv-template

# DESCRIPTION

This is a template for a VSCP level II driver. It is intended to be used as a
starting point when developing new VSCP level II drivers. The template
implements the full VSCP level II driver API, JSON configuration handling,
logging and send/receive queues. As delivered it just loops back events
written to it.

The level II driver API is [described here](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces).

## Configuration

The *configuration string* is the first configuration data that is read by
the driver. It is either a path to a JSON configuration file or the JSON
configuration itself as a string.

### Adding the driver to the VSCP daemon.

Add the driver to the vscpd configuration file (default location
*/etc/vscp/vscpd.json*). The entry looks the same for all level II drivers.

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

### Driver configuration file

```json
{
  "debug" : false,
  "write" : false,
  "rx-filter" : "0,0,0,-,0,0,0,-",
  "max-queue-size" : 32000,
  "logging" : {
    "console-enable-log": false,
    "console-log-level" : "info",
    "console-pattern" : "[vscpl2drv-template %c] [%^%l%$] %v",
    "file-enable-log": true,
    "file-log-level" : "info",
    "file-pattern" : "[vscpl2drv-template %c] [%^%l%$] %v",
    "file-path" : "/var/log/vscp/vscpl2drv-template.log",
    "file-max-size" : 5242880,
    "file-max-files" : 7
  }
}
```

See the full documentation for a description of all configuration options.

# SEE ALSO

`vscpd` (8).
`uvscpd` (8).
`vscpworks` (1).
`vscpcmd` (1).
`vscp-makepassword` (1).
`vscphelperlib` (1).

The VSCP project homepage is here <https://www.vscp.org>.

# COPYRIGHT

Copyright 2000-2026 Åke Hedman, the VSCP Project - MIT license.
