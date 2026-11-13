# Changes in vscpl2drv-template

Every change that goes into a release is listed here, newest release first.
Add each change under **Unreleased** when it is made. When a release is
tagged, rename **Unreleased** to the version and date, and start a new
**Unreleased** section above it.

## Unreleased

- Fix the CI checksum step: readable SHA256SUMS files, bash 3.2 on macOS, and jobs that run in a subfolder
- Add scripts/download-artifacts.sh to download and verify the newest CI build artifacts
- CI writes a SHA256SUMS-<artifact> checksum file into every uploaded build artifact
- chore: bump version after push (95933fd)
- Auto update of verision patch # (8958d8c)

## v2026.11.1 - 2026-11-10

- Updated version (d8b0fb0)
- Changed initial copyright text (6db0825)
- Create VSCP level II driver template based on vscpl2drv-websocksrv (4a8b6a7)
- Initial commit (b0bd94f)
