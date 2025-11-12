#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <string.h> // Para memset

#include "constantes.h" // Inclui os Opcodes

// =================================================================
// Declarações da API em Assembly (api_fpga.s)
// =================================================================
extern int setup_memory_map(void);
extern void cleanup_memory_map(void);
extern void coproc_write_pixel(uint32_t address, uint8_t value);
extern uint8_t coproc_read_pixel(uint32_t address, uint32_t sel_mem);
extern void coproc_apply_zoom(uint32_t algorithm_code);
extern void coproc_reset_image(void);
extern void coproc_wait_done(void);
extern void coproc_apply_zoom_with_offset(uint32_t algorithm_code, uint32_t x_offset, uint32_t y_offset);
extern void coproc_pan_zoom_with_offset(uint32_t algorithm_code, uint32_t x_offset, uint32_t y_offset);


// =================================================================
// Estruturas do Bitmap (Sem alterações)
// =================================================================
#pragma pack(1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack()

// =================================================================
// Função de Carregamento de Imagem
// =================================================================
int load_bmp_image(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo BMP");
        return -1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 8) {
        printf("Erro: O arquivo deve ser um BMP de 8 bits (escala de cinza).\n");
        printf("       (Detectado: Tipo %x, %d bits)\n", fileHeader.bfType, infoHeader.biBitCount);
        fclose(file);
        return -1;
    }
    
    if (infoHeader.biCompression != 0) {
        printf("Aviso: Imagem BMP está comprimida (Tipo: %d). \n", infoHeader.biCompression);
        printf("       A imagem pode aparecer distorcida.\n");
    }
    
    printf("Lendo imagem: %s (%dx%d pixels, %d bits)\n", 
           filename, infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount);

    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int padding = (4 - (width * 1) % 4) % 4;
    
    printf("Iniciando transferência para a FPGA...\n");

    for (int y = height - 1; y >= 0; y--) { // Declaração C99
        for (int x = 0; x < width; x++) { // Declaração C99
            
            uint8_t gray_pixel;
            if (fread(&gray_pixel, 1, 1, file) < 1) {
                gray_pixel = 0; 
            }
            
            uint32_t fpga_addr = (uint32_t)(y * width + x);

            if (fpga_addr > 76799) break; 

            coproc_write_pixel(fpga_addr, gray_pixel);
            coproc_wait_done(); 
        }
        
        fseek(file, padding, SEEK_CUR);
        
        if (y > 0 && y % (height/10) == 0) {
            printf("Progresso: %d%%\n", (int)(((float)(height - y) / height) * 100));
        }
    }
    
    printf("Transferência de imagem concluída.\n");
    fclose(file);
    return 0;
}


// =================================================================
// Menu Interativo
// =================================================================

typedef enum {
    ZOOM_OUT_NEAREST_NEIGHBOR,
    ZOOM_OUT_BLOCK_AVERAGE
} ZoomOutMode;

typedef enum {
    ZOOM_IN_PIXEL_REPETITION,
    ZOOM_IN_NEAREST_NEIGHBOR
} ZoomInMode;

ZoomOutMode current_zoom_out_mode = ZOOM_OUT_BLOCK_AVERAGE;
ZoomInMode  current_zoom_in_mode  = ZOOM_IN_PIXEL_REPETITION;

static uint32_t g_zoom_offset_x = 0;
static uint32_t g_zoom_offset_y = 0;
#define MOVE_STEP 10 // Quantos pixels mover por clique
#define IMG_WIDTH 320
#define IMG_HEIGHT 240

static struct termios old_termios, new_termios;

