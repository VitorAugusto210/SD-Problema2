# Zoom Digital: Módulo de Redimensionamento de Imagens

## 1. Visão Geral do Projeto

Este projeto foi desenvolvido como parte da avaliação da disciplina de Sistemas Digitais (TEC499) do curso de Engenharia de Computação da Universidade Estadual de Feira de Santana (UEFS). O objetivo principal é projetar um módulo embarcado para redimensionamento de imagens em tempo real, aplicando efeitos de zoom in e zoom out.

Nesta etapa, o sistema passa a ser controlado pelo processador HPS (ARM) , que executa uma aplicação em C. Esta aplicação, acessada pelo usuário via terminal SSH, aceita comandos do teclado e utiliza a API em Assembly para enviar os comandos de processamento ao coprocessador na FPGA.

A base do codigo em Verilog foi fornecida pelo seguinte repositório: <https://github.com/DestinyWolf/Problema-SD-2025-2> com devida permissão do Autor.

## 2. Definição do Problema

O tema deste problema é a **Programação Assembly e a construção de um driver de software** para a interface hardware-software.

O objetivo é projetar um **módulo embarcado de redimensionamento de imagens** (zoom in/out) para sistemas de vigilância e exibição em tempo real.

Este repositório consiste em construir uma biblioteca de funções (API) em Assembly. Esta API permite que o HPS (controlador) se comunique com o coprocessador gráfico (na FPGA) através de um repertório de instruções.


## 3. Requisitos do Projeto

A seguir estão os requisitos funcionais e não funcionais a serem desenvolvidos o durante este projeto.

### 3.1. Requisitos Funcionais

* **RF01:** O sistema deve implementar algoritmos de zoom in (aproximação) e zoom out (redução) em imagens.
* **RF02:** As operações de redimensionamento devem ser em passos de 2X.
* **RF03:** O algoritmo de **Vizinho Mais Próximo** (*Nearest Neighbor*) deve ser implementado para a aproximação.
* **RF04:** O algoritmo de **Replicação de Pixel** deve ser implementado para a aproximação.
* **RF05:** O algoritmo de **Decimação/Amostragem** deve ser implementado para a redução.
* **RF06:** O algoritmo de **Média de Blocos** deve ser implementado para a redução.
* **RF07:** A seleção da operação (zoom in/out) deve ser controlada pelo teclado do computador integrado a placa pelo HPS.
* **RF08:** A imagem processada deve ser exibida em um monitor através da saída VGA.

### 3.2. Requisitos Não Funcionais

* **RNF01:** O código da API deve ser escrito em linguagem Assembly;
* **RNF02:** O projeto deve utilizar apenas os componentes disponíveis na placa de desenvolvimento **DE1-SoC**.
* **RNF03:** As imagens devem ser representadas em escala de cinza, com cada pixel codificado por um inteiro de 8 bits.
* **RNF04:** O coprocessador deve ser compatível com o processador ARM (Hard Processor System - HPS) para viabilizar o desenvolvimento da solução.
* **RNF05:** A imagem deve ser lida a partir de um arquivo e transferida para o coprocessador;
* **RNF06:** Deverão ser implementados na API os comandos da ISA do coprocessador. As instruções devem utilizar as operações que foram anteriormente implementadas via chaves e botões na placa (vide Problema 1);

## 4. Fundamentação Teórica

Esta seção detalha a teoria por trás dos algoritmos de redimensionamento de imagem implementados.

### 4.1. Zoom In (Aproximação)

Aproximar uma imagem significa criar novos pixels onde antes não existia informação.

* **Replicação de Pixel [RF04] / Vizinho Mais Próximo [RF03]**
    * No contexto de um zoom 2x, ambos são funcionalmente idênticos.
    * **Teoria:** A Replicação de Pixel, para um zoom 2x, simplesmente "estica" a imagem, fazendo com que cada pixel original se torne um bloco de 2x2 pixels na imagem de destino.
    * **Funcionamento:** Um pixel na posição `(x, y)` da imagem original é replicado para as posições `(2x, 2y)`, `(2x+1, 2y)`, `(2x, 2y+1)` e `(2x+1, 2y+1)` da imagem de destino.
    * **Resultado:** É um algoritmo computacionalmente leve (rápido), mas que produz um resultado "serrilhado" ou "pixelado", com bordas visivelmente quadradas.

### 4.2. Zoom Out (Redução)

Reduzir uma imagem significa descartar informações (pixels) de forma inteligente.

