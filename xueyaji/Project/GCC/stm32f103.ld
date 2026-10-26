ENTRY(Reset_Handler)
MEMORY {
 FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
 RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
_stack_limit = _estack - 4096;
SECTIONS {
 .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
 .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
 .ARM.extab : { *(.ARM.extab*) } > FLASH
 .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH
 .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT> FLASH
 _sidata = LOADADDR(.data);
 .bss (NOLOAD) : { . = ALIGN(8); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(8); _ebss = .; } > RAM
 .heap (NOLOAD) : { _heap_start = .; . += 2048; _heap_end = .; } > RAM
 ASSERT(_heap_end <= _stack_limit, "RAM overflow: preserve 4KB stack")
}
