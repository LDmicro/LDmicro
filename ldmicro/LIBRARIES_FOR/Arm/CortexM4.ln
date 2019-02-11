/************************************************************************************
 *  File:     Flash.ld
 *  Purpose:  Linker script to configure memory regions for Cortex-M4 
 *            and place sections and symbol values.
 *  Date:     05 July 2013
 *  Info:     Reset_Handler must be defined in code as entry point of reset handler.
 *
 *            By default the script is configured for the first flash and RAM bank 
 *            of the device.
 *            If additional banks of the device shall be used, 
 *            they can be configured manually.
 *
 *            Following symbols are defined in the script 
 *            and can be used without definition in code:
 *            __exidx_start
 *            __exidx_end
 *            __etext
 *            __data_start__
 *            __preinit_array_start
 *            __preinit_array_end
 *            __init_array_start
 *            __init_array_end
 *            __fini_array_start
 *            __fini_array_end
 *            __data_end__
 *            __bss_start__
 *            __bss_end__
 *            __stack_end__
 ************************************************************************************/

/************************************************************************************
 *
 *  Memory regions and required amount of heap and stack
 ************************************************************************************/
MEMORY
{
  FLASH  (rx)      : ORIGIN = 0x08000000, LENGTH = 0x00100000
  
  RAM  (xrw)       : ORIGIN = 0x20000000, LENGTH = 0x00020000
  
}
_Min_Heap_Size  = 0x1000;   /* required amount of heap  */
_Min_Stack_Size = 0x800;  /* required amount of stack */

/************************************************************************************
 *
 *  Code and data sections
 ************************************************************************************/
ENTRY(Reset_Handler)

SECTIONS
{
/************************************************************************************
 * 
 *  text section (code)
 *  Starts with startup code and vectors
 *  Goes into FLASH
 ************************************************************************************/
  .text :
  {
    /* Startup code */
    KEEP(*(.isr_vector))
    
    /* Code section */
    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
     *crtbegin.o(.dtors)
     *crtbegin?.o(.dtors)
     *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
     *(SORT(.dtors.*))
     *(.dtors)
    
    /* Rodata section (constants, strings, ...) */
    *(.rodata*)

    KEEP(*(.eh_frame*))
  } > FLASH

  .ARM.extab : 
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > FLASH
  __exidx_end = .;
  
  .ARM.attributes : {
    *(.ARM.attributes)
  } > FLASH

  /* Global symbol at the end of code (text section) */
  __etext = .;
    
/************************************************************************************
 *
 *  data section
 *  Goes into RAM
 ************************************************************************************/
  .data : AT (__etext)
  {
    /* Global symbol at start of data */
    __data_start__ = .;
    *(vtable)
    *(.data*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);


    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* Global symbol at end of data */
    __data_end__ = .;

  } > RAM

/************************************************************************************
 * 
 *  bss section (Uninitialized data)
 *  Goes into RAM
 ************************************************************************************/
  .bss :
  {
    . = ALIGN(4);
    /* Global symbol at start of bss */
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    /* Global symbol at end of bss */
    __bss_end__ = .;
  } > RAM

/************************************************************************************
 *
 *  user_heap_stack section
 *  Check that there is enough RAM left for stack and heap
 ************************************************************************************/
  ._user_heap_stack :
  {
    . = ALIGN(8);
    __heap_start__ = .;
    . = . + _Min_Heap_Size;
    __heap_end__ = .;
    . = ALIGN(8);
    __stack_start__ = .;
    . = . + _Min_Stack_Size;
    __stack_end__ = .;
    _estack = .;
    . = ALIGN(8);
  } >RAM
  
/************************************************************************************
 *  
 *  Remove information from the standard libraries
 ************************************************************************************/
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}
