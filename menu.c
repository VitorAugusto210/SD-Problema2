#include <stdio.h>      // Funções de E/S padrão (printf, fopen, perror, scanf, getchar)
#include <stdlib.h>     // Funções da biblioteca padrão
#include <stdint.h>     // Define tipos de inteiros de tamanho fixo (uint32_t, uint8_t)
#include <termios.h>    // API POSIX para controle de terminal (tcgetattr, tcsetattr)
#include <unistd.h>     // Funções POSIX (STDIN_FILENO)
#include <string.h>     // Para memset (embora não usado, é comum)

#include "constantes.h" // Inclui os Opcodes e offsets de hardware

// =================================================================
// Declarações da API em Assembly (api_fpga.s)
// =================================================================
// Informa ao compilador C que estas funções são externas e serão resolvidas na link-edição.
extern int setup_memory_map(void);
extern void cleanup_memory_map(void);
extern void coproc_write_pixel(uint32_t address, uint8_t value);
extern uint8_t coproc_read_pixel(uint32_t address, uint32_t sel_mem);
extern void coproc_apply_zoom(uint32_t algorithm_code);
extern void coproc_reset_image(void);
extern void coproc_wait_done(void);

// =================================================================
// Estruturas do Bitmap
// =================================================================
#pragma pack(1) // Instrui o compilador a não adicionar padding, permitindo ler bytes exatos do arquivo.
typedef struct {
    uint16_t bfType;      // Assinatura do arquivo (deve ser 0x4D42 = "BM")
    uint32_t bfSize;      // Tamanho total do arquivo em bytes
    uint16_t bfReserved1; // Reservado
    uint16_t bfReserved2; // Reservado
    uint32_t bfOffBits;   // Deslocamento do início do arquivo até os dados de pixel
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          // Tamanho desta estrutura (40 bytes)
    int32_t  biWidth;         // Largura da imagem em pixels
    int32_t  biHeight;        // Altura da imagem em pixels
    uint16_t biPlanes;        // Número de planos (deve ser 1)
    uint16_t biBitCount;      // Bits por pixel (requerido 8 para escala de cinza)
    uint32_t biCompression;   // Tipo de compressão (requerido 0 para BI_RGB)
    uint32_t biSizeImage;     // Tamanho dos dados da imagem em bytes
    int32_t  biXPelsPerMeter; // Resolução horizontal (pixels por metro)
    int32_t  biYPelsPerMeter; // Resolução vertical
    uint32_t biClrUsed;       // Número de cores na paleta
    uint32_t biClrImportant;  // Número de cores importantes
} BITMAPINFOHEADER;
#pragma pack() // Restaura o alinhamento (padding) padrão do compilador

// =================================================================
// Função de Carregamento de Imagem
// =================================================================
/**
 * @brief Lê um arquivo .bmp (8-bit, escala de cinza, sem compressão) e
 * transfere seus pixels, um a um, para a memória interna da FPGA.
 * @param filename O caminho (path) para o arquivo .bmp.
 * @return 0 em sucesso, -1 em falha.
 */
