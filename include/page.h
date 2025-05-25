#ifndef PAGE_H
#define PAGE_H

void page_allocator_init();
void* alloc_page();
void free_page(void* addr);

#endif