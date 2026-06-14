#include "../include/fb.h"
#include "../include/mailbox.h"
#include "../include/lib.h"
#include "font8x8_basic.h"

// Glyph scale factor: each 8x8 font cell is drawn as (8*FB_SCALE) px.
// Scale 2 keeps boot text readable when captured on a TV/monitor for video.
#define FB_SCALE 2
#define GLYPH    (8 * FB_SCALE)

static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint32_t* fb_ptr;
static uint32_t fb_fg = 0xFFFFFFFF;
static uint32_t fb_bg = 0x00000000;
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static int fb_ready = 0;

static volatile uint32_t mbox[36] __attribute__((aligned(16)));

int fb_is_ready(void) {
    return fb_ready;
}

void fb_set_color(uint32_t fg, uint32_t bg) {
    fb_fg = fg;
    fb_bg = bg;
}

static void fb_clear(void) {
    if (!fb_ready) return;
    uint32_t total = (fb_pitch * fb_height) / 4;
    for (uint32_t i = 0; i < total; i++) {
        fb_ptr[i] = fb_bg;
    }
    cursor_x = 0;
    cursor_y = 0;
}

int fb_init(uint32_t width, uint32_t height) {
    mbox[0] = 35 * 4;
    mbox[1] = 0;

    mbox[2] = 0x48003; // set physical width/height
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = width;
    mbox[6] = height;

    mbox[7] = 0x48004; // set virtual width/height
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = width;
    mbox[11] = height;

    mbox[12] = 0x48005; // set depth
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = 32;

    mbox[16] = 0x48006; // set pixel order (RGB)
    mbox[17] = 4;
    mbox[18] = 4;
    mbox[19] = 1;

    mbox[20] = 0x40001; // allocate buffer
    mbox[21] = 8;
    mbox[22] = 8;
    mbox[23] = 16;
    mbox[24] = 0;

    mbox[25] = 0x40008; // get pitch
    mbox[26] = 4;
    mbox[27] = 4;
    mbox[28] = 0;

    mbox[29] = 0;

    if (!mbox_call(8, mbox)) {
        fb_ready = 0;
        return -1;
    }

    fb_width = mbox[5];
    fb_height = mbox[6];
    fb_pitch = mbox[28];
    uint32_t fb_addr = mbox[23] & 0x3FFFFFFF;
    fb_ptr = (uint32_t*)(uintptr_t)fb_addr;

    if (fb_ptr == 0 || fb_pitch == 0) {
        fb_ready = 0;
        return -1;
    }

    fb_ready = 1;
    fb_clear();
    return 0;
}

static void fb_scroll(void) {
    if (!fb_ready) return;
    uint32_t row_bytes = fb_pitch * GLYPH;
    uint32_t total_bytes = fb_pitch * fb_height;
    uint8_t* fb = (uint8_t*)fb_ptr;

    for (uint32_t i = 0; i < total_bytes - row_bytes; i++) {
        fb[i] = fb[i + row_bytes];
    }
    for (uint32_t i = total_bytes - row_bytes; i < total_bytes; i++) {
        fb[i] = 0;
    }
    if (cursor_y > 0) cursor_y--;
}

void fb_putc(char c) {
    if (!fb_ready) return;
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if ((cursor_y + 1) * GLYPH > fb_height) fb_scroll();
        return;
    }
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        return;
    }

    uint8_t ch = (uint8_t)c;
    if (ch > 127) ch = '?';
    uint32_t px = cursor_x * GLYPH;
    uint32_t py = cursor_y * GLYPH;
    for (uint32_t row = 0; row < 8; row++) {
        uint8_t bits = font8x8_basic[ch][row];
        for (uint32_t sy = 0; sy < FB_SCALE; sy++) {
            uint32_t* dst = (uint32_t*)((uint8_t*)fb_ptr
                          + (py + row * FB_SCALE + sy) * fb_pitch + px * 4);
            for (uint32_t col = 0; col < 8; col++) {
                // Font bitmaps are stored MSB-left; flip bit lookup to avoid mirror output.
                uint32_t color = (bits & (1u << (7 - col))) ? fb_fg : fb_bg;
                for (uint32_t sx = 0; sx < FB_SCALE; sx++) {
                    dst[col * FB_SCALE + sx] = color;
                }
            }
        }
    }

    cursor_x++;
    if ((cursor_x + 1) * GLYPH > fb_width) {
        cursor_x = 0;
        cursor_y++;
        if ((cursor_y + 1) * GLYPH > fb_height) fb_scroll();
    }
}

void fb_puts(const char* s) {
    if (!fb_ready) return;
    while (*s) {
        fb_putc(*s++);
    }
}