* **Decimação / Amostragem [RF05]**
    * **Teoria:** Este é o método mais rápido e computacionalmente simples de redução. Também é conhecido como "subamostragem".
    * **Funcionamento:** O algoritmo simplesmente "pula" pixels. Para uma redução 2x, ele lê o valor de 1 em cada 2 pixels, tanto na horizontal quanto na vertical, descartando os 75% de pixels restantes.
    * **Resultado:** É extremamente rápido, mas a qualidade da imagem resultante é baixa. A grande perda de dados pode causar "aliasing" (bordas serrilhadas) e perda de detalhes finos.

* **Média de Blocos [RF06]**
    * **Teoria:** Este método produz um resultado de qualidade superior ao da decimação, pois considera todos os pixels da imagem original.
    * **Funcionamento:** Para cada pixel na imagem de destino (reduzida), o algoritmo calcula a média de um "bloco" de pixels correspondente na imagem original. Para uma redução 2x, ele lê um bloco de 2x2 pixels, soma seus valores de intensidade e divide por 4 (média).
    * **Resultado:** A imagem resultante é mais suave, com menos serrilhado ("aliasing") e representa melhor a imagem original, pois nenhuma informação é completamente descartada.

## 5. Ambiente de Desenvolvimento

### 5.1. Software Utilizado

| Software | Versão | Descrição |
| :--- | :--- | :--- |
| Quartus Prime | 23.1.0 | Ferramenta de desenvolvimento para FPGAs Intel. |

### 5.2. Hardware Utilizado

| Componente | Especificação |
| :--- | :--- |
| Kit de Desenvolvimento | Terasic DE1-SoC |
| Monitor | Monitor com entrada VGA. |
| Computador | Para compilação do projeto e controle do Zoom In e Zoom Out |

## 6. Manual do Usuário

Esta seção descreve como instalar, configurar e operar o sistema, servindo como o manual do usuário do projeto.

### 6.1. Instalação e Configuração

1.  **Clonar o Repositório:**
    ```bash
    git clone [https://github.com/VitorAugusto210/SD-Problema2.git](https://github.com/VitorAugusto210/SD-Problema2.git)
    cd <NOME_DO_SEU_REPOSITORIO>
    ```
2.  **Configuração do Quartus Prime:**
    * Abra o Quartus Prime.
    * Abra o arquivo de projeto `.qpf`.
3.  **Compilação (Hardware):**
    * Com o Quartus aberto, clique no botão de Start Compilation.
        ![botao_compilar](imgs/start_compilation.png)
    * Isso irá gerar o arquivo de programação (`.sof`).
    * Ainda no Quartus, vá em Programmer.
4.  **Programação da FPGA:**
    * Conecte a placa DE1-SoC ao computador.
    * Abra o "Programmer" no Quartus Prime.
        ![programmer](imgs/programmer.png)
    * Selecione o arquivo `.sof` gerado e programe a placa.
    * Clique em "Start" e as instruções serão repassadas a placa.

5.  **Conectando e Compilando o Software no HPS:**
    * Abra um terminal e conecte-se à placa via SSH:
        ```bash
        ssh aluno@172.65.213.<Digitos finais da Placa utilizada>
        # Forneça a senha da máquina assim que solicitada
        ```
    * Transfira os arquivos de software (`menu.c`, `constantes.h`, `api_fpga.s`) do seu computador para a placa usando `scp`, ou crie-os manualmente usando `nano`:
        ```bash
        # Exemplo com nano
        nano api_fpga.s
        # (Cole o conteúdo do arquivo e salve)

        nano constantes.h
        # (Cole o conteúdo do arquivo e salve)

        nano menu.c
        # (Cole o conteúdo do arquivo e salve)
        ```
    
    * Transfira a imagem bitmap desejada para a placa:
        ```bash
        # Em um terminal no SEU computador
        scp /<Diretorio da imagem em bitmap> aluno@172.65.213.<...>:~/
        ```

    * Compile o software no HPS:
        ```bash
        make # Makefile que faz toda compilação
        sudo ./programa_final # Comando para rodar o programa
        ```

### 6.2. Comandos de Operação

Após executar o programa (`sudo ./programa_final`), os seguintes comandos estão disponíveis:

| Teclas | Ação |
| :--- | :--- |
| Setas | Direcionar a "janela" de Zoom |
| "i" ou + | Selecionar Zoom In |
| "o" ou - | Selecionar Zoom Out |
| "n" | Alternar Modo de Zoom In *Note que após alterar, o menu irá ser modificado, mostrando o algoritmo de seleção atual |
| "m" | Alternar Modo de Zoom Out *Note que após alterar, o menu irá ser modificado, mostrando o algoritmo de seleção atual |
| "l" | Carregar nova imagem em Bitmap *Imagem precisa estar dentro da placa |
| "r" | Resetar imagem (recarrega para a imagem no formato original) |
| "h" | Voltar para o Menu Inicial |
| "q" | Sair do programa. |

