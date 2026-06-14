# 🛠 RealNetOS — A Real-Time Microkernel for Raspberry Pi 4

**RealNetOS** is a from-scratch, bare-metal AArch64 operating system for the Raspberry Pi 4. It boots without Linux, brings up an HDMI framebuffer console and a UART serial shell, and demonstrates real-time scheduling, a custom TCP/IP stack, and a FAT32 filesystem — all in ~5,000 lines of C and assembly.

> **Runtime mode:** RealNetOS runs its interactive shell in **kernel mode (EL1)** for stability. EL0 user-mode syscall return is still unstable, so user-mode apps are compiled but not executed in the boot path (see the status tables below).

---

## ✅ What works today (verified in QEMU `raspi4b`)

| Area | Status | Where |
|------|--------|-------|
| AArch64 boot, EL2→EL1 drop, BSS/vector setup | ✅ | [boot/start.S](boot/start.S) |
| PL011 UART serial console | ✅ | [kernel/uart.c](kernel/uart.c) |
| HDMI framebuffer console (mailbox, 1024×768, 2× font) | ✅ | [kernel/fb.c](kernel/fb.c) |
| Colored boot banner + self-running on-screen demo | ✅ | [kernel/main.c](kernel/main.c) |
| Interactive kernel shell (EL1) | ✅ | [kernel/ksh.c](kernel/ksh.c) |
| VFS + FAT32 read/write (simulated SD under QEMU) | ✅ | [fs/fat32.c](fs/fat32.c) |
| Cooperative real-time scheduler + tick accounting | ✅ | [kernel/threads.c](kernel/threads.c) |
| Ethernet/IP/UDP/TCP + GENET driver (compiles; rpi only) | ⚠️ unverified on HW | [net/](net/), [kernel/genet.c](kernel/genet.c) |
| SDHCI driver for a real SD card | ⚠️ unverified on HW | [block/sd_rpi.c](block/sd_rpi.c) |

## ❌ Not implemented (be aware before planning a demo)

- **No USB stack** (no DWC2/xHCI host controller driver).
- **No Bluetooth** — so **no Bluetooth keyboard/mouse**.
- **No WiFi** (no SDIO + brcmfmac firmware loader + 802.11 + WPA2 supplicant).
- **No EL0 user-mode execution** — multitasking is cooperative inside the EL1 shell.

Shell **input** therefore comes over the **UART serial line** (a USB-to-serial adapter on a host PC). HDMI shows all output. Adding on-device keyboard input would require a full USB host + HID stack first; Bluetooth/WiFi are each multi-month efforts comparable to porting chunks of BlueZ / brcmfmac / wpa_supplicant. They are intentionally out of scope.

---

## 📦 Project structure

```
kernel/   Core: boot glue, threads, scheduler, syscalls, shell, framebuffer
net/      Networking: Ethernet, IP, UDP, TCP, DMA rings
fs/       VFS + FAT32
block/    SD block device — sd_qemu.c (in-RAM fake) / sd_rpi.c (SDHCI)
boot/     start.S, exception vectors, linker script
user/     User-mode apps (compiled, not run in default path)
include/  Shared headers
src/      Minimal C library (memcpy, itoa, string utils)
```

---

## 🚀 Build & run

Requires the `aarch64-none-elf` GNU toolchain and `qemu-system-aarch64`.

```bash
# Build for QEMU (in-RAM fake SD, GENET disabled) — this is the default
make

# Serial-only QEMU run (shell on your terminal; Ctrl-A then X to quit)
make run-qemu

# QEMU with an emulated HDMI window (shows the framebuffer console)
make run-graphic

# Build the real-hardware kernel (SDHCI + GENET enabled)
make PLATFORM=rpi
```

On boot you'll see the cyan banner, the FAT32 root listing, eight live real-time
scheduler ticks, then the `RealNetOS#` prompt. Try `help`, `ls`, `cat TEST`,
`write notes.txt hello`, `rt`, `ticks`.

---

## 📺 Running on a real Raspberry Pi 4

### 1. Stage the SD-card files

```bash
make sdcard
```

This builds the `PLATFORM=rpi` kernel and creates `dist/` containing
`kernel8.img` and `config.txt`.

### 2. Add the Raspberry Pi firmware

The Pi's GPU bootloader needs two firmware files. Download them from the
official firmware repo and drop them into `dist/`:

