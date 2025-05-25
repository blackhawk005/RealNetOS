ENTRY(_start)

SECTIONS {
  . = 0x80000;

  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  _end = .;
  .bss (NOLOAD) : { *(.bss COMMON) }
}