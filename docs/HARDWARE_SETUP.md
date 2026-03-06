# Hardware Setup Guide

This document describes how to connect the MFRC522 RFID reader to the ESP32 Cheap Yellow Display (CYD) and details the hardware modifications required to support battery vs. USB power detection.

## 1. MFRC522 RFID Connection

The MFRC522 communicates via SPI. To keep the screen functional, we connect the reader to the **VSPI** bus, which is typically routed to the SD card slot on the CYD. By sacrificing the SD card slot, we can repurpose its pins.

Please wire your MFRC522 module to the CYD as follows:

| MFRC522 Pin   | CYD Pin (Connector)   | Location Description          |
| :------------ | :-------------------- | :-----------------------------|
| `3.3V`        | `3.3V`                | 3.3V rail                     |
| `RST`         | `IO22`                | Available on P3/CN1 connector |
| `GND`         | `GND`                 | Ground                        |
| `MISO`        | `IO19`                | 7th pin of SD connector       |
| `MOSI`        | `IO23`                | 3rd pin of SD connector       |
| `SCK`         | `IO18`                | 5th pin of SD connector       |
| `SDA` (CS)    | `IO5`                 | 2nd pin of SD connector       |

*(Note: The SD card slot counts pins left-to-right when looking at the slot opening.)*

## 2. Power Detection & Battery Modification

Standard CYD boards do not have an onboard battery charging circuit or a dedicated way to detect if the board is powered via USB (5V) or an external battery.

To support the Power Management feature (which turns off the display on USB or enters deep sleep on battery after 60 seconds), you need to modify the board to allow the ESP32 to measure the incoming voltage.

### The LDR Mod
The easiest approach is to repurpose the Light Dependent Resistor (LDR) pin (`IO34`).
1. **Remove the LDR** component from the CYD board (usually located near the top edge).
2. **Wire a Voltage Divider** from the main power rail (V-In / 5V rail) to `IO34`:
   - Connect a `22kΩ` resistor from the main power rail to `IO34`.
   - Connect a `10kΩ` resistor from `IO34` to GND.
3. **How it works:** 
   - When powered by **USB (5V)**, the divider presents roughly `1.56V` to `IO34`.
   - When powered by a **Lithium Battery (3.7V - 4.2V)**, the voltage drops to roughly `1.15V - 1.3V`.
   - The `PowerManager` reads the analog value on `IO34`. If it's above a certain threshold (calibrated for USB), it assumes USB power. If lower, it assumes battery.

### Waking from Deep Sleep
When running on battery power, the device will eventually enter **Deep Sleep** to conserve energy.
- **To wake up:** Press the physical `RST` (Reset) button located on the back or side of the CYD. This hardware interrupts the ESP32 and restarts the boot cycle, immediately jumping back to the Scanning interface.
