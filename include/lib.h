#pragma once
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n);
void mini_snprintf(char* buf, int value);
void int_to_str(int value, char* str);