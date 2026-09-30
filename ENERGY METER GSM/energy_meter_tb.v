`timescale 1ns/1ps
// Small window (1000 clocks) so the simulation is short.
// Pulse every 100 clocks -> exactly 10 pulses per window.
module energy_meter_tb;
    reg clk = 0, rst = 1, cf = 0;
    wire [31:0] pulse_total, power_w;
    wire [47:0] energy_uwh;
    wire [15:0] last_win, avg8;
    wire        window_done;

    energy_meter_dsp #(.PULSES_PER_KWH(3200), .WINDOW_CLKS(1000)) dut (
        .clk(clk), .rst(rst), .cf_in(cf),
        .pulse_total(pulse_total), .energy_uwh(energy_uwh),
        .pulses_last_window(last_win), .pulses_avg8(avg8),
        .power_w(power_w), .window_done(window_done));

    always #5 clk = ~clk;

    integer k;
    initial begin
        $dumpfile("meter.vcd"); $dumpvars(0, energy_meter_tb);
        #50 rst = 0;
        for (k = 0; k < 1300; k = k + 1) begin      // 13 windows
            cf = 1; repeat (5)  @(posedge clk);
            cf = 0; repeat (95) @(posedge clk);
        end
        $display("pulse_total=%0d energy_uWh=%0d last_window=%0d avg8=%0d power_w=%0d",
                 pulse_total, energy_uwh, last_win, avg8, power_w);
        if (last_win == 10 && avg8 == 10) $display("PASS: 10 pulses/window, average = 10");
        else                              $display("CHECK: unexpected window count");
        $finish;
    end
endmodule
