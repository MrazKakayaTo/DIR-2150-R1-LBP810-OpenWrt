# Canon LBP-810 over IPP on D-Link DIR-2150 R1 (OpenWrt)

> **Experimental hardware-specific print server.** Windows 10 printing was tested successfully on 2026-11-04. Android/AirPrint/Mopria printing is **not supported yet**; mobile test jobs have previously blocked the queue.

This project makes the USB-only Canon LBP-810 (CAPT v1) available as an IPP network printer **without installing Canon drivers on Windows**. The CAPT conversion takes place on the router.

## Verified configuration

| Component | Version / model |
| --- | --- |
| Router | **D-Link DIR-2150 R1**, `dlink,dir-2150-r1`, MediaTek MT7621 |
| Firmware | **OpenWrt 25.12.5**, kernel `6.12.94` |
| Target / package architecture | `ramips/mt7621` / `mipsel_24kc` |
| USB printer | **Canon LBP-810**, USB vendor/product `04a9:260a` |
| Print server | CUPS `2.3.3`, custom CAPT backend and PWG Raster filter |
| Tested client | Windows 10 with Microsoft IPP Class Driver |
| Print mode | A4, 600 DPI, monochrome |

**This is not a generic OpenWrt installation.** `install.sh` intentionally checks the DIR-2150 R1 board name, the exact OpenWrt release, package architecture and printer USB device. Other routers, releases, and printers are untested. A port requires rebuilding packages and adapting the checks; **do not force-install these APKs on another target**.

## Architecture

```text
Windows 10 (Microsoft IPP Class Driver)
              |
         IPP / PWG Raster
              |
      CUPS 2.3.3 on OpenWrt
              |
     pwgtocaptraster filter
              |
       captppd / captusb
              |
      USB Canon LBP-810
```

The `captppd` / `libcapt` backend speaks Canon's CAPT v1 protocol. A plain USB TCP forwarder such as `p910nd` is insufficient for driverless clients.

## Install using the prebuilt archive

1. Back up your OpenWrt configuration and connect the printer to the router's USB port.
2. Download `DIR2150-LBP810-printkit.tar.gz` from this repository. The repository also contains its unpacked files so that the code and configuration can be inspected.
3. Copy the archive to the router (replace `192.168.1.1` with your LAN address):

   ```powershell
   scp .\DIR2150-LBP810-printkit.tar.gz root@192.168.1.1:/tmp/
   ```

4. Run on the router:

   ```sh
   cd /tmp
   tar -xzf DIR2150-LBP810-printkit.tar.gz
   cd DIR2150-LBP810-printkit
   sh install.sh --check
   sh install.sh --apply
   ```

   Run `--apply` **only when `--check` passes**. Installation uses 14 local APK packages and requires no working OpenWrt package mirrors.

5. Check the server before printing:

   ```sh
   ps w | grep '[c]upsd'
   netstat -lntp | grep ':631'
   lpstat -t
   ```

   The intended state is **one** `/usr/sbin/cupsd -f` listening on the **LAN IP only** (for example `192.168.1.1:631`) with an enabled `LBP810` queue.

6. Open **PowerShell as Administrator** on Windows 10 and add the IPP printer:

   ```powershell
   Add-Printer -IppURL "ipp://192.168.1.1:631/printers/LBP810"
   Get-Printer | Format-Table Name,DriverName,PortName
   ```

   The printer should use **Microsoft IPP Class Driver**. The `-Name` parameter is ignored with `-IppURL`; the printer's IPP name is used. Without elevation, PowerShell may return `0x80070005`.

7. Print **one simple A4 page** before attempting larger documents.

### First-installation CUPS process conflict

The initial hardware test exposed an OpenWrt packaging issue: a background `cupsd` started by the package could coexist with the procd-managed process. If `ps` lists two instances, identify the **old process** running with `-C /etc/cups/cupsd.conf -s /etc/cups/cups-files.conf`, then stop **only that PID** and restart the service:

