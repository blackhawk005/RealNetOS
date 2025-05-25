#include "../include/mem.h"

extern unsigned long __heap_start;
extern unsigned long __heap_end;

static unsigned long heap_ptr = 0;

void* kmalloc(unsigned long size){
    if (!heap_ptr){
        heap_ptr = (unsigned long)& __heap_start;
    }

    // Align size to 16 bytes
    size = (size + 15) & ~15;

    if (heap_ptr + size > (unsigned long)&__heap_end){
        return 0;   // out of memory
    }

    void* mem = (void*)heap_ptr;

    heap_ptr += size;

    return mem;
}