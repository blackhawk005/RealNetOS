#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n){
    char* d = (char*)dest;
    const char* s = (const char*) src;

    while (n--){
        *d++ = *s++;
    }
    return dest;
}

// Minimal itoa (integer to string) implementation
void int_to_str(int value, char* str) {
    int i = 0, j, sign = 0;
    char temp[16];

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (value < 0) {
        sign = 1;
        value = -value;
    }

    while (value != 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    if (sign) {
        temp[i++] = '-';
    }

    // Reverse
    for (j = 0; j < i; j++) {
        str[j] = temp[i - j - 1];
    }

    str[j] = '\0';
}