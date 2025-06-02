
# 🛠 RealNetOS – A Microkernel-based Real-Time Embedded Operating System

**RealNetOS** is a custom-built operating system designed from scratch for embedded platforms like the Raspberry Pi 4. It follows a microkernel architecture and includes real-time scheduling, multithreaded networking, user-space services, and a minimal virtual file system with FAT32 support.

---

## 📦 Project Structure

```
/
├── kernel/              # Core kernel logic (threads, scheduler, syscalls)
├── net/                 # Networking layers (Ethernet, IP, UDP, TCP, DMA)
├── user/                # User-space programs (TCP echo server, CLI shell)
├── fs/                  # VFS, FAT32 file system logic
├── block/               # Simulated SD card block driver
├── include/             # Shared headers for kernel, drivers, user apps
└── src/lib.c            # Minimal C library (memcpy, itoa, string utils)
```

---

## 🚀 Phased Development Breakdown

### ✅ Phase 1: Boot & Microkernel Core
- **Objective**: Bring up the board and implement a basic microkernel with interrupt handling.
- **Key Components**:
  - Startup code in `start.S`
  - UART initialization for serial I/O
  - Syscall mechanism using `svc #0`
  - Minimal thread management and IPC
- **Highlights**: Manual context switching, vector table setup, trap handling.

---

### ✅ Phase 2: Real-Time Thread Scheduling
- **Objective**: Enable multitasking with real-time guarantees.
- **Features Implemented**:
  - Preemptive Round-Robin + Priority-based scheduling
  - Sleep and Yield primitives
  - Deadline-aware task queue
- **Files**: `scheduler.c`, `threads.c`, `syscall.c`

---

### ✅ Phase 3: Signal Handling & User-mode I/O
- **Objective**: Extend user-mode capabilities.
- **Implemented**:
  - Signal registration and dispatch
  - `sys_signal`, `sys_kill`, and signal handler trampoline
  - `sys_write` and `sys_read` for UART I/O
- **Demo**: User1 registers a handler and responds to signals sent by User2.

---

### ✅ Phase 4: Networking Stack (Ethernet + IP + UDP + TCP)
- **Goal**: Full custom-built network stack with protocol layering.
- **Implemented Protocols**:
  - `ethernet.c/h`: Framing and MAC layer
  - `ip.c/h`: Basic IPv4 header handling, checksum, routing
  - `udp.c/h`: Port demux, simple `send/recv`
  - `tcp.c/h`: Handshake (SYN/ACK), sequence numbers, send/recv buffers
- **Testing**: Verified using TCP echo server and UDP message exchange.

---

### ✅ Phase 4.4: Multithreaded RX/TX Engine
- **Objective**: Parallelize RX and TX processing using kernel threads.
- **Features**:
  - Dedicated RX and TX threads polling DMA rings
  - Lockless queues for packet exchange
  - Interrupt-safe scheduling
- **Impact**: Ensures packet processing does not block user threads.

---

### ✅ Phase 5: User-Space Applications

#### ✅ 5.1 TCP Echo Server
- **Implements**: A user-mode program listening over TCP and echoing responses.
- **Syscalls Used**: `sys_tcp_connect`, `sys_tcp_send`, `sys_tcp_receive`

#### ✅ 5.2 Sensor Polling Task
- **Simulates**: Periodic sensor reads with formatted string output via UART.
- **Tools**: `itoa`, `sys_write`, `sys_sleep`

#### ✅ 5.3 CLI Shell
- **Implements**: A user-space command loop (minimal shell).
- **Commands Supported**: `time`, `echo <msg>`, `help`
- **Supports**: Token parsing, string handling with `strtok`, `strchr`, `strcmp`

---

### ✅ Phase 6: Zero-Copy Networking
- **Objective**: Improve performance using DMA-backed buffer rings.
- **Files**: `genet_dma.c/h`, `dma.c/h`
- **Architecture**:
  - **DMA RX/TX Rings** aligned in memory
  - **Buffer Recycling**: On receiving a packet, the buffer is reused or reallocated
  - **No intermediate copies**: DMA writes directly to aligned ring buffers
- **Hardware Mapping**: MMIO registers mapped for GENET Ethernet on Raspberry Pi 4

---

### ✅ Phase 7: File System Support

#### ✅ 7.1 Virtual File System (VFS)
- **Implements**: An abstract VFS with:
  - `vnode`, `file`, `filesystem`, `file_ops` structs
  - `vfs_open`, `vfs_close`, `vfs_read`, `vfs_write`
  - Dynamic registration of file systems

#### ✅ 7.2 FAT32 File System on SD (Simulated)
- **Implements**: FAT32 parsing using `fat32.c` and `sd.c`
- **Block Device Layer**: Provides `sd_read_block` and `sd_write_block`
- **VFS Integration**: FAT32 registered as a filesystem and mounted on boot
- **Read Support**: Root directory listing, file block parsing, cluster reading
- **Future Work**: FAT write support, journaling FS integration

---

## 📌 Build & Run Instructions

```bash
make clean
make
Run on QEMU or Raspberry Pi 4
```

---

## ✅ Milestones

| Phase | Title                             | Status     |
|-------|-----------------------------------|------------|
| 1     | Microkernel Boot                  | ✅ Complete |
| 2     | Real-Time Scheduling              | ✅ Complete |
| 3     | User I/O & Signal Handling        | ✅ Complete |
| 4     | Ethernet + TCP/IP Stack           | ✅ Complete |
| 4.4   | Multithreaded RX/TX               | ✅ Complete |
| 5     | User Applications                 | ✅ Complete |
| 6     | Zero-Copy Networking with DMA     | ✅ Complete |
| 7     | VFS + FAT32 File System           | ✅ Complete |

---

## 🙌 Credits

Designed and implemented by **Shinit Dinesh Shetty**  
Target Platform: Raspberry Pi 4  
Toolchain: `aarch64-none-elf-gcc`, `QEMU`, `make`

---

## 📎 License

MIT License — Use, modify, and share freely.
