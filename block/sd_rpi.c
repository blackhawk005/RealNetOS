#include "../include/block/sd.h"
#include "../include/uart.h"
#include "../include/lib.h"
#include "../include/mailbox.h"

// Minimal SDHCI driver for Raspberry Pi 4 (BCM2711).
// This uses the EMMC2 SDHCI controller at 0xFE340000.

#define SDHCI_BASE 0xFE340000UL

#define MMIO32(addr) (*(volatile uint32_t*)(uintptr_t)(addr))
#define MMIO16(addr) (*(volatile uint16_t*)(uintptr_t)(addr))
#define MMIO8(addr)  (*(volatile uint8_t*)(uintptr_t)(addr))

#define SDHCI_DMA_ADDRESS    (SDHCI_BASE + 0x00)
#define SDHCI_BLKSIZECNT     (SDHCI_BASE + 0x04)
#define SDHCI_ARG1           (SDHCI_BASE + 0x08)
#define SDHCI_CMDTM          (SDHCI_BASE + 0x0C)
#define SDHCI_RESP0          (SDHCI_BASE + 0x10)
#define SDHCI_RESP1          (SDHCI_BASE + 0x14)
#define SDHCI_RESP2          (SDHCI_BASE + 0x18)
#define SDHCI_RESP3          (SDHCI_BASE + 0x1C)
#define SDHCI_DATA           (SDHCI_BASE + 0x20)
#define SDHCI_STATUS         (SDHCI_BASE + 0x24)
#define SDHCI_HOST_CTRL      (SDHCI_BASE + 0x28)
#define SDHCI_POWER_CTRL     (SDHCI_BASE + 0x29)
#define SDHCI_CLOCK_CTRL     (SDHCI_BASE + 0x2C)
#define SDHCI_TIMEOUT_CTRL   (SDHCI_BASE + 0x2E)
#define SDHCI_SW_RESET       (SDHCI_BASE + 0x2F)
#define SDHCI_INT_STATUS     (SDHCI_BASE + 0x30)
#define SDHCI_INT_ENABLE     (SDHCI_BASE + 0x34)
#define SDHCI_INT_SIGNAL     (SDHCI_BASE + 0x38)
#define SDHCI_CAPABILITIES   (SDHCI_BASE + 0x40)
#define SDHCI_CAPABILITIES_1 (SDHCI_BASE + 0x44)
#define SDHCI_HOST_VER       (SDHCI_BASE + 0xFE)

#define CMD_RSP_NONE 0
#define CMD_RSP_136  1
#define CMD_RSP_48   2
#define CMD_RSP_48B  3

#define INT_CMD_DONE   (1u << 0)
#define INT_DATA_DONE  (1u << 1)
#define INT_ERROR      (1u << 15)

#define STATUS_CMD_INHIBIT (1u << 0)
#define STATUS_DAT_INHIBIT (1u << 1)
#define STATUS_READ_READY  (1u << 11)
#define STATUS_WRITE_READY (1u << 10)
#define STATUS_WRITE_PROTECT (1u << 19)

#define XFER_DMA_ENABLE   (1u << 0)
#define XFER_BLKCNT_EN    (1u << 1)
#define XFER_AUTO_CMD12   (1u << 2)
#define XFER_READ         (1u << 4)
#define XFER_MULTI        (1u << 5)

#define SDHCI_DEBUG 1

static uint32_t sd_rca = 0;
static int sd_is_sdhc = 0;

static volatile uint32_t mbox[9] __attribute__((aligned(16)));

static void sdhci_dbg(const char* msg) {
#if SDHCI_DEBUG
    uart_puts(msg);
#else
    (void)msg;
#endif
}

static void sdhci_dbg_hex(const char* msg, uint32_t v) {
#if SDHCI_DEBUG
    uart_puts(msg);
    uart_puthex64((uint64_t)v);
    uart_puts("\n");
#else
    (void)msg;
    (void)v;
#endif
}

