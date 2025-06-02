#pragma once
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n);
void mini_snprintf(char* buf, int value);
void int_to_str(int value, char* str);
int strcmp(const char* s1, const char* s2);
void int_to_buf(unsigned long value, char* str);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strtok(char* str, const char* delim);
char* strchr(const char* str, int c);
void* kmalloc(size_t size);
void* memset(void* s, int c, size_t n);