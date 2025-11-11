# Zoom Digital: Módulo de Redimensionamento de Imagens

## 1. Visão Geral do Projeto

Este projeto foi desenvolvido como parte da avaliação da disciplina de Sistemas Digitais (TEC499) do curso de Engenharia de Computação da Universidade Estadual de Feira de Santana (UEFS). O objetivo principal é projetar um módulo embarcado para redimensionamento de imagens em tempo real, aplicando efeitos de zoom in e zoom out.

Nesta etapa, o sistema passa a ser controlado pelo processador HPS (ARM) , que executa uma aplicação em C. Esta aplicação, acessada pelo usuário via terminal SSH, aceita comandos do teclado e utiliza a API em Assembly para enviar os comandos de processamento ao coprocessador na FPGA.

## 2. Requisitos do Projeto

A seguir estão os requisitos funcionais e não funcionais a serem desenvolvidos o durante este projeto.

### 2.1. Requisitos Funcionais

* **RF01:** O sistema deve implementar algoritmos de zoom in (aproximação) e zoom out (redução) em imagens.
* **RF02:** As operações de redimensionamento devem ser em passos de 2X.
* **RF03:** O algoritmo de **Vizinho Mais Próximo** (*Nearest Neighbor*) deve ser implementado para a aproximação.
* **RF04:** O algoritmo de **Replicação de Pixel** deve ser implementado para a aproximação.
* **RF05:** O algoritmo de **Decimação/Amostragem** deve ser implementado para a redução.
* **RF06:** O algoritmo de **Média de Blocos** deve ser implementado para a redução.
* **RF07:** A seleção da operação (zoom in/out) deve ser controlada por chaves e/ou botões da placa.
* **RF08:** A imagem processada deve ser exibida em um monitor através da saída VGA.

### 2.2. Requisitos Não Funcionais

* **RNF01:** O código da API deve ser escrito em linguagem Assembly;
* **RNF02:** O projeto deve utilizar apenas os componentes disponíveis na placa de desenvolvimento **DE1-SoC**.
* **RNF03:** As imagens devem ser representadas em escala de cinza, com cada pixel codificado por um inteiro de 8 bits.
* **RNF04:** O coprocessador deve ser compatível com o processador ARM (Hard Processor System - HPS) para viabilizar o desenvolvimento da solução.
* **RNF05:** A imagem deve ser lida a partir de um arquivo e transferida para o coprocessador;
* **RNF06:** Deverão ser implementados na API os comandos da ISA do coprocessador. As instruções devem utilizar as operações que foram anteriormente implementadas via chaves e botões na placa (vide Problema 1);

## 3. Ambiente de Desenvolvimento

### 3.1. Software Utilizado

| Software | Versão | Descrição |
| :--- | :--- | :--- |
| Quartus Prime | 23.1.0 | Ferramenta de desenvolvimento para FPGAs Intel. |

### 3.2. Hardware Utilizado

| Componente | Especificação |
| :--- | :--- |
| Kit de Desenvolvimento | Terasic DE1-SoC |
| Monitor | Monitor com entrada VGA. |
| Computador | Para compilação do projeto e controle do Zoom In e Zoom Out |

## 4. Instalação e Configuração

1.  **Clonar o Repositório:**
    ```bash
    git clone [https://github.com/VitorAugusto210/SD-Problema2.git](https://github.com/VitorAugusto210/SD-Problema2.git)
    cd <NOME_DO_SEU_REPOSITORIO>
    ```
2.  **Configuração do Quartus Prime:**
    * Abra o Quartus Prime.
    * Abra o arquivo de projeto `.qpf`.
3.  **Compilação:**
    * Com o Quartus aberto, clique no botão de Start Compilation.
        ![botao_compilar](imgs/start_compilation.png)
    para gerar o arquivo de programação (`.sof`).
    * Ainda no Quartus, vá em Programmer
4.  **Programação da FPGA:**
    * Conecte a placa DE1-SoC ao computador.
    * Abra o "Programmer" no Quartus Prime.
        ![programmer](imgs/programmer.png)
    * Selecione o arquivo `.sof` gerado e programe a placa.
    * Clique em "Start" e as instruções serão repassadas a placa.

## 5. Manual do Usuário

| Teclas | Ação |
| :--- | :--- |
| "i" ou + | Selecionar Zoom In |
| "o" ou - | Selecionar Zoom Out |
| "n" | Alternar Modo de Zoom In |
| "m" | Alternar Modo de Zoom Out |
| "l" | Carregar nova imagem em Bitmap |
| "r" | Resetar imagem (recarrega da original) |
| "h" | Voltar para o Menu Inicial |
| "q" | Sair do programa. |