static void delay_cycles(volatile uint32_t c) {
    while (c--) { asm volatile("nop"); }
}

static int sdhci_enable_clock_id(uint32_t clk_id) {
    // Tag 0x38001 = Set clock state, Tag 0x38002 = Set clock rate.
    mbox[0] = 9 * 4;
    mbox[1] = 0;
    mbox[2] = 0x00038002; // set clock rate
    mbox[3] = 12;
    mbox[4] = 12;
    mbox[5] = clk_id;
    mbox[6] = 50000000;   // 50 MHz base
    mbox[7] = 0;          // skip turbo
    mbox[8] = 0;
    if (!mbox_call(8, mbox)) return -1;

    mbox[0] = 8 * 4;
    mbox[1] = 0;
    mbox[2] = 0x00038001; // set clock state
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = clk_id;
    mbox[6] = 1;          // on
    mbox[7] = 0;
    return mbox_call(8, mbox) ? 0 : -1;
}

static int sdhci_enable_power_id(uint32_t dev_id) {
    // Tag 0x28001 = Set power state
    mbox[0] = 8 * 4;
    mbox[1] = 0;
    mbox[2] = 0x00028001;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = dev_id;
    mbox[6] = 1; // on
    mbox[7] = 0;
    return mbox_call(8, mbox) ? 0 : -1;
}

static int sdhci_reset(void) {
    MMIO8(SDHCI_SW_RESET) = 0x01;
    for (uint32_t t = 0; t < 10000000; t++) {
        if ((MMIO8(SDHCI_SW_RESET) & 0x01) == 0) return 0;
    }
    return -1;
}

static void sdhci_set_clock_hz(uint32_t hz) {
    uint16_t ctrl = MMIO16(SDHCI_CLOCK_CTRL);
    ctrl &= ~(1u << 2); // disable SD clock
    MMIO16(SDHCI_CLOCK_CTRL) = ctrl;

    uint32_t cap0 = MMIO32(SDHCI_CAPABILITIES);
    uint32_t base_mhz = (cap0 >> 8) & 0xFF;
    if (base_mhz == 0) base_mhz = 100; // fallback
    uint32_t base_hz = base_mhz * 1000000u;
    uint32_t div = (base_hz + (hz * 2 - 1)) / (hz * 2);
    if (div == 0) div = 1;
    if (div > 0x3FF) div = 0x3FF;

    uint16_t div_lo = (uint16_t)((div & 0xFF) << 8);
    uint16_t div_hi = (uint16_t)((div & 0x300) >> 2);

    ctrl = MMIO16(SDHCI_CLOCK_CTRL);
    ctrl &= ~((uint16_t)0xFF00u | (uint16_t)(0x3u << 6));
    ctrl |= div_lo | div_hi;
    ctrl |= (1u << 0); // internal clock enable
    MMIO16(SDHCI_CLOCK_CTRL) = ctrl;

    for (uint32_t t = 0; t < 1000000; t++) {
        if (MMIO16(SDHCI_CLOCK_CTRL) & (1u << 1)) break;
    }
    if (!(MMIO16(SDHCI_CLOCK_CTRL) & (1u << 1))) {
        sdhci_dbg("SDHCI: internal clock not stable\n");
    }
    MMIO16(SDHCI_CLOCK_CTRL) |= (1u << 2); // SD clock enable
}

static int sdhci_wait_clear(uint32_t reg, uint32_t mask, uint32_t timeout) {
    while (timeout--) {
        if ((MMIO32(reg) & mask) == 0) return 0;
    }
    return -1;
}

static int sdhci_wait_set(uint32_t reg, uint32_t mask, uint32_t timeout) {
    while (timeout--) {
        if ((MMIO32(reg) & mask) == mask) return 0;
    }
    return -1;
}

