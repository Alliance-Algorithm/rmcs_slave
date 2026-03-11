/*
******************************************************************************
**

**  File        : LinkerScript.ld
**
**  Author      : STM32CubeMX
**
**  Abstract    : Linker script for STM32F407IGHx series
**                1024Kbytes FLASH and 192Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed "as is", without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** <h2><center>&copy; COPYRIGHT(c) 2025 STMicroelectronics</center></h2>
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**   1. Redistributions of source code must retain the above copyright notice,
**      this list of conditions and the following disclaimer.
**   2. Redistributions in binary form must reproduce the above copyright notice,
**      this list of conditions and the following disclaimer in the documentation
**      and/or other materials provided with the distribution.
**   3. Neither the name of STMicroelectronics nor the names of its contributors
**      may be used to endorse or promote products derived from this software
**      without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Specify the memory areas */
MEMORY
{
RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
CCMRAM (xrw)   : ORIGIN = 0x10000000, LENGTH = 64K - 64
MAILBOX (rw)   : ORIGIN = 0x1000FFC0, LENGTH = 64
FLASH (rx)     : ORIGIN = 0x8010000, LENGTH = 960K
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

/* Define output sections */
SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _siccmram = LOADADDR(.ccmram);

  .ccmram :
  {
    . = ALIGN(4);
    _sccmram = .;
    *(.ccmram)
    *(.ccmram*)

    . = ALIGN(4);
    _eccmram = .;
  } >CCMRAM AT> FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)

    . = ALIGN(4);
  } >RAM AT> FLASH

  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    _edata = .;
    PROVIDE(__data_end = .);
    PROVIDE(__tdata_end = .);
  } >RAM AT> FLASH

  PROVIDE(__tdata_start = ADDR(.tdata));
  PROVIDE(__tdata_size = __tdata_end - __tdata_start);

  PROVIDE(__data_start = ADDR(.data));
  PROVIDE(__data_size = __data_end - __data_start);

  PROVIDE(__tdata_source = LOADADDR(.tdata));
  PROVIDE(__tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata));
  PROVIDE(__tdata_source_size = __tdata_source_end - __tdata_source);

  PROVIDE(__data_source = LOADADDR(.data));
  PROVIDE(__data_source_end = __tdata_source_end);
  PROVIDE(__data_source_size = __data_source_end - __data_source);

  .tbss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE(__tbss_end = .);
  } >RAM

  PROVIDE(__tbss_start = ADDR(.tbss));
  PROVIDE(__tbss_size = __tbss_end - __tbss_start);
  PROVIDE(__tbss_offset = ADDR(.tbss) - ADDR(.tdata));

  PROVIDE(__tls_base = __tdata_start);
  PROVIDE(__tls_end = __tbss_end);
  PROVIDE(__tls_size = __tls_end - __tls_base);
  PROVIDE(__tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)));
  PROVIDE(__tls_size_align = (__tls_size + __tls_align - 1) & ~(__tls_align - 1));
  PROVIDE(__arm32_tls_tcb_offset = MAX(8, __tls_align));
  PROVIDE(__arm64_tls_tcb_offset = MAX(16, __tls_align));

  .bss (NOLOAD) : ALIGN(4)
  {
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
    PROVIDE(__bss_end = .);
  } >RAM

  PROVIDE(__non_tls_bss_start = ADDR(.bss));
  PROVIDE(__bss_start = __tbss_start);
  PROVIDE(__bss_size = __bss_end - __bss_start);

  .boot_mailbox (NOLOAD) :
  {
    . = ALIGN(4);
    __boot_mailbox_start__ = .;
    KEEP(*(.boot_mailbox))
    KEEP(*(.boot_mailbox.*))
    . = ALIGN(4);
    __boot_mailbox_end__ = .;
  } >MAILBOX

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a:*(*)
    libm.a:*(*)
    libgcc.a:*(*)
  }
}
