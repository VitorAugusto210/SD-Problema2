module memory_control (
    addr_base,
    clock,
    operation,
    current_zoom,
    enable,
    addr_out,
    done,
    wr_enable,
    counter_op,
);

    // estado de aguardar uma nova instrução
    localparam IDLE = 3'b000;

    //operações de leitura ou escrita na memoria
    localparam RD_DATA = 3'b001;
    localparam WR_DATA = 3'b010;

    //algoritmos
    localparam NHI_ALG = 3'b011;
    localparam PR_ALG = 3'b100;
    localparam NH_ALG = 3'b101;
    localparam BA_ALG = 3'b110;
    
    //estado apenas para aguardar finalizar a operação de leitura ou escrita
    localparam WAIT_WR_RD = 3'b111;

    input [17:0] addr_base;
    input [2:0] operation;
    input [2:0] current_zoom;
    input enable, clock;
    output reg [17:0] addr_out;
    output reg done;
    output reg wr_enable;
    output counter_op;

    reg [18:0] num_steps_needed; // tempo que uma operação necessita para ser execultada

    reg [2:0] state; //estado atual

    reg [17:0] last_addr_rd; //ultimo endereço utilizado
    reg [17:0] last_addr_wr; //ultimo endereço utilizado
    reg [1:0] wr_rd_timer_counter; // conta os 3 ciclos necessarios para realizar uma operação de leitura ou escrita

    reg [2:0] last_op; //armazena qual foi a ultima operação enviada

    reg [18:0] algorithm_step_counter; //conta o numero de passos realizados em alguma operação

    reg [2:0] operation_step_counter; // armazena em qual passo do algoritimo está

    assign counter_op = operation_step_counter;

    reg has_alg_on_exec;

    reg [9:0] count_x_new, count_y_new, count_x_old, count_y_old;
    reg [17:0] addr_base_wr, addr_base_rd;


    always @(posedge clock) begin

        case (state)
                IDLE: begin
                    wr_enable <= 1'b0;
                    has_alg_on_exec <= 1'b0;
                    algorithm_step_counter <= 19'b0;
                    num_steps_needed <= 19'b0;
                    count_x_new <= 10'b0;
                    count_y_new <= 10'b0;
                    if (enable == 1'b1 && !done) begin
                        state <= operation;
                    end else  begin
                        done <= (enable) ? 1'b0:1'b1;
                        state <= IDLE;
                    end
                end

                WAIT_WR_RD: begin
                    wr_rd_timer_counter <= wr_rd_timer_counter + 1'b1;
                    if (wr_rd_timer_counter == 2'b11) begin //quando o contador chegar em 3
                        if (last_op == RD_DATA || last_op == WR_DATA) begin //se for operação de escrita
                            state <= IDLE; //vai pro idle
                        end else begin // se não
                            if (algorithm_step_counter == num_steps_needed) begin //se tiver chegado no numero de passos necessarios
                                state <= IDLE; //vai pro idle
                                has_alg_on_exec <= 1'b0; //desativa o sinal que tem um algoritmo em execução
                                done <= 1'b1; //ativa o sinal de pronto
                            end else begin // se não
                                has_alg_on_exec <= 1'b1; //ativa o sinal que tem um algoritmo em execução
                                state <= last_op; //retorna a ultima operação
                            end
                        end

                        wr_enable <= 1'b0;   //desabilita o sinal de escrita

                        case (last_op) // verifico qual a ultima operação realizada 
                            RD_DATA: begin //se for leitura, atualiza last_addr_rd
                                last_addr_rd <= addr_out;
                                last_addr_wr <= last_addr_wr;
                            end
                            WR_DATA: begin //se for escrita, atualiza last_addr_wr
                                last_addr_wr <= addr_out;
                                last_addr_rd <= last_addr_rd;
                            end
                            PR_ALG: begin //se for um dos algoritmos, atualiza last_addr_rd e last_addr_wr dependendo do passo
                                if (operation_step_counter == 3'b000) begin //leitura do dado
                                    last_addr_wr <= last_addr_wr;
                                    last_addr_rd <= addr_out;
                                end else begin  // escrita do dado
                                    last_addr_wr <= addr_out;
                                    last_addr_rd <= last_addr_rd;
                                end
                            end
                            default: begin
                                last_addr_rd <= last_addr_rd;
                                last_addr_wr <= last_addr_wr;
                            end
                        endcase
                    end else begin
                        state <= WAIT_WR_RD;
                    end
                end
                WR_DATA: begin
                    addr_out <= addr_base;  //endereço que sera utilizado
                    wr_enable <= 1'b1;  //habilita a escrita
                    wr_rd_timer_counter <= 2'b00; //reseta o contador de tempo para escrita e leitura
                    done <= 1'b0; //desabilita o sinal de pronto
                    last_op <= WR_DATA; //armazena a ultima operação
                end 
                
                RD_DATA: begin
                    addr_out <= addr_base;
                    wr_enable <= 1'b0;
                    wr_rd_timer_counter <= 2'b00;
                    done <= 1'b0;
                    last_op <= RD_DATA;
                end

                PR_ALG: begin
                    done <= 1'b0;
                    if (!has_alg_on_exec) begin // se não tem um algoritmo em execução = primeira execução
                        num_steps_needed <= 19'd95400;
                        state <= WAIT_WR_RD;
                        operation_step_counter <= 3'b001;
                        count_x_old <= 10'd80; //configura o range que será lido da imagem anterior
                        count_y_old <= 10'd60; //configura o range que será lido da imagem anterior
                        count_x_new <= 10'd00; //configura os valores da nova imagem para o começo
                        count_y_new <= 10'd00; //configura os valores da nova imagem para o começo
                        if (current_zoom == 3'b101) begin
                            addr_base_rd <= 18'd96000;
                            addr_base_wr <= 18'd153600;
                        end else begin
                            addr_base_rd <= 18'd19200;
                            addr_base_wr <= 18'd76800;
                        end
                    end else begin
                        case(operation_step_counter)
                            3'b000: begin
                                operation_step_counter <= operation_step_counter + 1'b1;

                                if (count_x_old == 10'd239) begin
                                    count_y_old <= count_y_old + 1'b1; //incrementa uma linha
                                    count_x_old <= 10'd80;             // volta pra primeira coluna
                                    addr_out <= addr_base_rd + (count_y_new*320) + 10'd80; //leitura na imagem antiga
                                end else begin
                                    count_x_old <= count_x_old + 1'b1;
                                    addr_out <= addr_base_rd + (count_y_new*320) + count_x_old; //leitura do endereço na imagem antiga
                                end
                                state <= WAIT_WR_RD;
                                
                            end
                            3'b001: begin
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_wr +(320*count_y_new) + count_x_new; 
                                wr_enable <= 1'b1;//habilita a escrita
                                state <= WAIT_WR_RD;
                                count_x_new <= count_x_new + 1'b1; //incrementa a coluna
                            end
                            3'b010: begin
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_wr +(320*count_y_new) + count_x_new; 
                                wr_enable <= 1'b1;//habilita a escrita
                                state <= WAIT_WR_RD;
                                count_x_new <= count_x_new - 1'b1; //decrementa uma coluna a coluna
                            end
                            3'b011: begin
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_wr +(320*count_y_new+1) + count_x_new; 
                                wr_enable <= 1'b1;//habilita a escrita
                                state <= WAIT_WR_RD;
                                count_x_new <= count_x_new + 1'b1; //incrementa a coluna
                            end
                            3'b100: begin
                                algorithm_step_counter <= algorithm_step_counter + 1'b1; // incrementa um a contagem de passos
                                operation_step_counter <= 3'b000; //reseta o contador da operação
                                addr_out <= addr_base_wr +(320*count_y_new+1) + count_x_new; //acessa o ultimo endereço escrito +1
                                wr_enable <= 1'b1; //habilita a escrita
                                state <= WAIT_WR_RD; //vai pro estado de aguardar a escrita acontecer

                                if (count_x_new == 10'd319) begin //caso tenha chegado na borda da imagem incrementa uma linha no x e desce para a fileira de baixo
                                    count_y_new <= count_y_new + 2'd2;
                                    count_x_new <= 10'd0;
                                end
                            end
                        endcase
                    end
                    last_op <= PR_ALG;
                end
                NHI_ALG: begin
                    done <= 1'b0;
                    if (!has_alg_on_exec) begin // se não tem um algoritmo em execução = primeira execução
                        num_steps_needed <= 19'd153600; //configura o numero de passos necessarios
                        
                        state <= WAIT_WR_RD;
                        operation_step_counter <= 3'b001;
                        count_x_old <= 10'd80; //configura o range que será lido da imagem anterior
                        count_y_old <= 10'd60; //configura o range que será lido da imagem anterior
                        count_x_new <= 10'd00; //configura os valores da nova imagem para o começo
                        count_y_new <= 10'd00; //configura os valores da nova imagem para o começo
                        if (current_zoom == 3'b101) begin
                            addr_base_rd <= 18'd96000;
                            addr_base_wr <= 18'd153600;
                        end else begin
                            addr_base_rd <= 18'd19200;
                            addr_base_wr <= 18'd76800;
                        end
                    end
                    else if (operation_step_counter == 3'b001) begin
                        algorithm_step_counter <= algorithm_step_counter + 1'b1; //incrementa o contador de passos
                        operation_step_counter <= 3'b000; //reinicia pro passo 1
                        addr_out <= addr_base_wr + (count_x_new) + ((count_y_new)*320); // endereço onde será escrito
                        wr_enable <= 1'b1; //habilita o sinal de escrita
                        state <= WAIT_WR_RD; //vai para o estado de aguardar o fim da operação de escrita
                        if (count_x_new == 10'd319) begin //se estiver na borda da imagem
                            count_y_new <= count_y_new + 1'b1; //incrementa uma coluna
                            count_x_new <= 10'b0; //volta pro começo da imagem
                        end else begin
                            count_x_new <= count_x_new + 1'b1;
                        end
                    
                    end else if (operation_step_counter == 3'b000) begin
                        operation_step_counter <= 3'b001; //incrementa um passo
                        addr_out <= addr_base_rd + ((count_x_new)>>1) + ((count_y_new*320)>>1);  //calcula o endereço do pixel a ser lido
                        state <= WAIT_WR_RD; //vai apra o estado de leitura
                    end
                    last_op <= NHI_ALG; //guarda "endereço" da ultima operação
                end

                NH_ALG: begin
                    done <= 1'b0;
                    if (!has_alg_on_exec) begin // se não tem um algoritmo em execução = primeira execução
                        num_steps_needed <= 19'd38400; 
                        
                        state <= WAIT_WR_RD;
                        operation_step_counter <= 3'b001;
                        count_x_old <= 10'd00; //configura o range que será lido da imagem anterior
                        count_y_old <= 10'd00; //configura o range que será lido da imagem anterior
                        count_x_new <= (current_zoom != 3'b010) ? 10'd80:10'd120; //configura os valores da nova imagem para o começo
                        count_y_new <= (current_zoom != 3'b010) ? 10'd60:10'd90; //configura os valores da nova imagem para o começo
                        if (current_zoom == 3'b010) begin
                            addr_base_rd <= 18'd0;
                            addr_base_wr <= 18'd182400;
                        end else begin
                            addr_base_rd <= 18'd0;
                            addr_base_wr <= 18'd96000;
                        end
                    end
                    else if (operation_step_counter == 3'b001) begin
                        algorithm_step_counter <= algorithm_step_counter + 1'b1; //incrementa o contador de passos
                        operation_step_counter <= 3'b000; //reinicia pro passo 1
                        addr_out <= addr_base_wr + (count_x_new) + ((count_y_new)*320); // endereço onde será escrito
                        wr_enable <= 1'b1; //habilita o sinal de escrita
                        state <= WAIT_WR_RD; //vai para o estado de aguardar o fim da operação de escrita
                        if (count_x_new == 10'd239) begin //se estiver na borda da imagem
                            count_y_new <= count_y_new + 1'b1; //incrementa uma coluna
                            count_x_new <= (current_zoom != 3'b010) ? 10'd80:10'd120; //volta pro começo da imagem
                            count_y_old <= count_y_old + (current_zoom != 3'b010) ? 10'd2:10'd4;
                            count_x_old <= 10'd0;
                        end else begin
                            count_x_new <= count_x_new + 1'b1;
                            count_x_old <= count_x_old + (current_zoom != 3'b010) ? 10'd2:10'd4;
                        end
                    
                    end else if (operation_step_counter == 3'b000) begin
                        operation_step_counter <= 3'b001; //incrementa um passo
                        addr_out <= addr_base_rd + (count_x_old) + ((count_y_old*320));  //calcula o endereço do pixel a ser lido
                        state <= WAIT_WR_RD; //vai apra o estado de leitura
                    end
                    last_op <= NH_ALG;
                end

                BA_ALG: begin 
                    done <= 1'b0;
                    if (!has_alg_on_exec) begin // se não tem um algoritmo em execução = primeira execução
                        num_steps_needed <= (current_zoom != 3'b010) ? 19'd30000:19'd95400;
                        state <= WAIT_WR_RD;
                        operation_step_counter <= 3'b001;
                        count_x_old <= (current_zoom != 3'b010) ? 10'd00:10'd80; //configura o range que será lido da imagem anterior
                        count_y_old <= (current_zoom != 3'b010) ? 10'd00:10'd60; //configura o range que será lido da imagem anterior
                        count_x_new <= (current_zoom != 3'b010) ? 10'd80:10'd120; //configura os valores da nova imagem para o começo
                        count_y_new <= (current_zoom != 3'b010) ? 10'd60:10'd90; //configura os valores da nova imagem para o começo
                        if (current_zoom == 3'b010) begin
                            addr_base_rd <= 18'd121600;
                            addr_base_wr <= 18'd182400;
                        end else begin
                            addr_base_rd <= 18'd0;
                            addr_base_wr <= 18'd96000;
                        end
                    end else begin
                        case(operation_step_counter)
                            3'b000: begin //realizar a primeira leitura
                                operation_step_counter <= operation_step_counter + 1'b1;
                                if ((count_x_new == 10'd199 && current_zoom != 3'b010) || (count_x_new == 10'd319 && current_zoom == 3'b010)) begin
                                    count_y_old <= count_y_old + 1'b1; //incrementa uma linha
                                    count_x_old <= (current_zoom != 3'b010) ? 10'd00:10'd80;             // volta pra primeira coluna
                                    addr_out <= addr_base_rd + (count_y_old*320) + ((current_zoom != 3'b010) ? 10'd00:10'd80); //leitura na imagem antiga
                                    //TODO: Verificar se o bloco acima funciona
                                end else begin
                                    count_x_old <= count_x_old + 1'b1;
                                    addr_out <= addr_base_rd + (count_y_old*320) + count_x_old; //leitura do endereço na imagem antiga
                                end
                                state <= WAIT_WR_RD;
                                
                            end
                            3'b001: begin //realizar a segunda leitura
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_rd +(320*count_y_old) + count_x_old+1; 
                                state <= WAIT_WR_RD;
                            end
                            3'b010: begin //realizar a terceira leitura
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_rd +(320*(count_y_old+1)) + count_x_old; 
                                state <= WAIT_WR_RD;
                            end
                            3'b011: begin //realizar a quarta leitura
                                operation_step_counter <= operation_step_counter + 1'b1; //incrementa o contador da operação
                                addr_out <= addr_base_rd +(320*count_y_old+1) + count_x_old+1; 
                                state <= WAIT_WR_RD;
                                count_x_old <= count_x_old + 2'd2; //incrementa a coluna
                            end
                            3'b100: begin //realizar a escrita
                                algorithm_step_counter <= algorithm_step_counter + 1'b1; // incrementa um a contagem de passos
                                operation_step_counter <= 3'b000; //reseta o contador da operação
                                addr_out <=addr_base_wr +(320*count_y_new) + count_x_new; //acessa o ultimo endereço escrito +1
                                wr_enable <= 1'b1; //habilita a escrita
                                state <= WAIT_WR_RD; //vai pro estado de aguardar a escrita acontecer

                                if ((count_x_new == 10'd199 && current_zoom != 3'b010) || (count_x_new == 10'd319 && current_zoom == 3'b010)) begin //caso tenha chegado na borda da imagem incrementa uma linha no x e desce para a fileira de baixo
                                    count_y_new <= count_y_new + 1'b1;
                                    count_x_new <= (current_zoom != 3'b010) ? 10'd60:10'd90;
                                end
                            end
                        endcase
                    end
                    last_op <= BA_ALG;
                end

                default: begin
                    addr_out <= addr_base;
                    done <= 1'b1;
                    state <= IDLE;
                end
            endcase
    end


endmodule