static int sdhci_send_cmd(uint32_t cmd, uint32_t arg, int resp_type, int data, int read, int multi, int auto_cmd12) {
    if (sdhci_wait_clear(SDHCI_STATUS, STATUS_CMD_INHIBIT, 1000000) != 0) return -1;
    if (data && sdhci_wait_clear(SDHCI_STATUS, STATUS_DAT_INHIBIT, 1000000) != 0) return -1;

    MMIO32(SDHCI_INT_STATUS) = 0xFFFFFFFF;
    MMIO32(SDHCI_ARG1) = arg;

    uint32_t cmdtm = 0;
    cmdtm |= (cmd & 0x3F) << 24;
    if (data) {
        cmdtm |= (1u << 21); // data present
        if (read) cmdtm |= XFER_READ;
        cmdtm |= XFER_BLKCNT_EN;
        if (multi) cmdtm |= XFER_MULTI;
        if (auto_cmd12) cmdtm |= XFER_AUTO_CMD12;
    }
    switch (resp_type) {
        case CMD_RSP_136: cmdtm |= (1u << 16); break;
        case CMD_RSP_48:  cmdtm |= (1u << 17); break;
        case CMD_RSP_48B: cmdtm |= (1u << 16) | (1u << 17); break;
        default: break;
    }
    if (resp_type != CMD_RSP_NONE) {
        // Disable CRC/index checks during init to avoid spurious errors.
        // We'll tighten this once init works.
    }

    MMIO32(SDHCI_CMDTM) = cmdtm;

    if (sdhci_wait_set(SDHCI_INT_STATUS, INT_CMD_DONE, 1000000) != 0) {
        uart_puts("SDHCI: CMD timeout\n");
        sdhci_dbg_hex("SDHCI: INT_STATUS=0x", MMIO32(SDHCI_INT_STATUS));
        sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
        return -1;
    }
    uint32_t status = MMIO32(SDHCI_INT_STATUS);
    if (status & INT_ERROR) {
        uart_puts("SDHCI: CMD error\n");
        sdhci_dbg_hex("SDHCI: INT_STATUS=0x", status);
        sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
        return -1;
    }
    return 0;
}

static int sdhci_stop_transmission(void) {
    return sdhci_send_cmd(12, 0, CMD_RSP_48B, 0, 0, 0, 0);
}

static int sdhci_wait_ready(void) {
    // Poll card status (CMD13) until ready-for-data and in TRAN state.
    for (int i = 0; i < 1000; i++) {
        if (sdhci_send_cmd(13, sd_rca, CMD_RSP_48, 0, 0, 0, 0) != 0) {
            return -1;
        }
        uint32_t r1 = MMIO32(SDHCI_RESP0);
        uint32_t state = (r1 >> 9) & 0xF;
        uint32_t ready = (r1 >> 8) & 0x1; // READY_FOR_DATA is bit 8
        if (ready && state == 4) return 0; // TRAN
        delay_cycles(10000);
    }
    uart_puts("SDHCI: card not ready for data\n");
    sdhci_dbg_hex("SDHCI: CMD13 R1=0x", MMIO32(SDHCI_RESP0));
    return -1;
}

static int sdhci_read_blocks(uint32_t lba, uint8_t* buffer, uint32_t count) {
    uint32_t addr = sd_is_sdhc ? lba : (lba * 512);
    MMIO32(SDHCI_BLKSIZECNT) = (512 & 0xFFF) | (count << 16);

    uint32_t cmd = (count > 1) ? 18 : 17;
    if (sdhci_send_cmd(cmd, addr, CMD_RSP_48, 1, 1, count > 1, count > 1) != 0) return -1;

    uint32_t words = count * 128;
    for (uint32_t i = 0; i < words; i++) {
        if (sdhci_wait_set(SDHCI_STATUS, STATUS_READ_READY, 1000000) != 0) return -1;
        uint32_t w = MMIO32(SDHCI_DATA);
        buffer[i * 4 + 0] = (uint8_t)(w & 0xFF);
        buffer[i * 4 + 1] = (uint8_t)((w >> 8) & 0xFF);
        buffer[i * 4 + 2] = (uint8_t)((w >> 16) & 0xFF);
        buffer[i * 4 + 3] = (uint8_t)((w >> 24) & 0xFF);
    }

    if (sdhci_wait_set(SDHCI_INT_STATUS, INT_DATA_DONE, 1000000) != 0) return -1;
    MMIO32(SDHCI_INT_STATUS) = INT_DATA_DONE;
    if (count > 1) sdhci_stop_transmission();
    return 0;
}

