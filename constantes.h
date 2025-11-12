#ifndef FPGA_CONSTANTS_H // Diretiva de pré-processador (include guard) para prevenir inclusão múltipla
#define FPGA_CONSTANTS_H

/*
 * =================================================================
 * Constantes do Hardware da FPGA para o HPS
 * =================================================================
 * Este arquivo define os endereços de hardware absolutos e os
 * opcodes de instrução para o coprocessador de imagem na FPGA.
 *
 * É projetado para ser incluído tanto por arquivos C (.c)
 * quanto por arquivos Assembly (.S) que são passados pelo
 * pré-processador C (cpp).
 */

// =================================================================
// Definições da Bridge HPS-para-FPGA (Lightweight)
// =================================================================
// Define a janela de memória (MMIO) para acessar os periféricos da FPGA
#define LW_BRIDGE_BASE      0xFF200000 // Endereço físico base da ponte leve HPS-FPGA
#define LW_BRIDGE_SPAN      0x1000     // Tamanho da janela de memória mapeada (4KB)

// Offsets dos PIOs (Parallel I/O) relativos ao LW_BRIDGE_BASE
#define PIO_INSTRUCT_OFFSET 0x00000000 // Deslocamento do registrador de instrução/dados (escrita)
#define PIO_ENABLE_OFFSET   0x00000010 // Deslocamento do registrador de 'enable' (controle)
#define PIO_FLAGS_OFFSET    0x00000030 // Deslocamento do registrador de 'flags' (status/leitura)
#define PIO_DATAOUT_OFFSET  0x00000020 // Deslocamento do registrador de dados de saída (leitura)

// =================================================================
// Endereços Base Absolutos (Usados pelo Assembly .S)
// =================================================================
// Endereços físicos completos (usados para referência no linker/assembler)
#define PIO_INSTRUCT_BASE   (LW_BRIDGE_BASE + PIO_INSTRUCT_OFFSET)
#define PIO_ENABLE_BASE     (LW_BRIDGE_BASE + PIO_ENABLE_OFFSET)
#define PIO_FLAGS_BASE      (LW_BRIDGE_BASE + PIO_FLAGS_OFFSET)
#define PIO_DATAOUT_BASE    (LW_BRIDGE_BASE + PIO_DATAOUT_OFFSET)

// =================================================================
// Opcodes de Instrução da FPGA (Bits [2:0] do pio_instruct)
// =================================================================
// Códigos de operação que o hardware do coprocessador entende
#define OP_REFRESH_SCREEN 0x0 // 3'b000: (Não utilizado neste menu)
#define OP_LOAD           0x1 // 3'b001: Comando para ler um pixel da memória interna da FPGA
#define OP_STORE          0x2 // 3'b010: Comando para escrever um pixel na memória interna da FPGA
#define OP_NHI_ALG        0x3 // 3'b011: (Zoom In) Algoritmo Vizinho Mais Próximo
#define OP_PR_ALG         0x4 // 3'b100: (Zoom In) Algoritmo Repetição de Pixel
#define OP_BA_ALG         0x5 // 3'b101: (Zoom Out) Algoritmo Média de Blocos
#define OP_NH_ALG         0x6 // 3'b110: (Zoom Out) Algoritmo Vizinho Mais Próximo
#define OP_RESET          0x7 // 3'b111: Comando para resetar (ex: copiar mem1 -> mem2)

// =================================================================
// Máscaras de Bits dos Flags da FPGA (Lidos do pio_flags)
// =================================================================
// Máscaras para decodificar o registrador de status do coprocessador
#define FLAG_DONE_MASK    0x1 // Bit 0: Indica que a operação foi concluída
#define FLAG_ERROR_MASK   0x2 // Bit 1: Indica que ocorreu um erro (ex: endereço inválido)
#define FLAG_ZMAX_MASK    0x4 // Bit 2: Indica que o zoom máximo foi atingido
#define FLAG_ZMIN_MASK    0x8 // Bit 3: Indica que o zoom mínimo foi atingido

#endif // FPGA_CONSTANTS_H
