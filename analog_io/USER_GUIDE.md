# Analog IO - User Guide

**Firmware Platform:** analog_io (STM32G030K8)
**Document Revision:** 1.0
**Last Updated:** 2026-04-15

---

## Table of Contents

1. [Product Overview](#1-product-overview)
2. [Hardware Description](#2-hardware-description)
3. [Getting Started](#3-getting-started)
4. [Firmware Runtime Architecture](#4-firmware-runtime-architecture)
5. [Modbus RTU Integration](#5-modbus-rtu-integration)
6. [Register Map Reference](#6-register-map-reference)
7. [AI/AO Data Processing and Calibration](#7-aiao-data-processing-and-calibration)
8. [Factory Default Settings](#8-factory-default-settings)
9. [Troubleshooting](#9-troubleshooting)
10. [Tester Test Procedure](#10-tester-test-procedure)
11. [Quick Validation Checklist](#11-quick-validation-checklist)

---

## 1. Product Overview

The `analog_io` firmware is an embedded application for STM32G030K8 that operates as a **Modbus RTU slave** over RS-485 to expose and control analog I/O.

The current firmware focuses on two main functional blocks:

- **Analog Input (AI):** 4 channels read from ADS1115 over I2C2
- **Analog Output (AO):** 4 channels driven by MCP4728 over I2C2

Modbus data is organized as follows:

- **Input Registers (FC04):** AI raw/scaled data and internal debug registers
- **Holding Registers (FC03/FC06/FC10):** AO setpoint/mode and device information

### Key Capabilities

| Feature | Details |
|---|---|
| Protocol | Modbus RTU slave (RS-485) |
| Supported function codes | FC03, FC04, FC06, FC10 |
| AI channels | 4 channels (ADS1115) |
| AO channels | 4 channels (MCP4728) |
| AI public contract | RAW 0..4095, SCALED 0..10000 mV |
| AO public contract | SETPOINT 0..4095, MODE 0/1 |
| Device ID range | 1..7 (stored in Flash) |
| Debug UART | USART2 @ 115200 8N1 |

---

## 2. Hardware Description

### 2.1 MCU and Core Platform

- MCU: STM32G030K8
- Clock source: HSI
- Firmware model: polling loop (no RTOS)

### 2.2 RS-485 Port (Modbus RTU)

- UART: USART1
- RS-485 direction controlled by `RS485_EN` (GPIOC Pin 6)
- Default serial settings:
  - Baudrate: 9600
  - Data bits: 8
  - Parity: None
  - Stop bits: 1

> Note: Modbus serial parameters are compile-time settings in the current firmware.

### 2.3 Debug Serial Port

- Debug UART: USART2
- Baudrate: 115200, 8N1
- Purpose: boot logs, runtime status, I2C scan information, and fault messages

### 2.4 I2C Devices

I2C2 is scanned during startup, from address `0x08` to `0x77`.

Target address windows:

- ADS1115: `0x48..0x4B`
- MCP4728: `0x60..0x67`

### 2.5 LED and Watchdog

- Heartbeat LED: GPIOA Pin 8
- Default blink period: 500 ms
- IWDG watchdog support exists, but is disabled by default (`APP_WATCHDOG_ENABLE = 0`)

---

## 3. Getting Started

### 3.1 Build Firmware

Using VS Code tasks (recommended):

1. Configure analog_io Debug
2. Build analog_io Debug
3. (Optional) Show analog_io Size

Using CMake command line:

Run these commands from the workspace root directory:

```bash
cmake -S analog_io -B analog_io/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=analog_io/toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build analog_io/build --parallel
```

Build artifacts:

- `analog_io/build/analog_io.elf`
- `analog_io/build/analog_io.hex`
- `analog_io/build/analog_io.bin`

### 3.2 Flash and Connect

1. Program the firmware file (`.elf` or `.bin`) to the board.
2. Connect RS-485 to your Modbus master.
3. Connect debug UART (USART2, 115200) to monitor boot logs.

### 3.3 Boot Log Expectations

A normal boot log includes:

- reset cause
- current Modbus address
- calibration source (`flash` or `defaults`)
- safe AO apply result
- I2C scan status and ADS1115/MCP4728 addresses

### 3.4 Basic Modbus Master Settings

Configure your master tool (Modbus Poll, QModMaster, pyserial script):

- Mode: RTU
- Baudrate: 9600
- 8N1
- Slave ID: value from `DEVICE_ID` register (default 1)

---

## 4. Firmware Runtime Architecture

### 4.1 Main Boot Sequence

Main initialization flow:

1. `HAL_Init()`
2. `bsp_init()`
3. `bsp_watchdog_init()`
4. reset/modbus/calibration log output
5. `analog_io_service_apply_safe_output_blocking()`
6. `modbus_service_init()`
7. `analog_io_service_init()`
8. enter infinite loop

### 4.2 Main Loop

In `while(1)`, firmware runs:

- `modbus_service_run_once()`
- `analog_io_service_run_once()`
- runtime status log (periodic)
- `bsp_heartbeat_process()`
- `bsp_watchdog_kick()`

### 4.3 Analog IO Service State Flow

`analog_io_service` defines three states:

- `IDLE`
- `SCANNING`
- `READY`

Current runtime transitions are `SCANNING -> READY`; `IDLE` is defined but not entered after service initialization in the current implementation.

In scanning state:

- Scan I2C one address at a time
- Initialize AI immediately if ADS1115 is found
- Initialize AO immediately if MCP4728 is found
- Complete bring-up after scanning reaches the max address

If devices are not found:

- AI/AO are still initialized with address `0` to mark fault state while keeping firmware alive and reportable through status/error registers.

### 4.4 Safe AO at Boot

Before full service startup, firmware performs:

1. Reset AO shadow state to safe defaults
2. Try blocking safe-state apply within timeout (default 100 ms)
3. Log result as `applied` or `failed`

Goal: prevent unsafe analog output levels during startup.

---

## 5. Modbus RTU Integration

### 5.1 Transport and Framing

- RS-485 half-duplex
- RX buffer: 64 bytes
- Frame gap timeout: 5 ms
- Max processed frame size: 64 bytes
- CRC16 Modbus polynomial: `0xA001`

### 5.2 Supported Function Codes

| FC | Name | Supported |
|---|---|---|
| 0x03 | Read Holding Registers | Yes |
| 0x04 | Read Input Registers | Yes |
| 0x06 | Write Single Register | Yes |
| 0x10 | Write Multiple Registers | Yes |

Not supported: FC01/FC02/FC05/FC0F and other function codes.

### 5.3 Slave Address Rules

- Firmware processes a frame only when the frame address matches current `slave_address`.
- Broadcast address `0` is not handled.
- Change address through `DEVICE_ID` register (`0x0008`) using FC06.
- New address is applied after 20 ms delay, then saved to Flash.
- Valid address range: `1..7`.

### 5.4 Exception Responses

| Exception Code | Meaning |
|---|---|
| 0x01 | Illegal function |
| 0x02 | Illegal address |
| 0x03 | Illegal value |
| 0x04 | Device failure |

### 5.5 Write Constraints

- FC10 is allowed only within AO holding register range (`0x0000..0x0007`).
- Max FC10 write count: 8 AO registers.
- AO setpoint must be `0..4095`.
- AO mode must be `0` (voltage) or `1` (current).

### 5.6 Minimal Request Examples

Read 4 input registers from address 0 (AI raw):

```text
Request PDU: 04 00 00 00 04
```

Write AO1 setpoint = 2048:

```text
Request PDU: 06 00 00 08 00
```

> Slave address and CRC are added for full RTU frame format.

---

## 6. Register Map Reference

All addresses below are firmware register addresses.

### 6.1 Input Registers (FC04) - Public

| Address | Symbol | Description | Range |
|---|---|---|---|
| 0x0000 | AI1_RAW | AI channel 1 raw | 0..4095 |
| 0x0001 | AI2_RAW | AI channel 2 raw | 0..4095 |
| 0x0002 | AI3_RAW | AI channel 3 raw | 0..4095 |
| 0x0003 | AI4_RAW | AI channel 4 raw | 0..4095 |
| 0x0004 | AI1_SCALED | AI1 scaled (mV) | 0..10000 |
| 0x0005 | AI2_SCALED | AI2 scaled (mV) | 0..10000 |
| 0x0006 | AI3_SCALED | AI3 scaled (mV) | 0..10000 |
| 0x0007 | AI4_SCALED | AI4 scaled (mV) | 0..10000 |

### 6.2 Input Registers (FC04) - Internal Debug

| Address | Symbol | Description |
|---|---|---|
| 0x0100 | AI1_BOARD_RAW | Board-domain raw after ADS1115 mapping |
| 0x0101 | AI2_BOARD_RAW | Board-domain raw |
| 0x0102 | AI3_BOARD_RAW | Board-domain raw |
| 0x0103 | AI4_BOARD_RAW | Board-domain raw |
| 0x0104 | AI1_ADC_CODE | Native ADS1115 ADC code |
| 0x0105 | AI2_ADC_CODE | Native ADS1115 ADC code |
| 0x0106 | AI3_ADC_CODE | Native ADS1115 ADC code |
| 0x0107 | AI4_ADC_CODE | Native ADS1115 ADC code |

### 6.3 Holding Registers (FC03/FC06/FC10)

| Address | Symbol | Access | Description |
|---|---|---|---|
| 0x0000 | AO1_SETPOINT | R/W | AO1 setpoint (0..4095) |
| 0x0001 | AO2_SETPOINT | R/W | AO2 setpoint |
| 0x0002 | AO3_SETPOINT | R/W | AO3 setpoint |
| 0x0003 | AO4_SETPOINT | R/W | AO4 setpoint |
| 0x0004 | AO1_MODE | R/W | 0=Voltage, 1=Current |
| 0x0005 | AO2_MODE | R/W | 0=Voltage, 1=Current |
| 0x0006 | AO3_MODE | R/W | 0=Voltage, 1=Current |
| 0x0007 | AO4_MODE | R/W | 0=Voltage, 1=Current |
| 0x0008 | DEVICE_ID | R/W* | Modbus slave address (1..7), write via FC06 |
| 0x0009 | FW_VERSION | R | Firmware version (`0x0104`) |
| 0x000A | STATUS | R | Status bitfield |
| 0x000B | ERROR_CODE | R | Latched error code |

`R/W*`: writing `DEVICE_ID` is delayed-apply and persisted to Flash.

### 6.4 STATUS Bit Definitions (0x000A)

| Bit | Mask | Meaning |
|---|---|---|
| bit0 | 0x0001 | MODULE_READY |
| bit1 | 0x0002 | AI_OVERRANGE |
| bit2 | 0x0004 | AO_FAULT |

Notes:

- `MODULE_READY` is set after all 4 AI channels are sampled.
- If `AO_FAULT` is set, firmware clears `MODULE_READY`.

### 6.5 ERROR_CODE Definitions (0x000B)

| Value | Symbol |
|---|---|
| 0x0000 | NONE |
| 0x0001 | AI_ADC_FAULT |
| 0x0002 | AO_DAC_FAULT |
| 0x0003 | POWER_SUPPLY |

Note: `ERROR_CODE` is latched to the last non-zero error and is not auto-cleared during normal runtime.

---

## 7. AI/AO Data Processing and Calibration

### 7.1 AI Processing Pipeline

AI processing per channel:

1. Trigger ADS1115 single-shot conversion
2. Read `ADC_CODE`
3. Convert to `BOARD_RAW`
4. Apply calibration map `BOARD_RAW -> SPEC_RAW`
5. Apply calibration map `BOARD_RAW -> SCALED_MV`

Public contract remains fixed:

- `AI_RAW`: 0..4095
- `AI_SCALED`: 0..10000 mV

### 7.2 AO Processing Pipeline

AO processing:

1. Receive `SETPOINT` + `MODE` from holding registers
2. Convert setpoint to DAC code using calibration per mode
3. Write MCP4728 multi-write frame
4. Trigger general-call update

Modes:

- `0`: voltage mode (public contract 0..10V)
- `1`: current mode (public contract 4..20mA)

### 7.3 Calibration Storage

Calibration data is stored in internal Flash:

- Address: `0x0800F000`
- Magic: `0x43414C31` (`CAL1`)
- CRC32 protection enabled

If calibration record is invalid, firmware falls back to defaults from `app_config.h`.

### 7.4 Device Config Storage (Modbus Address)

Device configuration is stored at:

- Address: `0x0800F800`
- Magic: `0x43464731` (`CFG1`)

Persisted data:

- Modbus slave address (1..7)

### 7.5 Optional Test Hook

Build option `ANALOG_IO_ENABLE_TEST_HOOKS=ON` enables calibration test hook support.

The test hook can install sample calibration when no factory calibration is available.

---

## 8. Factory Default Settings

### 8.1 Communication Defaults

| Parameter | Default |
|---|---|
| Modbus baudrate | 9600 |
| Modbus address | 1 |
| Modbus address range | 1..7 |
| Modbus frame timeout | 5 ms |
| Debug UART baudrate | 115200 |

### 8.2 AI Defaults

| Parameter | Default |
|---|---|
| AI raw max | 4095 |
| AI scaled max | 10000 mV |
| ADS1115 conversion wait | 2 ms |
| ADS1115 I2C timeout | 3 ms |

### 8.3 AO Defaults

| Parameter | Default |
|---|---|
| AO setpoint range | 0..4095 |
| AO mode | 0=Voltage, 1=Current |
| Safe boot setpoints CH1..CH4 | 0 |
| Safe boot modes CH1..CH4 | Voltage |
| Safe boot apply timeout | 100 ms |

### 8.4 Firmware Metadata

| Field | Value |
|---|---|
| FW_VERSION register | `0x0104` |
| Default build type | Debug |

---

## 9. Troubleshooting

### 9.1 Cannot Read Modbus

| Check | Action |
|---|---|
| Correct baudrate and 8N1? | Set 9600, 8N1 |
| Correct Slave ID? | Read/write register `0x0008` |
| RS-485 A/B wiring | Verify polarity and transceiver enable path |
| CRC/frame format | Ensure valid RTU frame + CRC16 |

### 9.2 Writing DEVICE_ID Has No Effect

| Check | Action |
|---|---|
| Value outside 1..7 | Write a valid value in range |
| Apply delay observed | Wait >20 ms after write command |
| Flash write failure | Check debug log `modbus addr save failed` |

### 9.3 AO Output Does Not Behave as Expected

| Check | Action |
|---|---|
| AO_FAULT bit set | Read `STATUS` bit2 |
| Error code | Read `ERROR_CODE` (0x000B) |
| MCP4728 ACK present | Check I2C scan logs in 0x60..0x67 |
| Mode/setpoint validity | Mode 0/1, setpoint 0..4095 |

### 9.4 AI Stays at Zero or Does Not Update

| Check | Action |
|---|---|
| ADS1115 ACK present | Check I2C logs in 0x48..0x4B |
| AI error code | Verify `0x000B = 0x0001` |
| Analog signal wiring | Verify input wiring and ground reference |
| Calibration mismatch | Validate calibration data in Flash |

### 9.5 STATUS/ERROR Does Not Return to Zero

`ERROR_CODE` is latched and may keep previous fault value until reset/reboot.

---

## 10. Tester Test Procedure

This section is intended for QA/test engineers to execute repeatable verification and record evidence.

### 10.1 Test Scope

Validate the following areas:

1. Boot behavior and startup diagnostics
2. Modbus RTU read/write behavior
3. Register map correctness (public and debug)
4. AO output control behavior
5. Device ID change and persistence
6. Error handling for invalid requests

### 10.2 Required Equipment and Software

| Item | Purpose |
|---|---|
| DUT board (STM32G030K8 with analog_io firmware) | Device under test |
| Stable power supply | Power the DUT |
| RS-485 to USB adapter | Modbus communication |
| Modbus master tool (Modbus Poll/QModMaster/pymodbus) | Send/read Modbus frames |
| USB-UART adapter for USART2 | Capture debug logs |
| Digital multimeter (recommended) | Verify AO electrical output |
| AI signal source or fixture (recommended) | Stimulate AI channels |

### 10.3 Pre-Test Setup

1. Flash firmware and power cycle the DUT.
2. Connect debug UART (USART2, 115200, 8N1).
3. Connect RS-485 adapter and configure Modbus master to RTU, 9600, 8N1.
4. If current slave ID is unknown, probe IDs `1..7`; then read `DEVICE_ID` at `0x0008` to confirm the active ID.
5. Ensure AI input fixture and AO measurement points are connected.

### 10.4 Functional Test Cases

Addressing note: all register addresses below are zero-based Modbus PDU addresses.
If your tool uses 1-based references (for example 40001/30001 style), disable offset mode or convert addresses accordingly.

| TC ID | Objective | Steps (using Modbus Poll or similar tool) | Expected Result |
|---|---|---|---|
| TC-BOOT-001 | Verify startup sequence | 1. Open Serial terminal (115200, 8N1).<br>2. Power on the DUT. | Logs show:<br>- `reset cause = ...`<br>- `modbus addr: X`<br>- `ads1115 ack` and `mcp4728 ack` |
| TC-MB-001 | Verify fixed holding registers | 1. Function: `03 Read Holding Registers`<br>2. Address: `9`<br>3. Quantity: `3` | Reads successfully:<br>- Addr `9` (FW_VERSION) = `260` (Hex 0x0104)<br>- Addr `10` and `11` return current status/error codes |
| TC-AI-001 | Verify public AI registers | 1. Function: `04 Read Input Registers`<br>2. Address: `0`<br>3. Quantity: `8` | Reads 8 registers successfully:<br>- First 4 ranges `0..4095`<br>- Next 4 ranges `0..10000` |
| TC-AI-002 | Verify debug AI registers | 1. Function: `04 Read Input Registers`<br>2. Address: `256` (Hex 0x0100)<br>3. Quantity: `8` | Reads internal board debug values successfully |
| TC-AO-001 | Verify AO setpoint write | 1. Function: `06 Write Single Register`<br>2. Address: `0` (AO1 Setpoint)<br>3. Value: `2048` | Modbus Tool reports Write OK.<br>Measured output changes consistently. |
| TC-AO-002 | Verify AO mode write | 1. Function: `06 Write Single Register`<br>2. Address: `4` (AO1 Mode)<br>3. Value: `1` (Current Mode) | Write OK.<br>Writing value `2` returns an Exception response. |
| TC-AO-003 | Verify FC10 multi-write | 1. Function: `16 Write Multiple Registers`<br>2. Address: `0`<br>3. Quantity: `8`<br>4. Use valid values for full AO block, example: `1000,2000,3000,4000,0,1,0,1` | Write OK.<br>Read back using FC03 (Address=0, Qty=8) returns the written values. |
| TC-ID-001 | Verify Device ID update | 1. Set Tool Slave ID to current device ID.<br>2. Function: `06 Write Single Register`<br>3. Address: `8`<br>4. Value: a new ID in `1..7` and different from current ID | Write OK.<br>Wait >20 ms. DUT responds on the new Slave ID and no longer responds on the old Slave ID. |
| TC-ID-002 | Verify Device ID persistence | 1. Power cycle DUT (after TC-ID-001).<br>2. Set Tool Slave ID = new ID from TC-ID-001.<br>3. Function: `03 Read Holding Registers` (Addr=8, Qty=1) | Read OK on the new ID. Register `0x0008` equals the configured new ID. |

### 10.5 Negative and Robustness Test Cases

| TC ID | Objective | Steps (Intentional Fault Injection) | Expected Result |
|---|---|---|---|
| TC-NEG-001 | Unsupported FC handling | 1. Send a valid RTU request using unsupported Function `05 Write Single Coil` | Exception Response Code `0x01` (Illegal Function). Device continues to operate normally. |
| TC-NEG-002 | Illegal address handling | 1. Function: `03 Read Holding Registers`<br>2. Address: `12` (0x000C, out of valid range)<br>3. Quantity: `1` | Exception Response Code `0x02` (Illegal Data Address). |
| TC-NEG-003 | Illegal value handling | 1. Function: `06 Write Single Register`<br>2. Address: `0` (AO Setpoint)<br>3. Value: `5000` (Exceeds 0..4095 range) | Exception Response Code `0x03` (Illegal Data Value). Previous value is unaffected. |
| TC-NEG-004 | Wrong slave ID behavior | 1. Set Tool Slave ID to a value different from the active DUT ID.<br>2. Execute any Read/Write command. | Timeout in Modbus Tool. No response from DUT (request ignored). |

### 10.6 Evidence to Capture

For each test case, collect:

1. Modbus transaction screenshot/log (request + response)
2. Debug UART snippet (for boot/error related tests)
3. Register readback values
4. AO measurement values (if AO is involved)
5. Pass/Fail decision and tester comment

### 10.7 Pass Criteria

The test run is considered passed when all conditions are met:

1. All functional test cases pass.
2. Negative test behavior matches expected exception/ignore behavior.
3. No unexpected resets or unstable communication during the run.
4. Device ID persistence is confirmed across reboot.

---

## 11. Quick Validation Checklist

After flashing firmware, validate in this order:

1. Open debug UART at 115200 and inspect boot log.
2. Confirm I2C scan detects ADS1115/MCP4728.
3. Read `FW_VERSION` at holding register `0x0009`.
4. Read AI public registers `0x0000..0x0007`.
5. Write AO setpoint/mode at `0x0000..0x0007` and observe output.
6. Change `DEVICE_ID` via `0x0008`, wait >20 ms and read back, then power cycle and read again to confirm Flash persistence.
7. Check `STATUS` (`0x000A`) and `ERROR_CODE` (`0x000B`) under normal and fault conditions.

If all checks pass, firmware is ready for commissioning.

---

*End of User Guide*
