SECTIONS
{
  . = 0x4000;
  .text : { *(init) *(code) *(.text) }
  .data : { *(dat) *(data) *(.data) }
  .bss : { *(bss) *(.bss) }
}
