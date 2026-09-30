# Smart-Energy-Meter-GSM
Smart energy meter interface: meter-IC pulse counting, GSM SMS reporting, slab billing and a Verilog DSP block. Simulated in Wokwi and EDA Playground.
# Smart Energy Meter Interface with GSM

## Overview
This project reads energy pulses from an energy metering IC (CF output) using an
Arduino, calculates energy usage, average power and a slab-based bill, and sends
the result to the user by SMS through a GSM module (SIM800L). A Verilog module
adds digital signal processing in hardware: it filters the pulse input,
accumulates energy and computes a moving-average power.

## Features
- Energy metering IC interfaced to the microcontroller through an interrupt pin
- Usage, power and bill sent by SMS using AT commands
- Slab tariff billing simulation
- Verilog DSP block: synchroniser, edge detector, energy accumulator, 8-window
  moving-average filter, power calculation
- Testbench with self-check

## Files
- energy_meter_gsm.ino - Arduino firmware
- energy_meter_dsp.v   - Verilog DSP block
- energy_meter_tb.v    - Verilog testbench
- screenshots/         - simulation results

## How it works
1. The metering IC outputs one pulse for each fixed amount of energy (3200 pulses = 1 kWh).
2. The Arduino counts pulses with an interrupt and, every report interval,
   calculates energy (kWh), average power (W) and bill (Rs).
3. The message is sent through the GSM module. In simulation mode it is printed
   on the Serial Monitor instead.
4. The Verilog block performs the same counting and averaging in hardware logic.

## Simulation results
- Wokwi (Arduino): simulated load 1500 W, measured about 1463 W; 1.219 kWh gave a bill of Rs 3.66.
- EDA Playground (Verilog): 10 pulses per window, 8-window average settles at 10.
  The test passes.

## Tools used
Arduino C++ (Wokwi), Verilog (Icarus Verilog, EPWave on EDA Playground).

## Hardware connections (real hardware)
Meter IC CF -> Arduino D2 | SIM800L TX -> D7 | SIM800L RX -> D8 (via divider)

## Note
Set SIMULATE to 0 in the .ino file and enter your phone number to use real hardware.
