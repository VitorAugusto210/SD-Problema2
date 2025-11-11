#ifndef FPGA_CONSTANTS_H
#define FPGA_CONSTANTS_H

/*
 * =================================================================
 * Constantes do Hardware da FPGA para o HPS
 * =================================================================
 * Este arquivo define os endereços de hardware absolutos e os
 * opcodes de instrução para o coprocessador de imagem na FPGA.
 *Para adicionar controle por teclado, precisamos modificar o programa em C que roda no HPS.
 * Ele é projetado para ser incluído tanto por arquivos C (.c)
 * quanto por arquivos Assembly (.S) que são passados pelo
 * pré-processador C.
 */

// =================================================================
// Definições da Bridge HPS-para-FPGA
// =================================================================
// Baseado nas definições em controlador.c
#define LW_BRIDGE_BASE      0xFF200000
#define LW_BRIDGE_SPAN 0x1000

// Offsets dos PIOs
#define PIO_INSTRUCT_OFFSET 0x00000000
#define PIO_ENABLE_OFFSET   0x00000010
#define PIO_FLAGS_OFFSET    0x00000030
#define PIO_DATAOUT_OFFSET  0x00000020

// =================================================================
// Endereços Base Absolutos (Usados pelo Assembly .S)
// =================================================================
// Endereços físicos completos para o linker/assembler
#define PIO_INSTRUCT_BASE   (LW_BRIDGE_BASE + PIO_INSTRUCT_OFFSET) // 0xFF201000
#define PIO_ENABLE_BASE     (LW_BRIDGE_BASE + PIO_ENABLE_OFFSET)   // 0xFF201010
#define PIO_FLAGS_BASE      (LW_BRIDGE_BASE + PIO_FLAGS_OFFSET)    // 0xFF201020
#define PIO_DATAOUT_BASE    (LW_BRIDGE_BASE + PIO_DATAOUT_OFFSET)  // 0xFF201030

// =================================================================
// Opcodes de Instrução da FPGA (Bits [2:0] do pio_instruct)
// =================================================================
#define OP_REFRESH_SCREEN 0x0 // 3'b000
#define OP_LOAD           0x1 // 3'b001 (LOAD_OPCODE)
#define OP_STORE          0x2 // 3'b010 (STORE_OPCODE)
#define OP_NHI_ALG        0x3 // 3'b011 (Zoom In - Vizinho Mais Próximo)
#define OP_PR_ALG         0x4 // 3'b100 (Zoom In - Repetição de Pixel)
#define OP_BA_ALG         0x5 // 3'b101 (Zoom Out - Média de Blocos)
#define OP_NH_ALG         0x6 // 3'b110 (Zoom Out - Vizinho Mais Próximo)
#define OP_RESET          0x7 // 3'b111 (RESET_OPCODE)

// =================================================================
// Máscaras de Bits dos Flags da FPGA (Lidos do pio_flags)
// =================================================================
#define FLAG_DONE_MASK    0x1 // Bit 0: Operação concluída
#define FLAG_ERROR_MASK   0x2 // Bit 1: Erro (ex: endereço inválido)
#define FLAG_ZMAX_MASK    0x4 // Bit 2: Zoom máximo atingido
#define FLAG_ZMIN_MASK    0x8 // Bit 3: Zoom mínimo atingido

#endif // FPGA_CONSTANTS_H