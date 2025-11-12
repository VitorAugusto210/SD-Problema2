@ ============================================================================
@ api_fpga.s
@
@ API em Assembly (ARMv7-a) para controlar o coprocessador de imagem na FPGA.
@ Esta biblioteca gerencia o acesso de baixo nível (MMIO) aos registradores
@ (PIOs) do coprocessador.
@
@ Compilação (requer pré-processador C para #include):
@ gcc -E -x assembler-with-cpp -o api_fpga.pp.s api_fpga.s
@ as -o api_fpga.o api_fpga.pp.s
@ ============================================================================

.syntax unified     @ Habilita a sintaxe unificada (ARM/Thumb)
.arch armv7-a       @ Define a arquitetura alvo (HPS do Cyclone V)
.text               @ Define a seção de código (instruções)
.align 2            @ Alinha instruções em 4 bytes (2^2)

@ Inclui constantes (offsets, opcodes) do .h via pré-processador C
#include "constantes.h"

@ ============================================================================
@ Funções da LibC Importadas
@ ============================================================================
@ Declaração de símbolos externos que serão resolvidos pelo linker
.extern open        @ syscall para abrir /dev/mem
.extern mmap        @ syscall para mapear memória física em virtual
.extern munmap      @ syscall para desmapear memória
.extern close       @ syscall para fechar o file descriptor
.extern perror      @ Função da LibC para imprimir erros do sistema
.extern printf      @ (Incluído para completude, embora não usado aqui)

@ ============================================================================
@ Funções da API Exportadas (para o C)
@ ============================================================================
@ Torna os símbolos desta API visíveis para o linker C
.global setup_memory_map
.global cleanup_memory_map
.global coproc_write_pixel
.global coproc_read_pixel
.global coproc_apply_zoom
.global coproc_reset_image
.global coproc_wait_done

@ ============================================================================
@ Seção de Dados (Variáveis Globais)
@ ============================================================================
.data               @ Define a seção de dados inicializados
.align 4            @ Alinha dados em 4 bytes

g_fd_mem:
    .word   -1      @ Armazena o file descriptor para /dev/mem
g_virtual_base:
    .word   0       @ Armazena o endereço base virtual retornado por mmap

@ Ponteiros virtuais para os registradores PIO (calculados em tempo de execução)
g_pio_instruct_ptr:
    .word   0
g_pio_enable_ptr:
    .word   0
g_pio_dataout_ptr:
    .word   0
g_pio_flags_ptr:
    .word   0

@ Strings C (null-terminated) para caminhos e erros
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
@ Gera um pulso rápido (1 -> 0) no pio_enable para disparar o coprocessador.
.type pio_pulse_enable, %function
pio_pulse_enable:
    push    {r0, r1, lr}    @ Preserva registradores e o Link Register (retorno)
    ldr     r0, =g_pio_enable_ptr @ Carrega o endereço do ponteiro
    ldr     r0, [r0]        @ Carrega o valor do ponteiro (endereço virtual do PIO)
    
    mov     r1, #1          @ Valor 1
    str     r1, [r0]        @ Escreve 1 no registrador (borda de subida)
    
    mov     r1, #0          @ Valor 0
    str     r1, [r0]        @ Escreve 0 no registrador (borda de descida)
    
    pop     {r0, r1, pc}    @ Restaura registradores e retorna (movendo LR para PC)
.size pio_pulse_enable, .-pio_pulse_enable


@ ============================================================================
@ Função: setup_memory_map
@ Mapeia a memória física da HPS-to-FPGA bridge para o espaço de
@ endereçamento virtual do processo.
@ Retorno: r0 = 0 (sucesso) ou -1 (erro)
@ ============================================================================
.type setup_memory_map, %function
setup_memory_map:
    push    {r4-r8, lr}     @ Preserva registradores de trabalho e LR
    
    @ 1. Abre /dev/mem: open("/dev/mem", O_RDWR | O_SYNC)
    ldr     r0, =str_dev_mem    @ r0 (arg1): ponteiro para "/dev/mem"
    ldr     r1, =(0x2 | 0x1000) @ r1 (arg2): flags O_RDWR (0x2) | O_SYNC (0x1000)
    bl      open            @ Chama a função da LibC
    
    cmp     r0, #0          @ Compara o retorno (file descriptor) com 0
    blt     setup_open_fail @ Se r0 < 0 (erro), salta para o handler de erro
    
    ldr     r1, =g_fd_mem   @ Carrega o endereço da variável global
    str     r0, [r1]        @ Salva o file descriptor (fd) em g_fd_mem
    mov     r4, r0          @ Salva o fd no registrador r4 (preservado)
    
    @ 2. Mapeia a memória: mmap(NULL, SPAN, PROT_RW, MAP_SHARED, fd, BASE)
    mov     r0, #0              @ r0 (arg1): addr (NULL)
    ldr     r1, =LW_BRIDGE_SPAN @ r1 (arg2): length
    mov     r2, #3              @ r2 (arg3): prot (PROT_READ | PROT_WRITE)
    mov     r3, #1              @ r3 (arg4): flags (MAP_SHARED)
    str     r4, [sp]            @ Empilha o 5º argumento (fd, em r4)
    ldr     r5, =LW_BRIDGE_BASE @ Carrega o offset físico
    str     r5, [sp, #4]        @ Empilha o 6º argumento (offset)
    
    bl      mmap            @ Chama a função da LibC
    
    cmp     r0, #-1         @ Compara o retorno (endereço virtual) com MAP_FAILED (-1)
    beq     setup_mmap_fail @ Se erro, salta para o handler
    
    @ Salva o ponteiro base virtual
    ldr     r1, =g_virtual_base @ Carrega o endereço da variável
    str     r0, [r1]        @ Salva o endereço virtual em g_virtual_base
    mov     r5, r0          @ Salva o endereço base virtual em r5 (preservado)
    
    @ 3. Calcula os ponteiros virtuais absolutos para os PIOs
    @ (base_virtual + offset_pio)
    ldr     r1, =PIO_INSTRUCT_OFFSET
    add     r6, r5, r1      @ r6 = base_virtual + offset
    ldr     r7, =g_pio_instruct_ptr @ Carrega o endereço da variável ponteiro
    str     r6, [r7]        @ Salva o endereço virtual calculado

    ldr     r1, =PIO_ENABLE_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_enable_ptr
    str     r6, [r7]

    ldr     r1, =PIO_DATAOUT_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_dataout_ptr
    str     r6, [r7]
    
    ldr     r1, =PIO_FLAGS_OFFSET
    add     r6, r5, r1
    ldr     r7, =g_pio_flags_ptr
    str     r6, [r7]

    @ Sucesso
    mov     r0, #0          @ Define o valor de retorno como 0 (sucesso)
    pop     {r4-r8, pc}     @ Restaura registradores e retorna

setup_open_fail:
    ldr     r0, =str_open_err @ r0 (arg1): string de erro
    bl      perror          @ Chama perror() para imprimir o erro do sistema
    mvn     r0, #0          @ mov r0, -1 (define o retorno como -1)
    pop     {r4-r8, pc}     @ Restaura e retorna

setup_mmap_fail:
    ldr     r0, =str_mmap_err @ r0 (arg1): string de erro
    bl      perror          @ Chama perror()
    ldr     r0, =g_fd_mem   @ Carrega o fd salvo
    ldr     r0, [r0]
    bl      close           @ Fecha o /dev/mem
    mvn     r0, #0          @ mov r0, -1 (define o retorno como -1)
    pop     {r4-r8, pc}     @ Restaura e retorna
.size setup_memory_map, .-setup_memory_map


@ ============================================================================
@ Função: cleanup_memory_map
@ Libera os recursos (munmap e close) alocados por setup_memory_map.
@ ============================================================================
.type cleanup_memory_map, %function
cleanup_memory_map:
    push    {r4, lr}        @ Preserva r4 e LR
    
    @ 1. Desfaz o mapeamento: munmap(g_virtual_base, LW_BRIDGE_SPAN)
    ldr     r0, =g_virtual_base
    ldr     r0, [r0]        @ r0 (arg1): ponteiro virtual salvo
    ldr     r1, =LW_BRIDGE_SPAN @ r1 (arg2): tamanho
    bl      munmap          @ Chama munmap
    
    @ 2. Fecha o file descriptor: close(g_fd_mem)
    ldr     r0, =g_fd_mem
    ldr     r0, [r0]        @ r0 (arg1): file descriptor salvo
    bl      close           @ Chama close
    
    pop     {r4, pc}        @ Restaura e retorna
.size cleanup_memory_map, .-cleanup_memory_map


@ ============================================================================
@ Função: coproc_wait_done
@ Realiza 'busy-waiting' (polling) no flag 'DONE' do coprocessador.
@ ============================================================================
.type coproc_wait_done, %function
coproc_wait_done:
    push    {r0, r1, lr}    @ Preserva r0, r1, LR
    
    ldr     r0, =g_pio_flags_ptr @ Carrega o endereço do ponteiro
    ldr     r0, [r0]        @ r0 = endereço virtual do PIO de flags
    
wait_loop$:
    ldr     r1, [r0]        @ r1 = *g_pio_flags_ptr (Lê o valor do PIO)
    ldr     r2, =FLAG_DONE_MASK @ r2 = 0x1 (Máscara do bit DONE)
    tst     r1, r2          @ Testa se (r1 & r2) != 0. Atualiza flags Z.
    beq     wait_loop$      @ Branch se Z=1 (resultado foi 0). Loopa se o bit DONE for 0.
    
    pop     {r0, r1, pc}    @ Restaura e retorna (quando o bit DONE é 1)
.size coproc_wait_done, .-coproc_wait_done


@ ============================================================================
@ Função: coproc_apply_zoom
@ Envia um opcode de algoritmo de zoom para o coprocessador.
@ Argumento (r0): uint32_t algorithm_code (o opcode, ex: OP_BA_ALG)
@ ============================================================================
.type coproc_apply_zoom, %function
coproc_apply_zoom:
    push    {r0, r1, lr}    @ Preserva r0 (que contém o argumento), r1 e LR
    
    @ pio_write(g_pio_instruct_ptr, algorithm_code)
    ldr     r1, =g_pio_instruct_ptr
    ldr     r1, [r1]        @ r1 = endereço virtual do PIO de instrução
    str     r0, [r1]        @ Escreve o opcode (em r0) no PIO
    
    @ Pulsa 'enable' para iniciar a operação
    bl      pio_pulse_enable
    
    pop     {r0, r1, pc}    @ Restaura e retorna
.size coproc_apply_zoom, .-coproc_apply_zoom


@ ============================================================================
@ Função: coproc_reset_image
@ Envia o comando de RESET (OP_RESET) para o coprocessador.
@ ============================================================================
.type coproc_reset_image, %function
coproc_reset_image:
    push    {r0, lr}        @ Preserva r0 e LR
    
    @ pio_write(g_pio_instruct_ptr, OP_RESET)
    ldr     r0, =g_pio_instruct_ptr
    ldr     r0, [r0]        @ r0 = endereço virtual do PIO de instrução
    ldr     r1, =OP_RESET   @ r1 = valor da constante OP_RESET
    str     r1, [r0]        @ Escreve o opcode no PIO
    
    @ Pulsa 'enable' para iniciar
    bl      pio_pulse_enable
    
    pop     {r0, pc}        @ Restaura e retorna
.size coproc_reset_image, .-coproc_reset_image


@ ============================================================================
@ Função: coproc_write_pixel
@ Constrói e envia uma instrução OP_STORE para a FPGA.
@ Argumento (r0): uint32_t address
@ Argumento (r1): uint8_t value
@ ============================================================================
.type coproc_write_pixel, %function
coproc_write_pixel:
    push    {r0-r3, lr}     @ Preserva r0-r3 e LR
    @ r0 contém 'address', r1 contém 'value'
    
    @ Constrói a palavra de instrução (formato HW: [value(8)] [sel(1)] [addr(17)] [op(3)])
    ldr     r3, =OP_STORE   @ r3 = 0x2 (Opcode nos bits [2:0])
    
    lsl     r0, r0, #3      @ r0 = address << 3 (Alinha o endereço aos bits [19:3])
    orr     r3, r3, r0      @ r3 |= (address << 3)
    
    lsl     r1, r1, #21     @ r1 = value << 21 (Alinha o valor aos bits [28:21])
    orr     r3, r3, r1      @ r3 |= (value << 21)
    
    @ pio_write(g_pio_instruct_ptr, instruction)
    ldr     r2, =g_pio_instruct_ptr
    ldr     r2, [r2]        @ r2 = endereço virtual do PIO de instrução
    str     r3, [r2]        @ Escreve a instrução completa (em r3)
    
    @ Pulsa 'enable'
    bl      pio_pulse_enable
    
    pop     {r0-r3, pc}     @ Restaura e retorna
.size coproc_write_pixel, .-coproc_write_pixel


@ ============================================================================
@ Função: coproc_read_pixel
@ Constrói e envia uma instrução OP_LOAD, aguarda e retorna o resultado.
@ Argumento (r0): uint32_t address
@ Argumento (r1): uint32_t sel_mem
@ Retorno   (r0): uint8_t value
@ ============================================================================
.type coproc_read_pixel, %function
coproc_read_pixel:
    push    {r1-r4, lr}     @ Preserva r1-r4 e LR
    @ r0 contém 'address', r1 contém 'sel_mem'
    
    @ Constrói a palavra de instrução (formato HW: [value(8)] [sel(1)] [addr(17)] [op(3)])
    ldr     r3, =OP_LOAD    @ r3 = 0x1 (Opcode nos bits [2:0])
    
    lsl     r0, r0, #3      @ r0 = address << 3 (Alinha o endereço aos bits [19:3])
    orr     r3, r3, r0      @ r3 |= (address << 3)
    
    lsl     r1, r1, #20     @ r1 = sel_mem << 20 (Alinha o bit sel_mem ao bit [20])
    orr     r3, r3, r1      @ r3 |= (sel_mem << 20)
    
    @ pio_write(g_pio_instruct_ptr, instruction)
    ldr     r2, =g_pio_instruct_ptr
    ldr     r2, [r2]        @ r2 = endereço virtual do PIO de instrução
    str     r3, [r2]        @ Escreve a instrução completa
    
    @ Pulsa 'enable' para iniciar a leitura
    bl      pio_pulse_enable
    
    @ Aguarda a FPGA sinalizar que a leitura está pronta
    bl      coproc_wait_done
    
    @ Lê o resultado do PIO de dados
    ldr     r4, =g_pio_dataout_ptr
    ldr     r4, [r4]        @ r4 = endereço virtual do PIO de dados de saída
    ldr     r0, [r4]        @ r0 = *g_pio_dataout_ptr (Lê o valor de 32 bits)
    
    @ Extrai o byte (o valor do pixel)
    uxtb    r0, r0          @ Unsigned Extend Byte: Zera os bits [31:8] de r0
    
    pop     {r1-r4, pc}     @ Restaura e retorna (resultado está em r0)
.size coproc_read_pixel, .-coproc_read_pixel
