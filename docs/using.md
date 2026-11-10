# Test and use the driver

The template driver loops back every event that is written to it. This makes
it easy to verify that the driver loads correctly and that the host
integration works.

## Using with the VSCP daemon

1. Install the driver (see [Install the driver](./install.md)).
2. Add the driver to */etc/vscp/vscpd.json* (see [Configure the driver](./configure.md)).
3. Restart the daemon:

```bash
sudo systemctl restart vscpd
```

4. Check the daemon log and the driver log file for errors.
5. Send an event to the driver interface (for example with
   [VSCP Works](https://grodansparadis.github.io/vscp-works-qt/#/)) and verify
   that the event is looped back.

## Enable debug output

Set `"debug" : true` and raise the log levels in the driver configuration
file to get verbose output while testing:

```json
"logging" : {
  "console-enable-log" : true,
  "console-log-level" : "debug",
  "file-enable-log" : true,
  "file-log-level" : "debug"
}
```

<!-- TODO: Describe how to test and use your driver here -->

[filename](./bottom-copyright.md ':include')
