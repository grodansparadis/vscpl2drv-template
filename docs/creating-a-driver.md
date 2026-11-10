# Making a driver from the template

This document describes, step by step, how to turn the **vscpl2drv-template** skeleton into a finished VSCP level II driver. In the steps below we assume your new driver is called **vscpl2drv-mydriver** — replace *mydriver* with the real name of your driver everywhere.

A VSCP level II driver is a shared library (`.so`/`.dll`/`.dylib`) that exports the six functions of the [VSCP level II driver API](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_driver_interfaces):

| Function              | Purpose                                                   |
|-----------------------|-----------------------------------------------------------|
| `VSCPOpen`            | Create and open a driver instance                         |
| `VSCPClose`           | Close and delete a driver instance                        |
| `VSCPWrite`           | Host writes an event *to* the driver (host → device)      |
| `VSCPRead`            | Host reads an event *from* the driver (device → host)     |
| `VSCPGetVersion`      | Driver version                                            |
| `VSCPGetVendorString` | Vendor string                                             |

All of this plumbing is already implemented by the template. What you implement is the *worker thread* and the device specific configuration in the main driver class.

## 1. Create your repository from the template

Either use GitHub's *"Use this template"* button on the [vscpl2drv-template repository](https://github.com/grodansparadis/vscpl2drv-template), or clone and re-init:

```bash
git clone https://github.com/grodansparadis/vscpl2drv-template.git vscpl2drv-mydriver
cd vscpl2drv-mydriver
rm -rf .git
git init
```

## 2. Rename files

```bash
cd src
mv vscpl2drv-template.h   vscpl2drv-mydriver.h
mv vscpl2drv-template.cpp vscpl2drv-mydriver.cpp
mv template.h             mydriver.h
mv template.cpp           mydriver.cpp
cd ..
mv man/vscpl2drv-template.1.md man/vscpl2drv-mydriver.1.md
mv resources/linux/template.json resources/linux/mydriver.json
```

## 3. Search and replace

Do a project wide case sensitive search and replace (ignore the `build/` folder):

| Search                | Replace with          | Notes                                     |
|-----------------------|-----------------------|-------------------------------------------|
| `vscpl2drv-template`  | `vscpl2drv-mydriver`  | File names, log prefixes, docs, CMake     |
| `VSCPL2DRV_TEMPLATE`  | `VSCPL2DRV_MYDRIVER`  | Version macros in `src/version.h`         |
| `CTemplateDrv`        | `CMyDriver`           | The main driver class                     |
| `template.h`          | `mydriver.h`          | Include statements                        |
| `TEMPLATE`            | `MYDRIVER`            | Include guards etc                        |

For example with `sed`:

```bash
grep -rl --exclude-dir=build --exclude-dir=.git 'vscpl2drv-template' . | xargs sed -i 's/vscpl2drv-template/vscpl2drv-mydriver/g'
grep -rl --exclude-dir=build --exclude-dir=.git 'VSCPL2DRV_TEMPLATE' . | xargs sed -i 's/VSCPL2DRV_TEMPLATE/VSCPL2DRV_MYDRIVER/g'
grep -rl --exclude-dir=build --exclude-dir=.git 'CTemplateDrv' .       | xargs sed -i 's/CTemplateDrv/CMyDriver/g'
grep -rl --exclude-dir=build --exclude-dir=.git '"template.h"' .       | xargs sed -i 's/"template.h"/"mydriver.h"/g'
```

Also update:

* `CMakeLists.txt` — the `project(...)` name, `APP_LOW_NAME`, `APP_NAME`, `APP_URL`, package description/summary, the source file list and the install rule for the sample configuration file.
* `man/CMakeLists.txt` and `man/make_man_pages` — man page file names.
* `src/version.h` — set your initial version (the VSCP convention is `year.month.patch`).
* `docs/variables.xml` — `driver-name`, `document-version` and `creation-time`.
* `docs/index.html` — the `name` and `repo` fields.

Verify that nothing is left:

```bash
grep -ri --exclude-dir=build --exclude-dir=.git "template" . | grep -iv "mydriver"
```

## 4. Verify that the renamed skeleton builds

```bash
./build-unix.sh
```

The build produces `build/libvscpl2drv-mydriver.so` (on Linux). Fix any renaming mistakes before moving on.

## 5. Implement your driver

All the places where you need to add code are marked with `TODO:` in `src/mydriver.cpp`. The important ones:

### 5.1 Configuration

* Add defaults for your configuration values in the constructor of `CMyDriver`.
* Read your configuration values in `doLoadConfig()` — follow the pattern used for the standard values (`debug`, `write`, `rx-filter`, ...). The configuration is JSON and is available as the parsed object `m_j_config`.
* Add member variables for your settings in `src/mydriver.h`.
* Document every configuration value in `docs/configure.md` and update the sample configuration files in `resources/`.