- [`start4.elf`](https://github.com/raspberrypi/firmware/blob/master/boot/start4.elf)
- [`fixup4.dat`](https://github.com/raspberrypi/firmware/blob/master/boot/fixup4.dat)

```bash
cd dist
curl -LO https://github.com/raspberrypi/firmware/raw/master/boot/start4.elf
curl -LO https://github.com/raspberrypi/firmware/raw/master/boot/fixup4.dat
```

> Pi 4 loads `bootcode.bin` from its onboard EEPROM, so you do **not** need it on the SD card.

`dist/` should now contain: `start4.elf`, `fixup4.dat`, `config.txt`, `kernel8.img`.

### 3. Format the SD card and copy the files

> ⚠️ The FAT32 driver assumes the filesystem starts at **LBA 0** (no MBR/GPT).
> Format the card as a **single FAT32 volume**, then copy the four files to its root.

On macOS:

```bash
diskutil list                                   # find your card, e.g. /dev/disk4
diskutil eraseDisk FAT32 RNOS MBRFormat /dev/disk4
cp dist/* /Volumes/RNOS/
diskutil eject /dev/disk4
```

### 4. Wire it up and boot

1. Connect HDMI to a monitor or capture card.
2. Connect a **3.3V USB-to-serial adapter** to the Pi:
   - adapter **GND** → Pi pin 6 (GND)
   - adapter **RX** → Pi pin 8 (GPIO14 / TXD)
   - adapter **TX** → Pi pin 10 (GPIO15 / RXD)
3. On your host, open the serial console at **115200 baud**, 8N1:
   ```bash
   # macOS (device name varies)
   screen /dev/tty.usbserial-XXXX 115200
   # Linux
   screen /dev/ttyUSB0 115200
   ```
4. Power the Pi. The HDMI shows the banner + demo; type commands in the serial terminal.

---

## 🎬 Recording the demo video

Goal: one clean take showing the OS booting and being driven live.

1. **Frame both outputs.** Put the HDMI display (or capture-card window) and your
   serial-terminal window side by side so the video shows on-screen output *and*
   your typed commands.
2. **Power-cycle on camera** so the banner and the auto-demo (FAT32 listing + RT
   ticks) appear from a cold boot — that's the strongest opening shot.
3. **Drive the shell** for the interactive segment:
   ```
   help
   ls
   write demo.txt RealNetOS is alive
   cat demo.txt
   rt
   ticks
   ```
4. **Show real-time behavior:** run `rt` (meets deadlines) then `rt-stress`
   (intentionally overruns and counts `deadline_misses`) to contrast them.
5. If you can't wire serial, you can still record the **QEMU graphical** run
   (`make run-graphic`) as a fallback — same software, emulated display.

---

## 🗺 Roadmap (honest, in priority order)

1. **Fix EL0 user-mode return** so the user-space shell/apps actually run.
2. **Timer-interrupt-driven preemption** for true (not cooperative) RT scheduling.
3. **USB host stack (DWC2)** → wired **USB keyboard** (HID) for on-device input.
4. **SDIO + brcmfmac firmware** → WiFi MAC, then a WPA2 supplicant.
5. **UART HCI Bluetooth** (firmware patch → L2CAP → HID) for BT keyboard/mouse.

Items 3–5 are large, multi-session efforts and are not started.

---

## 🧱 Phase history

- **Phase 1** Microkernel boot, UART, vector table, syscall trap scaffolding
- **Phase 2** Real-time scheduler (priority + round-robin, deadlines, sleep/yield)
- **Phase 3** Signal handling + user I/O syscalls *(EL0 runtime blocked)*
- **Phase 4** Ethernet + IPv4 + UDP + TCP stack
- **Phase 4.4** Multithreaded RX/TX engine with lockless queues
- **Phase 5** User apps: TCP echo, sensor task, CLI shell *(EL0 runtime blocked)*
- **Phase 6** Zero-copy networking with DMA RX/TX rings (GENET MMIO)
- **Phase 7** VFS + FAT32 (read/write, subdirs, cluster chains) on simulated SD

---

## 🙌 Credits

Designed and implemented by **Shinit Dinesh Shetty**
Target: Raspberry Pi 4 · Toolchain: `aarch64-none-elf-gcc`, QEMU, `make`

## 📎 License

MIT — use, modify, and share freely.
