#pragma once
#include <stdint.h>

int fb_init(uint32_t width, uint32_t height);
void fb_putc(char c);
void fb_puts(const char* s);
void fb_set_color(uint32_t fg, uint32_t bg);
int fb_is_ready(void);
