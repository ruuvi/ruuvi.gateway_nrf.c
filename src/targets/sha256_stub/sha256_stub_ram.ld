/* sha256_stub_ram.ld
 *
 * Absolute fixed addresses for SWD protocol structs:
 *   .stub_params  @ 0x20000000
 *   .stub_result  @ 0x20000080
 *
 * Code/data/bss live in a separate RAM window to avoid clobbering params/results.
 */

MEMORY
{
  /* Small "metadata" window reserved for protocol structs written/read by host */
  META   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x0100   /* 256 bytes */

  /* Stub code + data window (your original 8 KiB) */
  STUBRAM (rwx) : ORIGIN = 0x20001000, LENGTH = 0x2000  /* 8 KiB */
}

/* Fixed stack top that works even on nRF52811 (24 KiB RAM => top 0x20006000) */
__stack_top__ = 0x20006000;
__stack_size__ = 0x800;            /* 2 KiB */
__stack_bottom__ = __stack_top__ - __stack_size__;

SECTIONS
{
  /* --- Fixed protocol structures (absolute addresses) --- */
  .stub_params 0x20000000 (NOLOAD) : {
    . = ALIGN(4);
    KEEP(*(.stub_params))
    . = ALIGN(4);
  } > META

  .stub_result 0x20000080 (NOLOAD) : {
    . = ALIGN(4);
    KEEP(*(.stub_result))
    . = ALIGN(4);
  } > META

  /* Optional sanity: ensure both symbols fit inside META range */
  ASSERT( (0x20000080 + SIZEOF(.stub_result)) <= (ORIGIN(META) + LENGTH(META)),
          "META region overflow: .stub_result does not fit" )

  /* --- Stub code / rodata --- */
  .text : {
    . = ALIGN(4);
    KEEP(*(.text.stub_reset))   /* if you use an asm shim */
    KEEP(*(.text.stub_entry))   /* main entry */
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > STUBRAM

  /* --- Stub data / bss --- */
  .data : {
    . = ALIGN(4);
    *(.data*)
    . = ALIGN(4);
  } > STUBRAM

  .bss (NOLOAD) : {
    . = ALIGN(4);
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
  } > STUBRAM

  . = ALIGN(8);
  __stub_end__ = .;

  /* Ensure stub image fits into STUBRAM */
  ASSERT(__stub_end__ <= ORIGIN(STUBRAM) + LENGTH(STUBRAM), "STUBRAM overflow")

  /* Ensure fixed stack is inside worst-case RAM (nRF52811: 0x20000000..0x20005FFF) */
  ASSERT(__stack_top__ <= 0x20006000, "stack_top invalid for nRF52811 worst-case RAM")
  ASSERT(__stack_bottom__ > __stub_end__, "stack_bottom below __stub_end__")

  /DISCARD/ : { *(.comment*) *(.note*) }
}
