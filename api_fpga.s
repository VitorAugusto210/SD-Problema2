@ ============================================================================
@ api_fpga.s
@
@ API em Assembly para controlar o coprocessador de imagem na FPGA.
@ Esta é a "tradução" da lógica de hardware do 'controlador.c'.
@
@ Para compilar, você DEVE usar o pré-processador C:
@ gcc -E -x assembler-with-cpp -o api_fpga.pp.s api_fpga.s
@ as -o api_fpga.o api_fpga.pp.s
@ ============================================================================

.syntax unified
.arch armv7-a
.text
.align 2

@ Inclui as constantes (offsets, opcodes) do .h
#include "constantes.h"

@ ============================================================================
@ Funções da LibC Importadas
@ ============================================================================
.extern open
.extern mmap
.extern munmap
.extern close
.extern perror
.extern printf

@ ============================================================================
@ Funções da API Exportadas (para o C)
@ ============================================================================
.global setup_memory_map
.global cleanup_memory_map
.global coproc_write_pixel
.global coproc_read_pixel
.global coproc_apply_zoom
.global coproc_reset_image
.global coproc_wait_done

@ ============================================================================
@ Seção de Dados (Ponteiros Globais)
@ ============================================================================
.data
.align 4

g_fd_mem:
    .word   -1
g_virtual_base:
    .word   0

@ Ponteiros para os PIOs
g_pio_instruct_ptr:
    .word   0
g_pio_enable_ptr:
    .word   0
g_pio_dataout_ptr:
    .word   0
g_pio_flags_ptr:
    .word   0

@ Strings para caminhos e erros (usadas internamente)
str_dev_mem:
    .asciz "/dev/mem"
str_open_err:
    .asciz "Erro ao abrir /dev/mem (Execute com sudo)"
str_mmap_err:
    .asciz "Erro no mmap()"

@ ============================================================================
@ Seção de Texto (Código)
@ ============================================================================
.text

@ --- Função interna: pio_pulse_enable ---
@ Não exportada. Usada por outras funções.
.type pio_pulse_enable, %function
pio_pulse_enable:
    push    {r0, r1, lr}
    ldr     r0, =g_pio_enable_ptr
    ldr     r0, [r0]
    
    mov     r1, #1
    str     r1, [r0]        @ *g_pio_enable_ptr = 1;
    
    mov     r1, #0
    str     r1, [r0]        @ *g_pio_enable_ptr = 0;
    
    pop     {r0, r1, pc}
.size pio_pulse_enable, .-pio_pulse_enable


