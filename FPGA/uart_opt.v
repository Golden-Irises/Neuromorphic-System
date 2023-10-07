module uart_tx (
    input            sys_clk, sys_reset,      //system variables, high level reset
    input            uart_start,              //user control the start signal
    input            in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12, in13, in14, in15, in16, in17, in18,        //input 18 signals
    output reg       uart_txd                 //output by bits
);

reg        tx_state;
reg        tx_enable;
reg[17:0]  data_origin;                                  //Original data buffer
reg[5:0]   tx_data;                                      //Transport data buffer
reg[2:0]   bit_cnt;
reg[15:0]  clk_cnt_1;
reg[15:0]  clk_cnt_2;
reg[4:0]   period_cnt;                                   //when uart_start is high level, sending data by this period(32 * BIT_TIME)

always@(posedge sys_clk or negedge sys_reset) begin      //registor : when staring, regist in 1~18
    if(!sys_reset)begin
      data_origin <= 18'b 000000000000000000;
    end
    else if(tx_enable)begin
      data_origin[0]  <= in1;
      data_origin[1]  <= in2;
      data_origin[2]  <= in3;
      data_origin[3]  <= in4;
      data_origin[4]  <= in5;
      data_origin[5]  <= in6;
      data_origin[6]  <= in7;
      data_origin[7]  <= in8;
      data_origin[8]  <= in9;
      data_origin[9]  <= in10;
      data_origin[10] <= in11;
      data_origin[11] <= in12;
      data_origin[12] <= in13;
      data_origin[13] <= in14;
      data_origin[14] <= in15;
      data_origin[15] <= in16;
      data_origin[16] <= in17;
      data_origin[17] <= in18;
    end
    else begin
      data_origin <= data_origin;
    end
end
always@(posedge sys_clk or negedge sys_reset) begin      //period conut : 3 * 6bits + 8 idle bits
    if(!sys_reset)begin
      period_cnt <= 5'b 00000;
      tx_enable  <= 1'b 0;
      clk_cnt_2  <= 16'b 0000000000000000;
    end
    else if(uart_start == 1'b 1)begin
      if(clk_cnt_2 < 16'b 101000101100000)begin
        if(period_cnt == 5'b 00000)begin
          tx_enable <= 1'b 1;
          period_cnt <= period_cnt;
          clk_cnt_2  <= clk_cnt_2 + 16'b 0000000000000001;
        end
        else if(period_cnt == 5'b 00001)begin
          tx_enable <= 1'b 0;
          clk_cnt_2  <= clk_cnt_2 + 16'b 0000000000000001;
          period_cnt <= period_cnt;
        end
        else begin
          clk_cnt_2  <= clk_cnt_2 + 16'b 0000000000000001;
          period_cnt <= period_cnt;
        end
      end
      else begin
        clk_cnt_2  <= 16'b 0000000000000000;
        period_cnt <= period_cnt + 5'b 00001;
      end
    end
    else begin
      tx_enable  <= 1'b 0;
      clk_cnt_2  <= 16'b 0000000000000000;
      period_cnt <= 5'b 00000;
    end
end
always@(posedge sys_clk or negedge sys_reset) begin      //sending controller
    if(!sys_reset)begin
      tx_state <= 1'b 0;
    end
    else if(tx_enable)begin
      tx_state <= 1'b 1;
    end
    else if((period_cnt == 5'b 10111) && (bit_cnt == 3'b 111) && (clk_cnt_2 == 16'b 101000101100000))begin
      tx_state <= 1'b 0;
    end
    else begin
      tx_state <= tx_state;
    end
end
always@(posedge sys_clk or negedge sys_reset)begin       //data selector
  if(!sys_reset)begin
    tx_data <= 6'b 000000;
  end
  else if((period_cnt == 5'b 00000) && tx_state)begin
    tx_data <= data_origin[5:0];
  end
  else if((period_cnt == 5'b 01000) && tx_state)begin
    tx_data <= data_origin[11:6];
  end
  else if((period_cnt == 5'b 10000) && tx_state)begin
    tx_data <= data_origin[17:12];
  end
  else begin
    tx_data <= tx_data;
  end
end
always@(posedge sys_clk or negedge sys_reset) begin      //clock count and bit count
    if(!sys_reset)begin
      bit_cnt <= 3'b 000;
      clk_cnt_1 <= 16'b 0000000000000000;
    end
    else if(tx_state)begin                               //transporting
      if(clk_cnt_1 < 16'b 101000101100000)begin          //Baut Rate 2400
        clk_cnt_1 <= clk_cnt_1 + 16'b 0000000000000001;
        bit_cnt <= bit_cnt;
      end
      else begin                                         // 1/2400 second , 1 bit is sent
        clk_cnt_1 <= 16'b 0000000000000000;
        bit_cnt <= bit_cnt + 3'b 001;
      end
    end
    else begin
      bit_cnt <= 3'b 000;
      clk_cnt_1 <= 16'b 0000000000000000;
    end
end
always@(posedge sys_clk or negedge sys_reset) begin      //sending program
    if(!sys_reset)begin
      uart_txd <= 1'b 1;
    end
    else if(tx_state)begin
      if(clk_cnt_1 == 16'b 0000000000000001)begin
        case(bit_cnt)
          3'b 000: uart_txd <= 1'b 0;                    //start bit 1'b0
          3'b 001: uart_txd <= tx_data[0];
          3'b 010: uart_txd <= tx_data[1];
          3'b 011: uart_txd <= tx_data[2];
          3'b 100: uart_txd <= tx_data[3];
          3'b 101: uart_txd <= tx_data[4];
          3'b 110: uart_txd <= tx_data[5];
          3'b 111: uart_txd <= 1'b 1;                    //end bit 1'b1
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
endmodule