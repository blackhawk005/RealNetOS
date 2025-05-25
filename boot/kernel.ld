ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .text : { *(.text*) }

  .rodata : { *(.rodata*) }

  .data : { *(.data*) }

  _end = .;

  .bss (NOLOAD) : { *(.bss COMMON) }
  
  . = ALIGN(16);
  __heap_start = .;
  . = . + 0x00100000;   /* Reserve 1 MB heap */
  __heap_end = .;

  }