@ ============================================================================
@ Função: setup_memory_map
@ Retorno: r0 = 0 (sucesso) ou -1 (erro)
@ ============================================================================
.type setup_memory_map, %function
setup_memory_map:
    push    {r4-r8, lr}
    
    @ 1. open("/dev/mem", O_RDWR | O_SYNC)
    ldr     r0, =str_dev_mem
    ldr     r1, =(0x2 | 0x1000) @ O_RDWR | O_SYNC (Valores comuns de bits)
    bl      open
    
    cmp     r0, #0
    blt     setup_open_fail
    
    ldr     r1, =g_fd_mem
    str     r0, [r1]        @ Salva o file descriptor
    mov     r4, r0          @ r4 = g_fd_mem
    
    @ 2. mmap(NULL, LW_BRIDGE_SPAN, PROT_READ|PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE)
    mov     r0, #0              @ NULL
    ldr     r1, =LW_BRIDGE_SPAN
    mov     r2, #3              @ PROT_READ | PROT_WRITE
    mov     r3, #1              @ MAP_SHARED
    str     r4, [sp]            @ 5º arg (fd)
    ldr     r5, =LW_BRIDGE_BASE
    str     r5, [sp, #4]        @ 6º arg (offset)
    
    bl      mmap
    
    cmp     r0, #-1
    beq     setup_mmap_fail
    
    @ Salva o ponteiro base virtual
    ldr     r1, =g_virtual_base
    str     r0, [r1]
    mov     r5, r0              @ r5 = g_virtual_base
    
    @ 3. Calcula os ponteiros dos PIOs
    @ g_pio_instruct_ptr = g_virtual_base + PIO_INSTRUCT_OFFSET
    ldr     r1, =PIO_INSTRUCT_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_instruct_ptr
    str     r6, [r7]
    
    @ g_pio_enable_ptr = g_virtual_base + PIO_ENABLE_OFFSET
    ldr     r1, =PIO_ENABLE_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_enable_ptr
    str     r6, [r7]

    @ g_pio_dataout_ptr = g_virtual_base + PIO_DATAOUT_OFFSET
    ldr     r1, =PIO_DATAOUT_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_dataout_ptr
    str     r6, [r7]
    
    @ g_pio_flags_ptr = g_virtual_base + PIO_FLAGS_OFFSET
    ldr     r1, =PIO_FLAGS_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_flags_ptr
    str     r6, [r7]

    @ Sucesso
    mov     r0, #0
    pop     {r4-r8, pc}

setup_open_fail:
    ldr     r0, =str_open_err
    bl      perror
    mvn     r0, #0          @ Retorna -1
    pop     {r4-r8, pc}

setup_mmap_fail:
    ldr     r0, =str_mmap_err
    bl      perror
    ldr     r0, =g_fd_mem   @ Fecha o /dev/mem
    ldr     r0, [r0]
    bl      close
    mvn     r0, #0          @ Retorna -1
    pop     {r4-r8, pc}
.size setup_memory_map, .-setup_memory_map


@ ============================================================================
@ Função: cleanup_memory_map
@ ============================================================================
.type cleanup_memory_map, %function
cleanup_memory_map:
    push    {r4, lr}
    
    @ munmap(g_virtual_base, LW_BRIDGE_SPAN)
    ldr     r0, =g_virtual_base
    ldr     r0, [r0]
    ldr     r1, =LW_BRIDGE_SPAN
    bl      munmap
    
    @ close(g_fd_mem)
    ldr     r0, =g_fd_mem
    ldr     r0, [r0]
    bl      close
    
    pop     {r4, pc}
.size cleanup_memory_map, .-cleanup_memory_map


@ ============================================================================
@ Função: coproc_wait_done
@ ============================================================================
.type coproc_wait_done, %function
coproc_wait_done:
    push    {r0, r1, lr}
    
    ldr     r0, =g_pio_flags_ptr
    ldr     r0, [r0]        @ r0 = g_pio_flags_ptr
    
wait_loop$:
    ldr     r1, [r0]        @ r1 = *g_pio_flags_ptr
    ldr     r2, =FLAG_DONE_MASK
    tst     r1, r2          @ (r1 & FLAG_DONE_MASK)
    beq     wait_loop$      @ Loop se for 0
    
    pop     {r0, r1, pc}
.size coproc_wait_done, .-coproc_wait_done


@ ============================================================================
@ Função: coproc_apply_zoom
@ r0: uint32_t algorithm_code
@ ============================================================================
.type coproc_apply_zoom, %function
coproc_apply_zoom:
    push    {r0, r1, lr}
    
    @ pio_write(g_pio_instruct_ptr, algorithm_code)
    ldr     r1, =g_pio_instruct_ptr
    ldr     r1, [r1]
    str     r0, [r1]
    
    @ pio_pulse_enable()
    bl      pio_pulse_enable
    
    pop     {r0, r1, pc}
.size coproc_apply_zoom, .-coproc_apply_zoom


@ ============================================================================
@ Função: coproc_reset_image
@ ============================================================================
.type coproc_reset_image, %function
coproc_reset_image:
    push    {r0, lr}
    
    @ pio_write(g_pio_instruct_ptr, OP_RESET)
    ldr     r0, =g_pio_instruct_ptr
    ldr     r0, [r0]
    ldr     r1, =OP_RESET
    str     r1, [r0]
    
    @ pio_pulse_enable()
    bl      pio_pulse_enable
    
    pop     {r0, pc}
.size coproc_reset_image, .-coproc_reset_image


@ ============================================================================
@ Função: coproc_write_pixel
@ r0: uint32_t address
@ r1: uint8_t value
@ ============================================================================
.type coproc_write_pixel, %function
coproc_write_pixel:
    push    {r0-r3, lr}
    @ r0 = address, r1 = value
    
    @ Constrói a instrução
    ldr     r3, =OP_STORE           @ r3 = OP_STORE
    
    lsl     r0, r0, #3              @ r0 = address << 3
    orr     r3, r3, r0              @ instruction |= (address << 3)
    
    lsl     r1, r1, #21             @ r1 = value << 21
    orr     r3, r3, r1              @ instruction |= (value << 21)
    
    @ pio_write(g_pio_instruct_ptr, instruction)
    ldr     r2, =g_pio_instruct_ptr
    ldr     r2, [r2]
    str     r3, [r2]
    
    @ pio_pulse_enable()
    bl      pio_pulse_enable
    
    pop     {r0-r3, pc}
.size coproc_write_pixel, .-coproc_write_pixel


@ ============================================================================
@ Função: coproc_read_pixel
@ r0: uint32_t address
@ r1: uint32_t sel_mem
@ Retorno: r0 = uint8_t value
@ ============================================================================
.type coproc_read_pixel, %function
coproc_read_pixel:
    push    {r1-r4, lr}
    @ r0 = address, r1 = sel_mem
    
    @ Constrói a instrução
    ldr     r3, =OP_LOAD            @ r3 = OP_LOAD
    
    lsl     r0, r0, #3              @ r0 = address << 3
    orr     r3, r3, r0              @ instruction |= (address << 3)
    
    lsl     r1, r1, #20             @ r1 = sel_mem << 20
    orr     r3, r3, r1              @ instruction |= (sel_mem << 20)
    
    @ pio_write(g_pio_instruct_ptr, instruction)
    ldr     r2, =g_pio_instruct_ptr
    ldr     r2, [r2]
    str     r3, [r2]
    
    @ pio_pulse_enable()
    bl      pio_pulse_enable
    
    @ coproc_wait_done()
    bl      coproc_wait_done
    
    @ return (uint8_t)(pio_read(g_pio_dataout_ptr) & 0xFF)
    ldr     r4, =g_pio_dataout_ptr
    ldr     r4, [r4]
    ldr     r0, [r4]                @ r0 = *g_pio_dataout_ptr
    
    uxtb    r0, r0                  @ Extrai o byte (equivale a & 0xFF)
    
    pop     {r1-r4, pc}
.size coproc_read_pixel, .-coproc_read_pixel