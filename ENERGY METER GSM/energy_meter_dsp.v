// Digital signal-processing block for the smart energy meter (VLSI logic).
// Input : cf_in  - pulse train from the energy metering IC (1 pulse = fixed energy)
// Output: pulse count, energy (uWh), per-window pulse rate, 8-window moving average,
//         and average power in watts.
// Assumes WINDOW_CLKS = clock cycles in exactly 1 second (power output is valid for 1 s windows).
module energy_meter_dsp #(
    parameter PULSES_PER_KWH = 3200,
    parameter WINDOW_CLKS    = 50_000_000            // 1 s at 50 MHz
)(
    input  wire        clk,
    input  wire        rst,
    input  wire        cf_in,
    output reg  [31:0] pulse_total,
    output reg  [47:0] energy_uwh,                   // cumulative energy in micro-Wh
    output reg  [15:0] pulses_last_window,
    output reg  [15:0] pulses_avg8,                  // moving average over 8 windows
    output reg  [31:0] power_w,                      // average power in watts
    output reg         window_done
);
    localparam UWH_PER_PULSE = 1_000_000_000 / PULSES_PER_KWH;   // 1 kWh = 1e9 uWh
    localparam W_PER_PPS     = 3_600_000 / PULSES_PER_KWH;       // watts per (pulse/second)

    // 1) synchronise the asynchronous pulse and detect its rising edge
    reg s1, s2, s3;
    wire rising = s2 & ~s3;
    always @(posedge clk) begin
        if (rst) begin s1 <= 0; s2 <= 0; s3 <= 0; end
        else     begin s1 <= cf_in; s2 <= s1; s3 <= s2; end
    end

    // 2) counting, windowing and moving-average filter
    reg [31:0] wcnt;
    reg [15:0] wpulses;
    reg [15:0] hist [0:7];
    reg [18:0] sum8;
    reg [15:0] wp_next;
    integer i;

    always @(posedge clk) begin
        if (rst) begin
            pulse_total <= 0; energy_uwh <= 0; pulses_last_window <= 0;
            pulses_avg8 <= 0; power_w <= 0; window_done <= 0;
            wcnt <= 0; wpulses <= 0; sum8 <= 0;
            for (i = 0; i < 8; i = i + 1) hist[i] <= 0;
        end else begin
            window_done <= 0;
            wp_next = wpulses + (rising ? 16'd1 : 16'd0);

            if (rising) begin
                pulse_total <= pulse_total + 1;
                energy_uwh  <= energy_uwh + UWH_PER_PULSE;
            end

            if (wcnt == WINDOW_CLKS - 1) begin       // end of a measurement window
                wcnt <= 0;
                wpulses <= 0;
                window_done <= 1;
                pulses_last_window <= wp_next;
                // moving average: add newest, drop oldest
                sum8 <= sum8 + wp_next - hist[7];
                for (i = 7; i > 0; i = i - 1) hist[i] <= hist[i-1];
                hist[0] <= wp_next;
                pulses_avg8 <= (sum8 + wp_next - hist[7]) >> 3;
                power_w     <= ((sum8 + wp_next - hist[7]) >> 3) * W_PER_PPS;
            end else begin
                wcnt <= wcnt + 1;
                wpulses <= wp_next;
            end
        end
    end
endmodule