int load_bmp_image(char *filename) {
    FILE *file = fopen(filename, "rb"); // Abre o arquivo em modo "read binary"
    if (!file) {
        perror("Erro ao abrir o arquivo BMP");
        return -1;
    }

    // Lê os cabeçalhos
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    // Validação do formato
    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 8) {
        printf("Erro: O arquivo deve ser um BMP de 8 bits (escala de cinza).\n");
        printf("       (Detectado: Tipo %x, %d bits)\n", fileHeader.bfType, infoHeader.biBitCount);
        fclose(file);
        return -1;
    }
    
    // Alerta sobre compressão (pode não funcionar)
    if (infoHeader.biCompression != 0) {
        printf("Aviso: Imagem BMP está comprimida (Tipo: %d). \n", infoHeader.biCompression);
        printf("       A imagem pode aparecer distorcida.\n");
    }
    
    printf("Lendo imagem: %s (%dx%d pixels, %d bits)\n", 
           filename, infoHeader.biWidth, infoHeader.biHeight, infoHeader.biBitCount);

    // Posiciona o ponteiro do arquivo no início dos dados de pixel
    fseek(file, fileHeader.bfOffBits, SEEK_SET);

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    // Linhas de BMP são alinhadas em múltiplos de 4 bytes. Calcula o padding.
    int padding = (4 - (width * 1) % 4) % 4;
    
    printf("Iniciando transferência para a FPGA...\n");

    // Loop de transferência (BMP armazena de baixo para cima, y = height-1 até 0)
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            
            uint8_t gray_pixel;
            if (fread(&gray_pixel, 1, 1, file) < 1) { // Lê 1 byte (pixel)
                gray_pixel = 0; // Padrão em caso de falha de leitura
            }
            
            // Calcula o endereço linear na memória da FPGA
            uint32_t fpga_addr = (uint32_t)(y * width + x);

            // Limite de segurança (ex: 320*240 = 76800)
            if (fpga_addr > 76799) break; 

            // Chama a API Assembly para escrever o pixel no hardware
            coproc_write_pixel(fpga_addr, gray_pixel);
            // Chama a API Assembly para esperar a confirmação (polling)
            coproc_wait_done(); 
        }
        
        // Pula os bytes de padding no final de cada linha
        fseek(file, padding, SEEK_CUR);
        
        // Feedback de progresso
        if (y > 0 && y % (height/10) == 0) {
            printf("Progresso: %d%%\n", (int)(((float)(height - y) / height) * 100));
        }
    }
    
    printf("Transferência de imagem concluída.\n");
    fclose(file);
    return 0; // Sucesso
}


// =================================================================
// Menu Interativo
// =================================================================

// Tipos enumerados para os modos de zoom (melhora a legibilidade)
typedef enum {
    ZOOM_OUT_NEAREST_NEIGHBOR,
    ZOOM_OUT_BLOCK_AVERAGE
} ZoomOutMode;

typedef enum {
    ZOOM_IN_PIXEL_REPETITION,
    ZOOM_IN_NEAREST_NEIGHBOR
} ZoomInMode;

// Variáveis globais para armazenar o estado atual da seleção de algoritmo
ZoomOutMode current_zoom_out_mode = ZOOM_OUT_BLOCK_AVERAGE;
ZoomInMode  current_zoom_in_mode  = ZOOM_IN_PIXEL_REPETITION;

// Estruturas para salvar os estados do terminal (modos canônico e não-canônico)
static struct termios old_termios, new_termios;

/**
 * @brief Configura o terminal para modo não-canônico e desliga o 'echo'.
 * Isso permite a leitura de caracteres únicos sem buffer de linha (sem precisar de Enter).
 */
void set_terminal_mode() {
    tcgetattr(STDIN_FILENO, &old_termios); // Salva o estado atual
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); // Desliga modo canônico e echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios); // Aplica
}

/**
 * @brief Restaura o terminal para o modo canônico original.
 */
void restore_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios); // Restaura o estado salvo
}

/**
 * @brief Exibe o menu de ajuda, mostrando o estado atual dos algoritmos selecionados.
 */
