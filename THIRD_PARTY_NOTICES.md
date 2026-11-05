# Third-party notices

This repository includes **unmodified or repackaged third-party software**. The root MIT license is limited to the new project code and does not alter upstream licensing obligations.

## CAPT support

- **captppd**: https://github.com/darkvision77/captppd — source snapshot under `sources/captppd-1.0.0.tar.gz`, pinned upstream commit `d6df4f867d801e9dbe77f7e58eaec3a77f118416`. The archive includes the author's BSD-style license (copyright 2025 Andrey Antufyev).
- **libcapt**: https://github.com/darkvision77/libcapt — pinned upstream commit `8f1fd502e6fee893d409f09c5895248c189ead4b`; incorporated into CAPT build from upstream source. Consult the upstream license included in the source snapshot.

## Printer server and runtime packages

The `packages/` directory also contains locally built OpenWrt packages of CUPS 2.3.3 and its runtime libraries (`libcups`, `libcupsimage`, GCC libraries, `zlib`, `libpng`, `libjpeg-turbo`, `libusb`). These components have **their own licenses**, attribution and redistribution terms. Sources and package recipes are available from:

- OpenWrt firmware and SDK: https://downloads.openwrt.org/releases/25.12.5/targets/ramips/mt7621/
- CUPS 2.3.3: https://github.com/apple/cups/tree/v2.3.3
- OpenWrt packages feed: https://github.com/openwrt/packages
- Upstream libraries: https://www.zlib.net/ , https://libpng.org/pub/png/libpng.html , https://libjpeg-turbo.org/ , https://libusb.info/

**Before redistributing binary packages**, verify their individual license notices and corresponding source availability. This project cannot grant new rights over third-party binaries.
