# HydraFW — Port for STM32F401 / STM32F411 (BlackPill) & Web Control Panel

> **Status: Beta v1** — This is an early beta port with active ongoing development. There may be bugs, quirks, or edge-case issues across certain modes and protocols. Bug reports, testing, and pull requests are welcome!

This project is a port of **HydraFW** for the **STM32F401** (and STM32F411) **BlackPill** board (LQFP48). It features remapped GPIO pins, clock and timing adjustments for STM32F401 (84 MHz), bug fixes, and an integrated in-browser Web Control Panel.

---

## Features

- **In-Browser Web Panel**: Full GUI control via Web Serial / WebUSB in Chrome and Edge (`webpanel/index.html`).
- **Memory Programmer**:
  - Read, write, erase, and dump **SPI Flash** (Winbond W25Qxx, Macronix, BIOS chips).
  - Direct support for bottom SOIC-8 footprint on BlackPill (SPI3 PA4..PA7).
  - Read and write **I2C EEPROM** (24C01 .. 24M01).
- **Supported Protocols & Modes**:
  - **SPI**: 3 buses, speeds up to 42 MHz, master/slave, sniffer.
  - **I2C**: 3 buses, 50k to 1M speeds, auto address scanner (`scan`), sniffer.
  - **UART / LIN**: 3 ports, baudrates up to 921600, transparent bridge mode, 115200 8E1 STM32 flasher preset, LIN synch break.
  - **1-Wire**: Search ROMs (`scan`), DS18B20 temperature read, iButton key reader (`read-rom`), WCH CH32V SWIO debug.
  - **2-Wire**: RawWire, ARM Cortex SWD `idcode` reading, automatic SWD pinout brute-force.
  - **3-Wire**: Microwire 93Cxx EEPROM operations.
  - **JTAG**: Boundary scan, chip IDCODE reading, chain length detection (`scan bypass`), pinout brute-force (`brute`), OpenOCD bridge.
  - **Wiegand RFID**: Access control card reader (Wiegand 26/34) and card emulator/transmitter.
  - **Logic Analyzer (SUMP)**: 1, 2, 4, 8, or 16 channels, up to 21 MS/s with Sigrok / PulseView integration.
  - **PWM**: Hardware generator on PA8 from 1 Hz to 42 MHz (0..100% duty cycle).
  - **Frequency Counter**: Hardware frequency and duty cycle meter on PB4 (TIM3_CH1).
  - **ADC**: Analog voltage measurements (0..3.3V on PA1, PB0, PB1) and internal sensors (Temp, Vref, Vbat).
  - **Continuity Tester**: Continuity test between PB8 and PB9 with LED feedback.
  - **SD Card**: FAT32 file system operations (`sd mount`, `sd ls`, `sd cat`, `sd format`).
  - **Debug**: ARM Cortex-M4 register dump, DWT CPU cycle counter, memory inspection.

---

## Pinout Map (BlackPill)

| Pin | Primary Function | Notes |
|---|---|---|
| **PA0** | `UBTN / KEY` | Hardware button on board. Used to exit SUMP and UART Bridge. |
| **PA1** | `ADC1` | Analog Input (0..3.3V) |
| **PA2 / PA3** | `UART2 TX / RX` | Also `LIN2` |
| **PA4..PA7** | `SPI3 (CS, SCK, MISO, MOSI)` | Connected to bottom SOIC-8 Flash footprint |
| **PA8** | `PWM1` / `I2C3 SCL` | PWM output (1 Hz .. 42 MHz) |
| **PA9 / PA10** | `UART1 TX / RX` | Also `LIN1` (USART1) |
| **PA11 / PA12** | `USB D- / D+` | Native USB-C HydraFW Console |
| **PA15, PB3..PB5**| `SPI1 (CS, SCK, MISO, MOSI)`| External SPI header pins |
| **PB0 / PB1** | `ADC2 / ADC3` | Analog inputs, also SUMP D0/D1 |
| **PB3 / PB4** | `2-Wire (CLK, SDA)` | Also ARM SWD |
| **PB3..PB5** | `3-Wire (CLK, SDI, SDO)` | Microwire 93Cxx |
| **PB4** | `Frequency (FREQ1)` | TIM3_CH1 frequency counter |
| **PB6 / PB7** | `I2C1 (SCL, SDA)` | Primary external I2C bus |
| **PB7..PB11** | `JTAG (TRST, TDI, TDO, TMS, TCK)` | JTAG Boundary Scan |
| **PB8 / PB9** | `Wiegand (D0, D1)` | Also Continuity Tester P1 / P2 |
| **PB10 / PB3** | `I2C2 (SCL, SDA)` | Secondary I2C bus |
| **PB11** | `1-Wire / SWIO` | Dallas 1-Wire, iButton, DS18B20 |
| **PB12..PB15** | `SPI2 (CS, SCK, MISO, MOSI)` | Secondary external SPI bus |
| **PB0..PB15** | `SUMP (D0..D15)` | Logic analyzer channels |
| **PC6 / PC7** | `UART3 TX / RX` | USART6 |
| **PC13** | `ULED` | On-board Blue LED (active low) |

---

## How to Build

Requirements: `arm-none-eabi-gcc` and `make`.

```bash
cd src
make -j4
```

Build outputs:
- `src/build/hydrafw.bin`
- `src/build/hydrafw.hex`
- `src/build/hydrafw.elf`

---

## How to Flash

### Method 1: WebUSB DFU (Browser)
1. Open `webpanel/start.html` in Chrome or Edge.
2. Hold down the **BOOT** button on the BlackPill board (BOOT0 = 1).
3. Press and release **RESET**, then release **BOOT** (enters DFU bootloader).
4. Click **"Flash via DFU"** &mdash; it will automatically download the latest build from [GitHub `build/hydrafw.bin`](https://github.com/Eternal-Entropia/hydrafw-port-stm32f401/blob/main/build/hydrafw.bin) and flash it to the board.
5. Press **RESET** on the board.

### Method 2: ST-Link / OpenOCD / STM32CubeProgrammer
```bash
STM32_Programmer_CLI -c port=SWD -w src/build/hydrafw.bin 0x08000000 -v -rst
```

---

## How to Use Web Panel

1. Connect the BlackPill board to your PC using a USB-C cable.
2. Open [`webpanel/index.html`](webpanel/index.html) in Chrome or Edge.
3. Click **"Connect"** in the top bar and select your board.
4. Use the tabs on the left to read/flash chips, control GPIO, generate PWM, capture logic signals, or enter commands in the terminal.

---

## License

GPL v3 & Apache 2.0 (see original HydraFW headers).