## 7. Descrição da Solução
Abaixo consta a descrição de cada modulo criado para a solução do projeto.

### `soc_system.qsys` (Sistema HPS e Barramento)

Criado no Platform Designer (Qsys), ele define o sistema de processamento principal e sua conexão com a lógica da FPGA.

* **Propósito:** Configurar o processador **ARM (HPS)** e criar a ponte de comunicação (barramento Avalon) entre o software (executando no HPS) e o hardware (Coprocessador na FPGA).
* **Componentes Chave:**
    * `hps_0`: O próprio Hard Processor System (HPS), que gerencia a memória e os periféricos principais.
    * `h2f_lw_axi_master`: A ponte "Lightweight HPS-to-FPGA". É através deste barramento que o HPS envia comandos para o coprocessador.
* **Interface de Comunicação (PIOs):** A comunicação entre o HPS e o coprocessador é realizada através de quatro periféricos PIO, que são mapeados em endereços de memória específicos para o HPS:
    * `pio_instruct` (Saída, 29 bits): Mapeado em `0x0000`. Usado pelo HPS para enviar o barramento completo de instrução (opcode, endereço de memória e valor) para o coprocessador.
    * `pio_enable` (Saída, 1 bit): Mapeado em `0x0010`. Usado pelo HPS para enviar um pulso de "enable" (habilitação) que inicia a operação no coprocessador.
    * `pio_dataout` (Entrada, 8 bits): Mapeado em `0x0020`. Usado pelo HPS para ler dados de resultado (como o valor de um pixel) do coprocessador.
    * `pio_flags` (Entrada, 4 bits): Mapeado em `0x0030`. Usado pelo HPS para ler bits de status, como `FLAG_DONE`, `FLAG_ERROR`, `FLAG_ZOOM_MAX` e `FLAG_ZOOM_MIN`.

### `ghrd_top.v` (Arquivo Top-Level)

Este é o arquivo Verilog de nível mais alto do projeto. Ele representa o design completo da FPGA, conectando os blocos lógicos aos pinos físicos da placa.

* **Propósito:** Instanciar e "conectar" o sistema HPS (`soc_system`) e o nosso coprocessador (`main.v`) um ao outro e aos pinos externos da placa DE1-SoC.
* **Instâncias Principais:**
    1.  `soc_system u0 (...)`: Instancia o sistema HPS/Qsys. Este bloco mapeia as portas lógicas do HPS (ex: `memory_mem_a`) para os pinos físicos da placa (ex: `HPS_DDR3_ADDR`).
    2.  `main main_inst (...)`: Instancia o nosso módulo lógico principal (`main.v`), que atua como o coprocessador.
* **Conexões Chave:**
    * **HPS <-> Coprocessador:** O `ghrd_top.v` conecta os fios de exportação dos PIOs do `soc_system` às portas de entrada/saída do `main_inst`. Por exemplo, o fio `pio_instruct` (vindo do HPS) é roteado para as entradas `INSTRUCTION`, `DATA_IN` e `MEM_ADDR` do módulo `main`. As saídas `FLAG_DONE` do `main` são conectadas ao fio `pio_flags` (indo para o HPS).
    * **Coprocessador -> Pinos da Placa:** Conecta as saídas de vídeo do `main_inst` (como `VGA_R`, `VGA_G`, `VGA_B`, `VGA_HS`, etc.) diretamente às portas correspondentes da placa, que levam ao conector VGA.

### `main.v` (Módulo do Coprocessador)

Este é o coração da lógica de FPGA customizada. Ele contém toda a lógica de processamento de imagem e responde aos comandos recebidos do HPS.

