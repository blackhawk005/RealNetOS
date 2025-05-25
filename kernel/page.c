#include "../include/page.h"

#define PAGE_SIZE 4096
#define TOTAL_PAGES 256 // 256 pages = 1 MB

static unsigned char page_bitmap[TOTAL_PAGES];   // 1 byte per page (free/used)
static unsigned long page_base = 0;

extern unsigned long __heap_end;

void page_allocator_init(){
    page_base = (unsigned long)&__heap_end;

    // Clear Bitmap
    for (int i = 0; i < TOTAL_PAGES; i++){
        page_bitmap[i] = 0;
    }
}

void* alloc_page(){
    for (int i = 0; i < TOTAL_PAGES; i++){
        if (page_bitmap[i] == 0){
            page_bitmap[i] = 1;
            return (void*)(page_base + i * PAGE_SIZE);
        }
    }
    return 0;
}

void free_page(void* addr){
    unsigned long offset = (unsigned long)addr - page_base;
    unsigned long index = offset / PAGE_SIZE;
    if (index < TOTAL_PAGES){
        page_bitmap[index] = 0;
    }
}
