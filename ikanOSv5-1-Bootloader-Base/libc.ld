SECTIONS
{
  . = 0x700000;  /* Fixed base address for absolute pointers */
  .jumptable : { *(.jumptable) }  /* Jump table first */
  .text : { *(.text) }  /* Code next */
  .rodata : { *(.rodata) }  /* Read-only data */
  .data : { *(.data) }  /* Writable data */
  .bss : { *(.bss) }  /* Zero-init data (not in file) */
}