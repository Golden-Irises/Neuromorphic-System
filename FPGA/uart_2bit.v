module uart_2bit_tx (
    input            sys_clk, sys_reset,       //system variables, high level reset
    input            uart_start,               //user control the start signal
    input            in1, in2,                 //input 2 signals
    output reg       uart_txd,                 //output by bits
    output reg       tx_state, tx_enable
);
    reg       uart_done;                                 //prompt PC to end receiving data
    //reg       tx_state;
    //reg       tx_enable;
    reg[1:0]  tx_data;                                   //buffer
    reg[2:0]  bit_cnt;
    reg[9:0]  clk_cnt_1;
    reg[9:0]  clk_cnt_2;
    reg[2:0]  period_cnt;                                //when uart_start is high level, sending data by this period(32 * BIT_TIME)
always@(posedge sys_clk or posedge sys_reset) begin      //registor : when staring, regist in 1~2
    if(sys_reset)begin
      tx_data <= 2'b 00;
    end
    else if(tx_enable)begin
      tx_data[0]  <= in1;
      tx_data[1]  <= in2;
    end
    else begin
      tx_data <= tx_data;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //period conut & continuous sending
    if(sys_reset)begin
      clk_cnt_2  <= 10'b 0000000000;
      period_cnt <= 3'b 000;
      tx_enable  <= 1'b 0;
    end
    else if(uart_start == 1'b 1)begin
      if(clk_cnt_2 < 10'b 1000111111)begin
        clk_cnt_2 <= clk_cnt_2 + 10'b 0000000001;
        period_cnt <= period_cnt;
      end
      else begin
        if(period_cnt == 3'b 000)begin
          tx_enable <= 1'b 1;
          clk_cnt_2 <= 10'b 0000000000;
          period_cnt <= period_cnt + 3'b 001;
        end
        else if(period_cnt == 3'b 001)begin
          tx_enable <= 1'b 0;
          clk_cnt_2 <= 10'b 0000000000;
          period_cnt <= period_cnt + 3'b 001;
        end
        else begin
          clk_cnt_2 <= 10'b 0000000000;
          period_cnt <= period_cnt + 3'b 001;
        end
      end
    end
    else begin
      tx_enable <= 1'b 0;
      clk_cnt_2 <= 10'b 0000000000;
      period_cnt <= 3'b 000;
      tx_state <= 1'b 0;
      uart_txd <= 1'b 1;
      tx_data <= 2'b 00;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //sending controller
    if(sys_reset)begin
      tx_state <= 1'b 0;
    end
    else if(tx_enable)begin
      tx_state <= 1'b 1;
    end
    else if((bit_cnt == 3'b 011) && (clk_cnt_1 == 10'b 1000111111))begin
      tx_state <= 1'b 0;
    end
    else begin
      tx_state <= tx_state;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //clock count and bit count
    if(sys_reset)begin
      bit_cnt <= 3'b 000;
      clk_cnt_1 <= 10'b 0000000000;
    end
    else if(tx_state)begin                               //transporting
      if(clk_cnt_1 < 10'b 1000111111)begin               //Baut Rate 19200
        clk_cnt_1 <= clk_cnt_1 + 10'b 0000000001;
        bit_cnt <= bit_cnt;
      end
      else begin                                         // 1/19200, 1 bit is sent
        clk_cnt_1 <= 10'b 0000000000;
        bit_cnt <= bit_cnt + 3'b 001;
      end
    end
    else begin
      bit_cnt <= 3'b 000;
      clk_cnt_1 <= 10'b 0000000000;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //sending program
    if(sys_reset)begin
      uart_txd <= 1'b 1;
    end
    else if(tx_state)begin
      if(clk_cnt_1 == 10'b 0000000001)begin
        case(bit_cnt)
          3'b 000: uart_txd <= 1'b 0;                  //start bit 1'b0
          3'b 001: uart_txd <= tx_data[0];
          3'b 010: uart_txd <= tx_data[1];
          3'b 011: uart_txd <= 1'b 1;                  //end bit 1'b1
          default: uart_txd <= 1'b 0; //ERROR
        endcase
      end
      else begin
        uart_txd <= uart_txd;
      end
    end
    else begin
      uart_txd <= uart_txd;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //prompting a batch of data has done
    if(sys_reset)begin
      uart_done <= 1'b 0;
    end
    else if((bit_cnt == 3'b 011) && (clk_cnt_1 == 10'b 1000111111))begin
      uart_done <= 1'b 1;
    end
    else begin
      uart_done <= 1'b 0;
    end
end
endmodule