void print_menu() {
    printf("\n--- Controle Interativo da FPGA (Híbrido C+ASM) ---\n");
    printf("Controles de Zoom:\n");
    printf("  [i] ou [+]: Zoom In\n");
    printf("  [o] ou [-]: Zoom Out\n");
    printf("\nSeleção de Algoritmo:\n");
    // Exibe o estado atual da variável de modo Zoom OUT
    printf("  [m]: Alternar modo de Zoom OUT (Atual: %s)\n", 
           (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) ? 
           "Media de Blocos" : "Vizinho Mais Proximo");
    // Exibe o estado atual da variável de modo Zoom IN
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

/**
 * @brief Gerencia a lógica de carregamento de uma nova imagem durante a execução.
 */
void handle_load_image() {
    char filename[256];
    
    // Restaura o terminal temporariamente para permitir entrada de texto (scanf)
    restore_terminal_mode();
    
    printf("\n--- Carregar Nova Imagem ---\n");
    printf("Digite o caminho para a imagem .bmp: ");
    
    // Lê o nome do arquivo
    scanf("%255s", filename);
    
    // Limpa o buffer de entrada (consome o '\n' deixado pelo scanf)
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    printf("Carregando '%s'...\n", filename);
    
    // Tenta carregar a imagem
    if (load_bmp_image(filename) != 0) {
        printf("Falha ao carregar a imagem.\n");
    } else {
        // Se carregar com sucesso, envia o RESET para a FPGA (ex: copiar mem1 -> mem2)
        printf("Imagem carregada. Enviando comando de RESET para exibir...\n");
        coproc_reset_image(); // Chama ASM
        coproc_wait_done();   // Chama ASM
        printf("Imagem exibida.\n");
    }
    
    // Retorna o terminal ao modo não-canônico
    set_terminal_mode();
    print_menu(); // Mostra o menu novamente
}

/**
 * @brief Loop principal de eventos do programa. Aguarda a entrada do usuário
 * e despacha os comandos para a API Assembly.
 */
void enter_control_loop() {
    char c;
    set_terminal_mode(); // Entra em modo de caractere único
    print_menu();

    while (1) { // Loop infinito de eventos
        c = getchar(); // Lê um único caractere
        
        if (c == 'q' || c == 'Q') {
            break; // Sai do loop
        }

        switch (c) {
            case 'i':
            case '+': // Zoom In
                if (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) {
                    coproc_apply_zoom(OP_PR_ALG); 
                    coproc_wait_done();           
                } else {
                    coproc_apply_zoom(OP_NHI_ALG);
                    coproc_wait_done();
                }
                break;
                
            case 'o':
            case '-': // Zoom Out
                if (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) {
                    coproc_apply_zoom(OP_BA_ALG);
                    coproc_wait_done();
                } else {
                    coproc_apply_zoom(OP_NH_ALG);
                    coproc_wait_done();
                }
                break;
                
            case 'm':
            case 'M': // Alterna modo de Zoom Out
                if (current_zoom_out_mode == ZOOM_OUT_BLOCK_AVERAGE) {
                    current_zoom_out_mode = ZOOM_OUT_NEAREST_NEIGHBOR;
                } else {
                    current_zoom_out_mode = ZOOM_OUT_BLOCK_AVERAGE;
                }
                break;

            case 'n':
            case 'N': // Alterna modo de Zoom In
                if (current_zoom_in_mode == ZOOM_IN_PIXEL_REPETITION) {
                    current_zoom_in_mode = ZOOM_IN_NEAREST_NEIGHBOR;
                } else {
                    current_zoom_in_mode = ZOOM_IN_PIXEL_REPETITION;
                }
                break;

            case 'r':
            case 'R': // Resetar imagem
                printf("Resetando imagem para o original...\n");
                coproc_reset_image(); // Chama ASM
                coproc_wait_done();   // Chama ASM
                printf("Reset concluído.\n");
                break;
            
            case 'l':
            case 'L': // Carregar nova imagem
                handle_load_image();
                break;
                
            case 'h':
            case 'H': // Ajuda (Help)
                print_menu();
                break;
        }
    }
    
    restore_terminal_mode(); // Restaura o terminal ao sair do loop
}

// =================================================================
// Função Principal (main)
// =================================================================

/**
 * @brief Ponto de entrada do programa HPS.
 */
int main(int argc, char *argv[]) {
    
    printf("=== Programa de Teste - Híbrido C + Assembly ===\n\n");
    
    // ETAPA 1: Configurar o Mapeamento de Memória (MMIO)
    printf("Etapa 1:
