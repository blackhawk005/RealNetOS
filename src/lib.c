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

int strcmp(const char* s1, const char* s2){
    while (*s1 && (*s1 == *s2)){
        s1++;
        s2++;
    }

    return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

void int_to_buf(unsigned long value, char* str){
    int i = 0;
    char temp[20];

    if (value == 0){
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (value != 0){
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    for (int j = 0; j < i; j++){
        str[j] = temp[i - j - 1];
    }

    str[i] = '\0';
}

char* strcpy(char* dest, const char* src){
    char* r = dest;
    while ((*dest++ = *src++));
    return r;
}

char* strcat(char* dest, const char* src){
    char* r = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return r;
}