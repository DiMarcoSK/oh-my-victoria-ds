ENTRY(main)
SECTIONS {
 . = 0x02000000;
 .text : { *(.text*) *(.itcm*) *(.glue_7*) *(.v4_bx) *(.vfp11_veneer) *(.rodata*) }
 .ARM.exidx : { *(.ARM.exidx*) }
 .data : { *(.data*) }
 .bss : { *(.bss*) *(COMMON) }
 _end = .; end = .;
}
