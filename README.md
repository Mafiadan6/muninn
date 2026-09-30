# MUNINN

> Memory's raven. A modern Samsung firmware flashing tool for Linux.

MUNINN is a native, open-source implementation of the Samsung **Thor** (Odin)
USB flashing protocol — rebranded and rebuilt, forged for **爪卂丂ㄒ乇尺爪工刀ᗪ**.

- Native aarch64 / x86_64 Linux, no Wine, no VM
- Open-source and auditable (Apache-2.0)
- .tar.md5 integrity verification (Crypto++)
- Automatic PIT retrieval, strict partition validation
- Dry-run validation mode

## Features (beyond upstream)

| Flag | Effect |
| --- | --- |
| `--no-reboot` | Never reboot the device after flashing (overrides `--reboot`) |
| `--wait-device <secs>` | Poll until a Download Mode device appears, then proceed |
| `--json` | Machine-readable device list / flash summary |

## Install

Prebuilt binaries are on the [Releases](../../releases) page
(`muninn_8.0.0-mastermind_linux-aarch64`).

```bash
install -m755 muninn /usr/local/bin/muninn
install -m644 udev/60-muninn.rules /etc/udev/rules.d/
```

## Build from source

Dependencies: `cmake`, `ninja`, `g++`/`clang`, `libusb`, `lz4`, `crypto++`
(`pkgconf` for the crypto++ detection).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DMUNINN_BUILD_GUI=OFF -DMUNINN_BUILD_TESTS=OFF
ninja -C build
```

## Usage

```
muninn -l                                    # list Download Mode devices
muninn -b BL.tar -a AP.tar -c CP.tar -s CSC.tar          # full flash
muninn -a AP.tar.md5 --no-reboot             # flash, stay in Download Mode
muninn --check-only -a AP.tar.md5           # validate PIT + archives only
muninn --wait-device 30 -a AP.tar.md5       # wait for plug-in then flash
muninn --json -l                             # machine-readable list
muninn -d /dev/bus/usb/001/002 -a AP.tar    # target a specific device
muninn --redownload                         # reboot back into Download Mode
```

Full options: `muninn -h`.

## udev rules

`udev/60-muninn.rules` grants access to Samsung Download Mode devices
(vendor `04e8`, PIDs `6601 685d 68c3 68ef 4eee 4eef`). udevd reloads rules
automatically on change; reconnect the device after install.

## Safety

Built from the `v7.3.0` tag (commit `0aba9c5`), sha256-checked, no
telemetry and no network I/O — the only interfaces are libusb (usbfs),
your firmware files, and `muninn.log`. Always flash the correct firmware
for your exact model, and `--check-only` first. Flashing wrong partitions
can brick a device.

## Credits

Derived from [Llucs/odin4](https://github.com/Llucs/odin4), a reconstructed
implementation of Samsung's Odin/Thor protocol (Apache-2.0). Upstream retains
copyright; this fork adds branding and the features above.