### 5.2 Open/close your device

* In `open()` (or in the worker thread if that fits your driver better): connect to the device, serial port, socket, service or whatever your driver interfaces.
* In `close()`: disconnect and free resources.

### 5.3 The worker thread

The worker thread (`workerThread()` in `src/mydriver.cpp`) is the heart of the driver:

* **Host → device**: events written to the driver by the host end up in `m_sendQueue`. The template already waits on the queue semaphore and pops events for you — replace the demo loopback with code that delivers the event to your device. Remember to free the event with `vscp_deleteEvent_v2()` when you are done with it.
* **Device → host**: when your device produces data, build a `vscpEvent`/`vscpEventEx` and hand it to the host with `addEvent2ReceiveQueue()`. Incoming events are automatically checked against the receive filter (`rx-filter`). Use the driver GUID (`m_guid`) as base for event GUIDs.

Typical event construction:

```cpp
vscpEventEx ex;
memset(&ex, 0, sizeof(ex));
ex.head       = VSCP_HEADER16_DUMB; // or appropriate header bits
ex.timestamp  = vscp_makeTimeStamp();
vscp_setEventExToNow(&ex);
ex.vscp_class = VSCP_CLASS1_MEASUREMENT;
ex.vscp_type  = VSCP_TYPE_MEASUREMENT_TEMPERATURE;
memcpy(ex.GUID, pDrv->m_guid.getGUID(), 16);
// set ex.data / ex.sizeData ...
pDrv->addEvent2ReceiveQueue(ex);
```

Pick the correct [VSCP class/type](https://grodansparadis.github.io/vscp-doc-spec/#/./vscp_class_list) for the data your device produces.

### 5.4 Threads and locking

If you add more threads, protect the queues with the existing mutexes (`m_mutexSendQueue`, `m_mutexReceiveQueue`) and signal with the semaphores (`m_semSendQueue`, `m_semReceiveQueue`). The `addEvent2...` helpers already do this for you.

## 6. Test the driver

The easiest way to test is with the [VSCP daemon](https://github.com/grodansparadis/vscpd). Add your driver to `/etc/vscp/vscpd.json`:

```json
"drivers" : {
  "level2" : [
    {
      "enable" : true,
      "name" : "mydriver",
      "path-driver" : "/var/lib/vscp/drivers/level2/libvscpl2drv-mydriver.so",
      "path-config" : "/etc/vscp/mydriver.json",
      "guid" : "FF:FF:FF:FF:FF:FF:FF:FC:88:99:AA:BB:CC:DD:EE:FF"
    }
  ]
}
```

Enable debug logging in the driver configuration while developing:

```json
"logging" : {
  "console-enable-log" : true,
  "console-log-level" : "debug",
  "file-enable-log" : true,
  "file-log-level" : "debug",
  "file-path" : "/tmp/vscpl2drv-mydriver.log"
}
```

You can also load the driver directly from a small test program using the six API functions — see the API documentation for details.

## 7. Update the documentation

* `README.md` — description of your driver (badges, intro, platforms).
* `docs/README.md` — introduction.
* `docs/configure.md` — document **all** configuration options.
* `docs/install.md`, `docs/build.md`, `docs/using.md`, `docs/faq.md`, `docs/links.md` — adapt to your driver.
* `docs/history.md` — document history.
* `man/vscpl2drv-mydriver.1.md` — the man page.
* Remove this file (`docs/creating-a-driver.md`) from the finished driver and from `docs/_sidebar.md`.

To preview the docsify documentation locally:

```bash
npm i docsify-cli -g
docsify serve docs
```

Enable GitHub Pages for the repository (deploy from the `docs/` folder on the main branch) to publish the documentation at `https://<user>.github.io/vscpl2drv-mydriver`.

## 8. Packaging and release

Debian/RPM/TGZ packages are built with CPack:

```bash
cd build
cpack
```

* Update the Debian package dependencies (`CPACK_DEBIAN_PACKAGE_DEPENDS`) in `CMakeLists.txt` if your driver needs extra runtime libraries.
* Set the version in `src/version.h` before every release (`year.month.patch`).
* Tag the release in git and upload the packages to the GitHub release page.

## 9. Checklist for a finished driver

- [ ] All files and identifiers renamed, no stray `template` left
- [ ] `src/version.h` has correct version and copyright
- [ ] Worker thread implements real functionality (demo loopback removed)
- [ ] All configuration options read in `doLoadConfig()` and documented in `docs/configure.md`
- [ ] Sample configuration files in `resources/` updated
- [ ] README and documentation updated, `docs/creating-a-driver.md` removed
- [ ] Man page updated
- [ ] Builds cleanly on all platforms you target (check the GitHub Actions workflow)
- [ ] Tested against the VSCP daemon
- [ ] Packages build with CPack

[filename](./bottom-copyright.md ':include')
