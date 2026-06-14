ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .text : { *(.text*) }

  .rodata : { *(.rodata*) }

  .data : { *(.data*) }

  __bss_start = .;
  .bss (NOLOAD) : { *(.bss COMMON) }
  __bss_end = .;

  . = ALIGN(16);
  __heap_start = .;
  . = . + 0x00100000;   /* Reserve 1 MB heap */
  __heap_end = .;

  . = ALIGN(16);
  __stack_bottom = .;
  . = . + 0x00010000;   /* 64 KB kernel stack */
  __stack_top = .;

  _end = .;

  }