static int sdhci_write_blocks(uint32_t lba, const uint8_t* buffer, uint32_t count) {
    uint32_t addr = sd_is_sdhc ? lba : (lba * 512);
    if (sdhci_wait_ready() != 0) return -1;
    // Clear any pending interrupts before write
    MMIO32(SDHCI_INT_STATUS) = 0xFFFFFFFF;
    MMIO32(SDHCI_BLKSIZECNT) = (512 & 0xFFF) | (count << 16);

    uint32_t cmd = (count > 1) ? 25 : 24;
    if (sdhci_send_cmd(cmd, addr, CMD_RSP_48, 1, 0, count > 1, count > 1) != 0) return -1;

    uint32_t words = count * 128;
    for (uint32_t i = 0; i < words; i++) {
        if (sdhci_wait_set(SDHCI_STATUS, STATUS_WRITE_READY, 1000000) != 0) return -1;
        uint32_t w = buffer[i * 4 + 0] |
                     (buffer[i * 4 + 1] << 8) |
                     (buffer[i * 4 + 2] << 16) |
                     (buffer[i * 4 + 3] << 24);
        MMIO32(SDHCI_DATA) = w;
    }

    if (sdhci_wait_set(SDHCI_INT_STATUS, INT_DATA_DONE, 1000000) != 0) return -1;
    MMIO32(SDHCI_INT_STATUS) = INT_DATA_DONE;
    if (count > 1) sdhci_stop_transmission();
    return 0;
}

int sd_init(void) {
    uart_puts("SDHCI: init (RPi)\n");

    sdhci_enable_power_id(0x0000000C);
    sdhci_enable_power_id(0x00000001);
    sdhci_enable_clock_id(0x0000000C);
    sdhci_enable_clock_id(0x00000001);

    if (sdhci_reset() != 0) {
        uart_puts("SDHCI: reset failed\n");
        return -1;
    }

    MMIO8(SDHCI_HOST_CTRL) = 0x0; // 1-bit
    MMIO8(SDHCI_POWER_CTRL) = 0x0F; // 3.3V power on
    MMIO8(SDHCI_TIMEOUT_CTRL) = 0x0E;
    MMIO32(SDHCI_INT_ENABLE) = 0xFFFFFFFF;
    MMIO32(SDHCI_INT_SIGNAL) = 0x0;
    MMIO32(SDHCI_INT_STATUS) = 0xFFFFFFFF;

    sdhci_dbg_hex("SDHCI: CAP0=0x", MMIO32(SDHCI_CAPABILITIES));
    sdhci_dbg_hex("SDHCI: CAP1=0x", MMIO32(SDHCI_CAPABILITIES_1));
    sdhci_dbg_hex("SDHCI: HOST_VER=0x", MMIO16(SDHCI_HOST_VER));
    sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
    sdhci_dbg_hex("SDHCI: INT_STATUS=0x", MMIO32(SDHCI_INT_STATUS));

    sdhci_set_clock_hz(400000);
    delay_cycles(2000000);

    if (sdhci_send_cmd(0, 0, CMD_RSP_NONE, 0, 0, 0, 0) != 0) {
        uart_puts("SDHCI: CMD0 failed\n");
        sdhci_dbg_hex("SDHCI: INT_STATUS=0x", MMIO32(SDHCI_INT_STATUS));
        sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
        return -1;
    }

    int supports_v2 = 0;
    if (sdhci_send_cmd(8, 0x1AA, CMD_RSP_48, 0, 0, 0, 0) == 0) {
        uint32_t r7 = MMIO32(SDHCI_RESP0);
        if ((r7 & 0xFF) == 0xAA && ((r7 >> 8) & 0xF) == 0x1) supports_v2 = 1;
    }

    uint32_t ocr = 0;
    for (int i = 0; i < 2000; i++) {
        if (sdhci_send_cmd(55, 0, CMD_RSP_48, 0, 0, 0, 0) != 0) {
            uart_puts("SDHCI: CMD55 failed\n");
            return -1;
        }
        uint32_t ocr_arg = supports_v2 ? 0x40FF8000u : 0x00FF8000u;
        if (sdhci_send_cmd(41, ocr_arg, CMD_RSP_48, 0, 0, 0, 0) != 0) {
            uart_puts("SDHCI: ACMD41 failed\n");
            return -1;
        }
        ocr = MMIO32(SDHCI_RESP0);
        if (ocr & (1u << 31)) break;
        delay_cycles(200000);
    }
    if (!(ocr & (1u << 31))) {
        uart_puts("SDHCI: ACMD41 timeout\n");
        return -1;
    }
    sd_is_sdhc = (ocr & (1u << 30)) ? 1 : 0;

    if (sdhci_send_cmd(2, 0, CMD_RSP_136, 0, 0, 0, 0) != 0) return -1;
    if (sdhci_send_cmd(3, 0, CMD_RSP_48, 0, 0, 0, 0) != 0) return -1;
    sd_rca = MMIO32(SDHCI_RESP0) & 0xFFFF0000;

    if (sdhci_send_cmd(7, sd_rca, CMD_RSP_48B, 0, 0, 0, 0) != 0) return -1;
    if (!sd_is_sdhc) {
        if (sdhci_send_cmd(16, 512, CMD_RSP_48, 0, 0, 0, 0) != 0) return -1;
    }

    // Keep a conservative clock for now to stabilize reads.
    sdhci_set_clock_hz(1000000);

    uart_puts("SDHCI: init ok\n");
    return 0;
}