```sh
ps w | grep '[c]upsd'
kill -TERM OLD_CUPSD_PID
/etc/init.d/cupsd restart
netstat -lntp | grep ':631'
```

`OLD_CUPSD_PID` is a placeholder, **not** a command argument to use literally. Check that port 631 is no longer bound to `0.0.0.0` or `::`.

### Boot persistence

The installation writes packages and configuration to persistent `/overlay`. Some installations can retain both `/etc/rc.d/S50cupsd` and `/etc/rc.d/S60cupsd`, causing duplicate startup. Inspect the links and retain a backup of `S50cupsd` before removing it from the boot order:

```sh
ls -l /etc/rc.d/*cupsd*
mkdir -p /overlay/lbp810-printkit/original
if [ -L /etc/rc.d/S50cupsd ]; then
    mv /etc/rc.d/S50cupsd /overlay/lbp810-printkit/original/S50cupsd
fi
/etc/init.d/cupsd enable
```

Keep the archive on your PC: `/tmp` is cleared on reboot. After firmware **sysupgrade**, re-check package compatibility before restoring binaries.

## Known issues and diagnostics

- **Mobile printing:** The initial bundle disables DNS-SD discovery (`Browsing Off`) and does not install Avahi. Its filter chain was developed for Windows PWG Raster output. Android print jobs have been observed reaching CUPS but remaining stuck at `Waiting for last page...`. Automatic AirPrint/Mopria discovery and printing are **not claimed to work**.
- **Queue saturation:** The sample configuration sets `MaxJobsPerPrinter 2`. Two stuck jobs can block all further printing, including from Windows. On a private router with no important pending jobs, identify the stuck IDs with `lpstat -W not-completed -o` and cancel **those IDs** with `cancel LBP810-N`.
- **USB resets:** Repeated `usblp0: removed` / `reset full-speed USB device` events were observed while diagnosing stalled print jobs. The exact root cause of the mobile-job stall remains unconfirmed.
- **Warning messages:** The initial installation reported missing `rastertopwg`, absent CUPS banners, and deprecated printer drivers. Windows printing nevertheless worked.
- **Printer layout:** A previous page-alignment problem was **not observed** in the successful Windows test.

Useful commands:

```sh
lpstat -t
lpstat -W not-completed -o
ps w | grep '[c]upsd'
netstat -lntp | grep ':631'
ls -l /dev/usb/lp*
logread | grep -Ei 'cups|capt|usb|printer|usblp' | tail -n 60
```

Before printing from a new kind of client, confirm that the queue is empty and submit **one page only**.

## Files

- `install.sh`, `uninstall.sh`: hardware checks, offline package installation, backup and rollback.
- `packages/`: **14 prebuilt APKs** for the pinned firmware/architecture.
- `config/`: minimal CUPS config, procd init script, **unused** Avahi service template.
- `sources/pwgtocaptraster.c`: custom streaming PWG Raster → monochrome CUPS Raster filter.
- `sources/raster_smoke.c`: raster smoke-test source.
- `sources/feed/`: OpenWrt package recipes and MIME conversion rule.
- `sources/captppd-1.0.0.tar.gz`: third-party CAPT project source snapshot including its license.
- `docs/Canon_LBP810_DIR2150_OpenWrt_guide.docx`: Russian Word installation guide.
- `DIR2150-LBP810-printkit.tar.gz`: the **original tested installer archive**, preserved without modifications.
- `SHA256SUMS`: checksums of files *inside* the original installer archive.

Original installer archive SHA-256:

```text
7d793d750985a5a67df312196fccf17c93a435a4ea94518e405d164514516fad
```

## Attribution and licensing

The **new custom PWG Raster filter source** is provided under MIT (see its own source header and `LICENSE`). The archive also includes software written and licensed by **third parties**; the MIT license **does not supersede** their licenses. See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) before redistribution. In particular, `captppd` and `libcapt` are credited to their respective authors.

This is an experimental, hardware-specific project with no warranty. Do not expose TCP port 631 to the Internet.
