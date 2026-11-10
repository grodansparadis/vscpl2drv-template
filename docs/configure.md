# Configure the driver

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

[filename](./bottom-copyright.md ':include')