* **Propósito:** Implementar a Máquina de Estados Finitos (FSM) e o *datapath* (caminho de dados) para os algoritmos de zoom e gerenciamento de memória.
* **Componentes Chave:**
    * **PLL (`pll0`):** Gera os clocks necessários para o sistema: `clk_100` (100MHz) para a FSM e lógicas internas, e `clk_25_vga` (25MHz) para o controlador VGA.
    * **Memórias (`mem1`):** O módulo `main` instancia **três** blocos de memória RAM:
        1.  `memory1`: "Memória da Imagem Original". É aqui que o HPS escreve a imagem (via instrução `STORE`) e de onde os algoritmos de *downscale* (redução) leem.
        2.  `memory2`: "Memória de Exibição". Este bloco é lido continuamente pelo `vga_module` para gerar o sinal de vídeo. O resultado final dos algoritmos é copiado para cá.
        3.  `memory3`: "Memória de Trabalho". Os algoritmos de *upscale* (ampliação) e *downscale* (redução) escrevem seus resultados nesta memória temporária.
    * **Máquina de Estados Finitos (FSM):** O `case (uc_state)` principal gerencia todo o fluxo de controle. Possui estados como:
        * `IDLE`: Aguardando um novo comando (pulso em `ENABLE`).
        * `READ_AND_WRITE`: Executa as instruções `LOAD` (leitura) e `STORE` (escrita) vindas do HPS.
        * `ALGORITHM`: Estado complexo que executa a lógica de pixel-a-pixel para o algoritmo de zoom selecionado (`PR_ALG`, `NHI_ALG`, `BA_ALG`, `NH_ALG`).
        * `COPY_READ`/`COPY_WRITE`: Estados usados para transferir a imagem processada (da `memory1` ou `memory3`) para a `memory2` (exibição).
    * **Controlador VGA (`vga_module`):** Instancia o módulo VGA, que varre a `memory2` com base nas coordenadas `next_x` e `next_y` e gera os sinais de sincronismo e cores (R, G, B) para o monitor.

### `mem1.v` (Módulo de Memória)

Este arquivo é um wrapper para um bloco de memória `altsyncram`, gerado pelo MegaFunction Wizard da Intel.

* **Propósito:** Definir um bloco de memória RAM síncrona de porta dupla (Dual-Port).
* **Configuração:**
    * **Modo:** `DUAL_PORT`. Isso é crucial, pois permite que a FSM escreva na memória (Porta A) ao mesmo tempo em que o controlador VGA lê dela (Porta B).
    * **Tamanho:** `WIDTH_A = 8` (8 bits de dados, para escala de cinza) e `WIDTHAD_A = 17` (17 bits de endereço). Isso fornece 131.072 endereços, mais do que o suficiente para os 76.800 pixels (320x240) necessários.
    * **Inicialização:** A memória é configurada para ser pré-carregada com o arquivo `../imagem_output.mif` durante a síntese. (que não é utilizada nesse projeto)

### `api_fpga.s` (A API de Hardware em Assembly)

Este é o arquivo de mais baixo nível da parte de software, atuando como o "driver" direto do nosso coprocessador.

* **Propósito:** Fornecer funções que o código C (`menu.c`) pode chamar para interagir diretamente com o hardware da FPGA. Ele é escrito em Assembly (ARM) porque precisa de controle absoluto para ler e escrever em endereços de memória físicos (os PIOs).

* **`setup_memory_map()`**
    * **Argumentos:** Nenhum.
    * **Descrição:** Função de inicialização essencial. Mapeia os endereços de memória físicos dos PIOs (hardware da FPGA) para endereços de memória virtuais que o programa C pode ler. **Deve ser chamada no início do programa.**

* **`cleanup_memory_map()`**
    * **Argumentos:** Nenhum.
    * **Descrição:** Função de finalização. Desfaz o mapeamento de memória (`munmap`) antes de o programa terminar, liberando os recursos.

* **`coproc_write_pixel(x, y, pixel_value)`**
    * **Argumentos:** `x` (int), `y` (int), `pixel_value` (int).
    * **Descrição:** Envia a instrução `STORE`. Calcula o endereço linear `(y * 320) + x` e envia o `opcode`, o `endereço` e o `pixel_value` para o hardware. Em seguida, pulsa o `enable` para iniciar a escrita na memória da FPGA.

* **`coproc_read_pixel(x, y, mem_select)`**
    * **Argumentos:** `x` (int), `y` (int), `mem_select` (int).
    * **Descrição:** Envia a instrução `LOAD`. Monta a instrução com o `opcode`, o `endereço` e o bit `mem_select`. Pulsa o `enable`, espera o hardware (chamando `coproc_wait_done`), lê o resultado do `pio_dataout` e retorna o valor do pixel lido.

* **`coproc_apply_zoom(algorithm_code)`**
    * **Argumentos:** `algorithm_code` (int).
    * **Descrição:** Envia uma instrução de algoritmo de zoom (ex: `INST_PR_ALG`) para o hardware. Esta versão não envia offsets, sendo usada para aplicar o zoom na imagem inteira.

* **`coproc_reset_image()`**
    * **Argumentos:** Nenhum.
    * **Descrição:** Envia a instrução `INST_RESET` para o hardware, fazendo com que a FSM recarregue a imagem original na memória de exibição.