void set_terminal_mode() {
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void restore_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void print_menu() {
    printf("\n--- Controle Interativo da FPGA (Híbrido C+ASM) ---\n");
    printf("Controles de Zoom:\n");
    printf("  [Setas]: Mover 'Pan' (panorâmica) do Zoom In\n");
    printf("  [i] ou [+]: Aplicar Zoom In (na posição atual do cursor)\n");
    printf("  [o] ou [-]: Zoom Out\n");
    printf("\nSeleção de Algoritmo:\n");
    printf("  [m]: Alternar modo de Zoom OUT (Atual: %s)\n", 
           (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) ? 
           "Media de Blocos" : "Vizinho Mais Proximo");
    printf("  [n]: Alternar modo de Zoom IN (Atual: %s)\n", 
           (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) ? 
           "Repeticao de Pixel" : "Vizinho Mais Proximo");
    printf("\nOutros Comandos:\n");
    printf("  [l]: Carregar nova imagem BMP\n"); 
    printf("  [r]: Resetar imagem (recarrega da mem1 original)\n");
    printf("  [h]: Mostrar este menu\n");
    printf("  [q]: Sair\n");
    printf("--------------------------------------------------\n");
}

void handle_load_image() {
    char filename[256];
    
    restore_terminal_mode();
    
    printf("\n--- Carregar Nova Imagem ---\n");
    printf("Digite o caminho para a imagem .bmp: ");
    
    scanf("%255s", filename);
    
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    printf("Carregando '%s'...\n", filename);
    
    if (load_bmp_image(filename) != 0) {
        printf("Falha ao carregar a imagem.\n");
    } else {
        printf("Imagem carregada. Enviando comando de RESET para exibir...\n");
        coproc_reset_image();
        coproc_wait_done();
        printf("Imagem exibida.\n");
    }
    
    set_terminal_mode();
    print_menu(); 
}

void aplicar_zoom_na_posicao_atual() {
    printf("Aplicando Zoom In na posição (%d, %d)...\n", g_zoom_offset_x, g_zoom_offset_y);
    
    if (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) {
        coproc_apply_zoom_with_offset(OP_PR_ALG, g_zoom_offset_x, g_zoom_offset_y);
        coproc_wait_done();           
    } else { // ZOOM_IN_NEAREST_NEIGHBOR
        coproc_apply_zoom_with_offset(OP_NHI_ALG, g_zoom_offset_x, g_zoom_offset_y);
        coproc_wait_done();
    }
    
    printf("Zoom In concluído.\n");
}

void aplicar_pan_na_posicao_atual() {
    printf("Aplicando Pan (movendo) para a posição (%d, %d)...\n", g_zoom_offset_x, g_zoom_offset_y);
    
    if (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) {
        coproc_pan_zoom_with_offset(OP_PR_ALG, g_zoom_offset_x, g_zoom_offset_y);
        coproc_wait_done();           
    } else { // ZOOM_IN_NEAREST_NEIGHBOR
        coproc_pan_zoom_with_offset(OP_NHI_ALG, g_zoom_offset_x, g_zoom_offset_y);
        coproc_wait_done();
    }
    
    printf("Pan concluído.\n");
}


void enter_control_loop() {
    char c;
    set_terminal_mode();
    print_menu();

    while (1) {
        c = getchar();
        
        if (c == 0x1B) { 
            if (getchar() == 0x5B) { 
                switch (getchar()) { 
                    case 0x41: // Seta para Cima
                        g_zoom_offset_y = (g_zoom_offset_y >= MOVE_STEP) ? (g_zoom_offset_y - MOVE_STEP) : 0;
                        printf("Nova posição do cursor: (%d, %d)\n", g_zoom_offset_x, g_zoom_offset_y);
                        aplicar_pan_na_posicao_atual(); // Chama a função de PAN
                        break;
                    case 0x42: // Seta para Baixo
                        g_zoom_offset_y = (g_zoom_offset_y + MOVE_STEP < IMG_HEIGHT) ? (g_zoom_offset_y + MOVE_STEP) : g_zoom_offset_y;
                        printf("Nova posição do cursor: (%d, %d)\n", g_zoom_offset_x, g_zoom_offset_y);
                        aplicar_pan_na_posicao_atual(); // Chama a função de PAN
                        break;
                    case 0x43: // Seta para Direita
                        g_zoom_offset_x = (g_zoom_offset_x + MOVE_STEP < IMG_WIDTH) ? (g_zoom_offset_x + MOVE_STEP) : g_zoom_offset_x;
                        printf("Nova posição do cursor: (%d, %d)\n", g_zoom_offset_x, g_zoom_offset_y);
                        aplicar_pan_na_posicao_atual(); // Chama a função de PAN
                        break;
                    case 0x44: // Seta para Esquerda
                        g_zoom_offset_x = (g_zoom_offset_x >= MOVE_STEP) ? (g_zoom_offset_x - MOVE_STEP) : 0;
                        printf("Nova posição do cursor: (%d, %d)\n", g_zoom_offset_x, g_zoom_offset_y);
                        aplicar_pan_na_posicao_atual(); // Chama a função de PAN
                        break;
                }
                continue; 
            }
        }
        
        if (c == 'q' || c == 'Q') {
            break;
        }

        switch (c) {
            case 'i':
            case '+':
                aplicar_zoom_na_posicao_atual();
                break;
                
            case 'o':
            case '-':
                printf("Aplicando Zoom Out...\n");
                if (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) {
                    coproc_apply_zoom(OP_BA_ALG);
                    coproc_wait_done();
                } else {
                    coproc_apply_zoom(OP_NH_ALG);
                    coproc_wait_done();
                }
                printf("Zoom Out concluído.\n");
                break;
                
            case 'm':
            case 'M':
                if (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) {
                    current_zoom_out_mode = ZOOM_OUT_NEAREST_NEIGHBOR;
                } else {
                    current_zoom_out_mode = ZOOM_OUT_BLOCK_AVERAGE;
                }
                printf("Modo de Zoom OUT alterado.\n");
                print_menu();
                break;

            case 'n':
            case 'N':
                if (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) {
                    current_zoom_in_mode = ZOOM_IN_NEAREST_NEIGHBOR;
                } else {
                    current_zoom_in_mode = ZOOM_IN_PIXEL_REPETITION;
                }
                printf("Modo de Zoom IN alterado.\n");
                print_menu();
                break;

            case 'r':
            case 'R':
                printf("Resetando imagem para o original...\n");
                g_zoom_offset_x = 0;
                g_zoom_offset_y = 0;
                coproc_reset_image(); 
                coproc_wait_done();   
                printf("Reset concluído.\n");
                break;
            
            case 'l':
            case 'L':
                handle_load_image(); 
                break;
                
            case 'h':
            case 'H':
                print_menu();
                break;
        }
    }
    
    restore_terminal_mode();
}

// =================================================================
// Função Principal (main)
// =================================================================

int main(int argc, char *argv[]) {
    
    printf("=== Programa de Teste - Híbrido C + Assembly ===\n\n");
    
    printf("Etapa 1: Mapeando memória da FPGA (via ASM)...\n");
    if (setup_memory_map() != 0) { 
        printf("Falha ao mapear a memória de hardware.\n");
        printf("Verifique se você está executando com 'sudo'.\n");
        return 1;
    }
    
    printf("Etapa 1.5: Enviando RESET inicial para FPGA...\n");
    coproc_reset_image(); 
    coproc_wait_done();   
    printf("Reset inicial concluído.\n");
    
    printf("\nEtapa 2: Entrando no modo interativo...\n");
    printf("Nenhuma imagem carregada. Use [l] no menu para carregar.\n");
    
    enter_control_loop(); 

    printf("\nEtapa 3: Limpando recursos (via ASM)...\n");
    cleanup_memory_map(); 
    printf("Programa encerrado. Configurações do terminal restauradas.\n");
    
    return 0;
}