int sd_read_block(uint32_t lba, uint8_t* buffer) {
    int rc = sdhci_read_blocks(lba, buffer, 1);
    if (rc != 0) {
        // Retry once at low clock to recover from transient timeouts.
        sdhci_set_clock_hz(1000000);
        rc = sdhci_read_blocks(lba, buffer, 1);
    }
    if (rc != 0) {
        uart_puts("SDHCI: read block failed LBA=");
        uart_puthex64((uint64_t)lba);
        uart_puts("\n");
        sdhci_dbg_hex("SDHCI: INT_STATUS=0x", MMIO32(SDHCI_INT_STATUS));
        sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
    }
    return rc;
}

int sd_write_block(uint32_t lba, const uint8_t* buffer) {
    // Ignore write-protect status (some adapters misreport it).
    // Use a very low clock for writes to improve reliability.
    sdhci_set_clock_hz(400000);
    // Reset CMD/DAT lines before write
    MMIO8(SDHCI_SW_RESET) = 0x06;
    for (uint32_t t = 0; t < 1000000; t++) {
        if ((MMIO8(SDHCI_SW_RESET) & 0x06) == 0) break;
    }
    int rc = sdhci_write_blocks(lba, buffer, 1);
    if (rc != 0) {
        // Retry once
        rc = sdhci_write_blocks(lba, buffer, 1);
        if (rc != 0) {
            sdhci_dbg_hex("SDHCI: STATUS=0x", MMIO32(SDHCI_STATUS));
            sdhci_dbg_hex("SDHCI: INT_STATUS=0x", MMIO32(SDHCI_INT_STATUS));
        }
    }
    return rc;
}

int sd_read_blocks(uint32_t lba, uint8_t* buffer, uint32_t count) {
    return sdhci_read_blocks(lba, buffer, count);
}

int sd_write_blocks(uint32_t lba, const uint8_t* buffer, uint32_t count) {
    return sdhci_write_blocks(lba, buffer, count);
}