* **`coproc_wait_done()`**
    * **Argumentos:** Nenhum.
    * **Descrição:** Função de bloqueio (sincronização). Entra num loop que lê continuamente o `pio_flags` até que o `FLAG_DONE` (bit 0) seja definido como 1 pelo hardware.

* **`coproc_apply_zoom_with_offset(algorithm_code, x_offset, y_offset)`**
    * **Argumentos:** `algorithm_code` (int), `x_offset` (int), `y_offset` (int).
    * **Descrição:** Envia uma instrução de zoom (como `INST_PR_ALG`) juntamente com os offsets X e Y. O hardware utiliza estes offsets para calcular a "janela" de zoom.

* **`coproc_pan_zoom_with_offset(algorithm_code, x_offset, y_offset)`**
    * **Argumentos:** `algorithm_code` (int), `x_offset` (int), `y_offset` (int).
    * **Descrição:** Similar à função anterior, mas também ativa o bit `SEL_MEM` (bit 20). Isto sinaliza ao hardware para executar uma operação de "pan" (mover a janela de zoom) em vez de aplicar um novo zoom.

### `constantes.h` (O Dicionário do Projeto)

Este é um arquivo de cabeçalho (header) C que serve como um "dicionário" central, garantindo que o `menu.c` (software) e o `main.v` (hardware) "falem a mesma língua".

* **Propósito:** Definir nomes legíveis para todos os valores numéricos do projeto, como endereços de hardware e códigos de instrução.
* **Definições Contidas:**
    * **Endereços dos PIOs:** Define os endereços físicos dos registradores PIO criados no Qsys (ex: `PIO_INSTRUCT_BASE 0x0000`, `PIO_FLAGS_BASE 0x0030`).
    * **Opcodes das Instruções:** Define os códigos de 3 bits para cada operação que a FSM do `main.v` entende (ex: `INST_LOAD 0b001`, `INST_PR_ALG 0b100`, `INST_RESET 0b111`).
    * **Máscaras de Flags:** Define máscaras de bits para facilitar a leitura do `pio_flags` (ex: `FLAG_DONE 0b0001`, `FLAG_ZOOM_MAX 0b0100`).

### `menu.c` (A Aplicação Principal)

Este é o programa C principal que o usuário executa no terminal SSH do HPS.

* **Propósito:** Fornecer a interface do usuário (o menu) e orquestrar as operações do coprocessador.
* **Como Funciona:**
    1.  **Inclui Definições:** Inclui `constantes.h` para usar os nomes legíveis dos endereços e opcodes.
    2.  **Declara Funções Assembly:** Declara os protótipos das funções que estão em `api_fpga.s` (ex: `extern void enviar_instrucao(int instrucao);`).
    3.  **Lógica do Menu:** Contém o loop principal (`while(1)`) que imprime o menu, espera o usuário digitar uma tecla (`getchar()`) e usa um `switch-case` para decidir o que fazer.
    4.  **Chamada da API:** Quando o usuário pressiona uma tecla (ex: 'i' para zoom in), o `menu.c` chama as funções da API em Assembly (ex: `enviar_instrucao()`, `enviar_enable()`) e depois entra em um loop de espera, verificando o `ler_status()` até que o bit `FLAG_DONE` seja ativado pelo hardware.
    5.  **Carregamento de Imagem:** A função para a tecla 'l' (Carregar Bitmap) abre o arquivo `.bmp`, lê o cabeçalho, e envia cada pixel para o hardware usando a instrução `INST_STORE` repetidamente.

## 8. Testes e Validação
Foram realizados testes de mesa pelo terminal do HPS comparando o comportamento do redimensionamento da imagem por cada algoritmo após utilização de cada tecla

### 8.1. Teste de Zoom In

**Vizinho Mais Próximo**

![vizinho-mais-proximo](imgs/vizinho-proximo.gif)

**Replicação de Pixel**

![vizinho-mais-proximo](imgs/replicacao-pixel.gif)

### 8.2. Teste de Zoom Out

**Média de Blocos**

![vizinho-mais-proximo](imgs/media-blocos.gif)

**Decimação**

![decimação](imgs/decimacao.gif)

### 8.3 Seleção de "Janela" de Zoom

![seleção-janela-zoom](imgs/selecao-janela.gif)



## 9. Análise dos Resultados

* **Comparativo Visual:** Zoom In: Ambos algoritmos não apresentaram diferenças visiveis tanto na imagem original quanto na imagem original amplificada.
Zoom Out: Ambos algoritmos não apresentaram diferenças visiveis tanto na imagem original quanto na imagem original diminuida.

* **Limitações:** Algumas Imagens mal convertidas apresentam alguns ruidos de cores.