## 6. Descrição da Solução
Abaixo consta a descrição de cada modulo criado para a solução do projeto.

### `soc_system.qsys` (Sistema HPS e Barramento)

Este arquivo, criado no Platform Designer (Qsys), define o sistema de processamento principal e sua conexão com a lógica da FPGA.

* **Propósito:** Configurar o processador **ARM (HPS)** e criar a ponte de comunicação (barramento Avalon) entre o software (executando no HPS) e o hardware (nosso coprocessador na FPGA).
* **Componentes Chave:**
    * `hps_0`: O próprio Hard Processor System (HPS), que gerencia a memória DDR3 e os periféricos principais.
    * `h2f_lw_axi_master`: A ponte "Lightweight HPS-to-FPGA" (Ponte Leve HPS-para-FPGA). É através deste barramento que o HPS envia comandos para o coprocessador.
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
        1.  `memory1`: "Memória da Imagem Original". É aqui que o HPS escreve a imagem (via instrução `STORE`) e de onde os algoritgos de *downscale* (redução) leem.
        2.  `memory2`: "Memória de Exibição". Este bloco é lido continuamente pelo `vga_module` para gerar o sinal de vídeo. O resultado final dos algoritmos é copiado para cá.
        3.  `memory3`: "Memória de Trabalho". Os algoritmos de *upscale* (ampliação) e *downscale* (redução) escrevem seus resultados nesta memória temporária.
    * **Máquina de Estados Finitos (FSM):** O `case (uc_state)` principal gerencia todo o fluxo de controle. Possui estados como:
        * `IDLE`: Aguardando um novo comando (pulso em `ENABLE`).
        * `READ_AND_WRITE`: Executa as instruções `LOAD` (leitura) e `STORE` (escrita) vindas do HPS.
        * `ALGORITHM`: Estado complexo que executa a lógica de pixel-a-pixel para o algoritmo de zoom selecionado (`PR_ALG`, `NHI_ALG`, `BA_ALG`, `NH_ALG`).
        * `COPY_READ`/`COPY_WRITE`: Estados usados para transferir a imagem processada (da `memory1` ou `memory3`) para a `memory2` (exibição).
    * **Controlador VGA (`vga_module`):** Instancia o módulo VGA, que varre a `memory2` com base nas coordenadas `next_x` e `next_y` e gera os sinais de sincronismo e cores (R, G, B) para o monitor.

### `mem1.v` (Módulo de Memória)

Este arquivo é um invólucro (wrapper) para um bloco de memória `altsyncram`, gerado pelo MegaFunction Wizard da Intel.

* **Propósito:** Definir um bloco de memória RAM síncrona de porta dupla (Dual-Port).
* **Configuração:**
    * **Modo:** `DUAL_PORT`. Isso é crucial, pois permite que a FSM escreva na memória (Porta A) ao mesmo tempo em que o controlador VGA lê dela (Porta B).
    * **Tamanho:** `WIDTH_A = 8` (8 bits de dados, para escala de cinza) e `WIDTHAD_A = 17` (17 bits de endereço). Isso fornece 131.072 endereços, mais do que o suficiente para os 76.800 pixels (320x240) necessários.
    * **Inicialização:** A memória é configurada para ser pré-carregada com o arquivo `../imagem_output.mif` durante a síntese.

## 7. Testes e Validação
Foram realizados testes de mesa pelo terminal do HPS comparando o comportamento do redimensionamento da imagem por cada algoritmo após utilização de cada tecla

### 7.1. Teste de Zoom In

**Imagem Original vs. Vizinho Mais Próximo vs. Replicação de Pixel**

| Original | Vizinho Mais Próximo | Replicação de Pixel |
| :--- | :--- | :--- |
| inserir aqui imagem| inserir aqui gif | inserir aqui gif |

### 7.2. Teste de Zoom Out

*Demonstre a aplicação do zoom out. Compare o resultado dos dois algoritmos implementados.*

**Imagem Original vs. Decimação vs. Média de Blocos**

| Original | Decimação | Média de Blocos |
| :--- | :--- | :--- |
| inserir aqui imagem | inserir aqui gif | inserir aqui gif |

## 8. Análise dos Resultados

* **Comparativo Visual:** Zoom In: Ambos algoritmos não apresentaram diferenças visiveis tanto na imagem original quanto na imagem original amplificada.
Zoom Out: Ambos algoritmos não apresentaram diferenças visiveis tanto na imagem original quanto na imagem original diminuida.

* **Limitações:** Algumas Imagens mal convertidas apresentam alguns ruidos de cores.