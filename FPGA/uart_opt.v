module uart_tx (
    input            sys_clk, sys_reset,      //system variables, high level reset
    input            uart_start,              //user control the start signal
    input            in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12, in13, in14, in15, in16, in17, in18,        //input 18 signals
    output reg       uart_done,               //prompt PC to end receiving data
    output reg       uart_txd,                //output by bits
    output reg       tx_state
);
    reg[17:0] tx_data;                                   //buffer
    reg[4:0]  bit_cnt;
    reg[9:0]  clk_cnt;
always@(posedge sys_clk or posedge sys_reset) begin      //registor : when staring, regist in 1~18
    if(sys_reset)begin
      tx_data <= 18'b 000000000000000000;
    end
    else if(uart_start)begin
      tx_data[0]  <= in1;
      tx_data[1]  <= in2;
      tx_data[2]  <= in3;
      tx_data[3]  <= in4;
      tx_data[4]  <= in5;
      tx_data[5]  <= in6;
      tx_data[6]  <= in7;
      tx_data[7]  <= in8;
      tx_data[8]  <= in9;
      tx_data[9]  <= in10;
      tx_data[10] <= in11;
      tx_data[11] <= in12;
      tx_data[12] <= in13;
      tx_data[13] <= in14;
      tx_data[14] <= in15;
      tx_data[15] <= in16;
      tx_data[16] <= in17;
      tx_data[17] <= in18;
    end
    else begin
      tx_data <= tx_data;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //sending controller
    if(sys_reset)begin
      tx_state <= 1'b 0;
    end
    else if(uart_start)begin
      tx_state <= 1'b 1;
    end
    else if((bit_cnt == 5'b 10011) && (clk_cnt == 10'b 1000111111))begin
      tx_state <= 1'b 0;
    end
    else begin
      tx_state <= tx_state;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //clock count and bit count
    if(sys_reset)begin
      bit_cnt <= 5'b 00000;
      clk_cnt <= 10'b 0000000000;
    end
    else if(tx_state)begin                               //transporting
      if(clk_cnt < 10'b 1000111111)begin                 //Baut Rate 19200
        clk_cnt <= clk_cnt + 10'b 0000000001;
        bit_cnt <= bit_cnt;
      end
      else begin                                         // 1/19200, 1 bit is sent
        clk_cnt <= 10'b 0000000000;
        bit_cnt <= bit_cnt + 5'b 00001;
      end
    end
    else begin
      bit_cnt <= 5'b 00000;
      clk_cnt <= 10'b 0000000000;
    end
end
always@(posedge sys_clk or posedge sys_reset) begin      //sending program
    if(sys_reset)begin
      uart_txd <= 1'b 1;
    end
    else if(tx_state)begin
      if(clk_cnt == 10'b 0000000001)begin
        case(bit_cnt)
          5'b 00000: uart_txd <= 1'b 0;                  //start bit 1'b0
          5'b 00001: uart_txd <= tx_data[0];
          5'b 00010: uart_txd <= tx_data[1];
          5'b 00011: uart_txd <= tx_data[2];
          5'b 00100: uart_txd <= tx_data[3];
          5'b 00101: uart_txd <= tx_data[4];
          5'b 00110: uart_txd <= tx_data[5];
          5'b 00111: uart_txd <= tx_data[6];
          5'b 01000: uart_txd <= tx_data[7];
          5'b 01001: uart_txd <= tx_data[8];
          5'b 01010: uart_txd <= tx_data[9];
          5'b 01011: uart_txd <= tx_data[10];
          5'b 01100: uart_txd <= tx_data[11];
          5'b 01101: uart_txd <= tx_data[12];
          5'b 01110: uart_txd <= tx_data[13];
          5'b 01111: uart_txd <= tx_data[14];
          5'b 10000: uart_txd <= tx_data[15];
          5'b 10001: uart_txd <= tx_data[16];
          5'b 10010: uart_txd <= tx_data[17];
          5'b 10011: uart_txd <= 1'b 1;                  //end bit 1'b1
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
    else if((bit_cnt == 5'b 10011) && (clk_cnt == 10'b 1000111111))begin
      uart_done <= 1'b 1;
    end
    else begin
      uart_done <= 1'b 0;
    end
end
endmodule