# Capacitive Tactile Sensing and Electrical Feedback

Firmware for a two-controller system that acquires 32 capacitive sensor channels, displays a hand heatmap, and controls electrical stimulation feedback.

| Controller | Platform | Role | Source baseline |
|---|---|---|---|
| MCU1 | STM32G0B1CBTx | Sensor acquisition and UART streaming | `851177203200d9f4da68da3185e82e0c7f283175` (`rm32channels`) |
| MCU2 | ESP32-S3 | Touch display, zero calibration, and stimulation control | `b766300` (`LCD-1.69`), with the retained local `sdkconfig` |

Prepared on **2026-10-08**. The exported firmware and build configuration files preserve the original contents. Packaging added ignore rules and documentation; it did not change firmware behavior.

## System overview

```text
8 x RM1002B (4 channels each)
        |
        | Two I2C buses
        v
MCU1: STM32G0B1
  Read ADC and compensation registers
  Apply the capacitance model and board calibration
  Encode 32 channels
        |
        | UART, 2,000,000 bps, 8N1
        v
MCU2: ESP32-S3
  Decode frames and subtract zero offsets
        |                         |
        v                         v
ST7789 / LVGL heatmap      HV2801 channel selection
CST816S touch controls     DAC80502 amplitude control
                          ADC-controlled pulse width
```

MCU1 produces capacitance-model values, described in the source comments in pF. No conversion to calibrated force or pressure units such as N or kPa has been established here. The MCU2 thresholds therefore apply to the received values after zero subtraction.

## Repository layout

```text
MCU1/
  Src/                      Acquisition firmware and peripheral code
  Inc/                      Application and peripheral headers
  Drivers/                  STM32 HAL and CMSIS, including license files
  MDK-ARM/
    project.uvprojx          Keil project
    startup_stm32g0b1xx.s    Startup assembly
    RTE/                    Runtime environment configuration
    EventRecorderStub.scvd
  project.ioc               Original CubeMX configuration
  .gitignore
MCU2/
  main/                     Application entry and dependency manifest
  components/BSP/
    LCD/                    Display interface
    LVGL/                   Touch UI, hand images, and heatmap
    UART/                   Frame decoding and zero calibration
    Stim/                   HV2801, DAC80502, and ADC control
  CMakeLists.txt
  dependencies.lock
  partitions.csv
  sdkconfig                 Retained configuration baseline
  .gitignore
README.md
```

MCU1 uses the acquisition revision specified above. The later UART isolation revision is not included. MCU2 retains the CMake project name `LCD-1.69` even though its directory is named `MCU2`.

## Hardware connections

### Between controllers

| MCU1 | MCU2 | Function |
|---|---|---|
| PA9, USART1 TX | GPIO35, UART RX | Sensor data |
| PA10, USART1 RX | GPIO34, UART TX | Start and control commands |
| GND | GND | Common reference |

Verify the electrical interface against the actual boards before connecting them.

### MCU1 acquisition hardware

| Interface | Signals | Configuration |
|---|---|---|
| I2C1 | PB8 SCL, PB9 SDA | Four RM1002B devices; channels 0-15 |
| I2C2 | PA7 SCL, PA6 SDA | Four RM1002B devices; channels 16-31 |
| USART1 | PA9 TX, PA10 RX | 2 Mbps, 8 data bits, no parity, 1 stop bit |
| System clock | Configured in firmware | 64 MHz |

Each I2C bus is read in address order `0x28`, `0x29`, `0x2A`, `0x2B`, with four channels per device. TIM1 sets an acquisition flag every 10 ms. This is a nominal scheduling rate of 100 frames/s; blocking reads and compensation can reduce the actual rate.

The parasitic-capacitance arrays are specific to the original board. Their values are preserved and should not be assumed valid for another board.

### MCU2 display and feedback hardware

| Module | Signal order | GPIO order |
|---|---|---|
| ST7789 LCD | CLK / MOSI / DC / CS / RST / BLK | 13 / 14 / 11 / 12 / 15 / 10 |
| CST816S touch | SDA / SCL / INT | 17 / 18 / 16 |
| UART | TX / RX | 34 / 35 |
| HV2801 | CLR / CS / CLK / DIN / DOUT | 40 / 39 / 38 / 37 / 36 |
| DAC80502 | SCK / MOSI / CS | 47 / 48 / 26 |
| Potentiometer ADC | Input | 3 |

## Build and flash

### MCU1: Keil MDK

1. Open [MCU1/MDK-ARM/project.uvprojx](MCU1/MDK-ARM/project.uvprojx).
2. Select the `project` target.
3. Use the recorded toolchain: **ARMCC V5.06 update 5, build 528**, and **Keil.STM32G0xx_DFP 2.1.0**. Install the CMSIS package required by the project's RTE configuration.
4. Build the target and configure your board's debug/programming adapter in Keil before downloading.

The original `project.ioc` is provided for reference. Regenerating the project in CubeMX is not required for this build and may affect custom source code.

The historical build log reported zero errors and zero warnings. The exported project has not been rebuilt or flashed during this preparation.

### MCU2: ESP-IDF

Use an **ESP-IDF 5.3.3** environment. From the repository root:

```sh
cd MCU2
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with the actual serial port. The first build requires access to the component registry to restore dependencies.

| Dependency | Recorded version |
|---|---|
| ESP-IDF | 5.3.3 |
| LVGL | 9.5.0 |
| esp_lvgl_port | 2.7.2 |
| esp_lcd_st7789 (jbrilha) | 1.0.2 |
| esp_lcd_touch_cst816s | 1.1.1~1 |

The retained `sdkconfig` selects ESP32-S3, 16 MB flash, a 240 MHz CPU, no PSRAM, and the custom `partitions.csv`. It is intentionally included rather than replaced by generated defaults. `dependencies.lock` is also retained.

The original manifest declares `IDF >=4.1.0`, which is broader than the locked dependencies support. Use the recorded 5.3.3 baseline; the declaration was not changed during packaging. No clean build of the exported MCU2 directory has been performed.

## Display and controls

| Setting or control | Current behavior |
|---|---|
| Display | ST7789, 240 x 280, RGB565 |
| Touch | CST816S |
| Default layout | Right hand, 29 points; a 16-point branch is retained |
| ZERO | Records the current received values as zero offsets |
| STIM | Enables or disables stimulation; disabled at startup |
| SEN | Displays the potentiometer-derived setting, from 0 to 10 |
| I / II / III | Selects a DACA upper limit of 850 / 950 / 1050 mV; default is II |
| Heatmap and stimulation thresholds | 700-800 after zero subtraction |
| Stimulation period | 40 ms, approximately 25 Hz |
| Pulse width | `sensitive * 300 us`, from 0 to 3000 us |
| Fixed-channel test mode | Disabled: `STIM_FORCE_OPEN_CH = -1` |

DACA values are control voltages, not verified stimulation-current values. Display positions and sensor-to-HV mappings are separate tables. Left-hand and alternative layouts require their own hardware validation.

## UART protocol

### Start streaming

MCU2 sends an 18-byte command. Bytes 0-3 are `FF FF 06 09`, byte 17 is `11`, and all other bytes are zero:

```text
FF FF 06 09 00 00 00 00 00 00 00 00 00 00 00 00 00 11
```

MCU1 then enters continuous streaming mode. MCU2 retries the start command after approximately three seconds without a valid frame.

### Sensor frame

The complete frame contains **212 bytes**. Offsets below are zero-based.

| Byte offset | Contents |
|---|---|
| 0-3 | `FF FF 06 09` header |
| 4-5 | Frame sequence number |
| 6-7 | `00 CC`: 204 bytes, equal to the total length minus the first 8 bytes |
| 8-11 | Device ID |
| 12-15 | Target ID |
| 16-17 | `00 12` response code |
| 18-19 | `00 20`: 32 channels |
| 20-211 | 32 channel values, 6 bytes per channel |

Each channel contains a three-byte big-endian integer part followed by a three-byte big-endian fractional part:

```text
value = integer_part + fractional_part / 1,000,000
channel i begins at byte 20 + 6 * i
```

This representation matches the MCU2 decoder for the intended nonnegative values. Negative and non-finite values are not handled robustly by the current firmware.

## Known limitations

These behaviors are preserved from the source versions; packaging did not fix them.

### Acquisition firmware

- The `0x71` coefficient-command buffer is cleared before the main loop reads its parameters. Coefficient handling is incomplete, and flash-save calls are commented out.
- `coe_buffer[32] = {1.0}` initializes only the first entry to 1; the others are zero. The acquisition conversion does not use this array.
- RM1002 automatic-compensation polling has no overall timeout and may block acquisition if the device does not report completion.
- Capacitance conversion lacks complete protection against a near-zero denominator, negative values, and non-finite results.
- Command-length validation, concatenated commands, and concurrent receive-buffer access need further work.

### Display and feedback firmware

- When sensor frames stop arriving, the UART task retries the start command but does not invalidate the last stimulation target. Static review indicates that stimulation may continue using the last valid sensor value.
- The available hand/layout branches do not establish that all configurations have been tested.
- Board identification, supporting circuit documentation, and measured validation results remain to be completed.

The project includes high-voltage stimulation control. Verify operation with a dummy load first. This source export is not evidence of completed hardware or safety validation.

## Verification and release preparation

- **MCU1:** 111 original files were exported directly from the specified commit and verified byte for byte. All 38 files listed in the Keil project's source-file entries are present. Streaming-frame layout and channel offsets were checked statically.
- **MCU2:** 26 original source and build-configuration files were copied and verified using SHA-256.
- New `.gitignore` files exclude generated outputs, personal settings, and common credential files.
- Neither firmware was rebuilt, flashed, or tested on hardware during preparation. No repository was initialized and nothing was pushed to a remote.

The exports exclude Git history, IDE user settings, debugger logs, build artifacts, and outdated project notes containing local paths. MCU2's downloaded components are restored by its component manager.

The original MCU1 build log contained software-license information and was tracked in its source history. Both the log and history were excluded. Retained files were scanned for common tokens, private keys, password assignments, credential-bearing URLs, license identifiers, and personal paths, with no matches in the selected MCU1 files. MCU2 review found no confirmed credentials within its scanned scope; local paths and historical author metadata were excluded from the export.

These checks are pattern-based, not a guarantee that every possible sensitive item has been identified. Future commits use the publisher's configured Git identity; choose an appropriate public email before creating them.

## Licensing and asset provenance

Existing third-party license files in MCU1's HAL and CMSIS directories are retained. MCU2 dependencies retain their own upstream licensing terms.

Original project code and documentation are licensed under the [MIT License](LICENCE), Copyright (c) 2026 Shuai Dong. Third-party code and assets remain subject to their respective licenses. The source of MCU2's `hand_map.c` and `hand_map_right.c` image assets has not been verified; the project license does not establish redistribution rights for these assets.

## Citation

If you use this software, please cite **Shuai Dong, Capacitive Tactile Sensing and Electrical Feedback**. Machine-readable metadata are available in [CITATION.cff](CITATION.cff), following the [Citation File Format](https://github.com/citation-file-format/citation-file-format/blob/main/schema-guide.md). The license text follows the [MIT License](https://opensource.org/license/mit).

## Original-file checksums

The following SHA-256 manifests record the exported original files. Newly written documentation and `.gitignore` files are outside these manifests.

<details>
<summary>MCU1 SHA-256 manifest (111 files)</summary>

| Path relative to MCU1 | SHA-256 |
|---|---|
| Drivers/CMSIS/Device/ST/STM32G0xx/Include/stm32g0b1xx.h | 7c6bd009a910a44338fee0ca537a26cc488f38ced5bb830efce6c41e9a4192af |
| Drivers/CMSIS/Device/ST/STM32G0xx/Include/stm32g0xx.h | 048f668364ee05c0124a23cca9a2a4100fdfc4d8c2dded4c72a63add0576d51a |
| Drivers/CMSIS/Device/ST/STM32G0xx/Include/system_stm32g0xx.h | 20ee999e4431472a7c351e2412537aa0ef7f5f5e499a21a92c1d4cc21f84ebf1 |
| Drivers/CMSIS/Device/ST/STM32G0xx/LICENSE.txt | 135fb2d86e9ecdf6824cc3bba21c72b8e380c07b055fbebb6b995463eb609baf |
| Drivers/CMSIS/Include/cmsis_armcc.h | 5a80286d68d0b4478895a222da683c8ca561d3bcf6392aaaea225d4a117c843e |
| Drivers/CMSIS/Include/cmsis_armclang.h | e24f4bd1548b94e0ad424f4f13f59a4db4ca5aad7b644d10a893a330775d3003 |
| Drivers/CMSIS/Include/cmsis_armclang_ltm.h | e8f040e9ef61e466cc751677ab3c413a463cb74d1fc33fb6dfd94225bd93c8c1 |
| Drivers/CMSIS/Include/cmsis_compiler.h | b51963d271c1571ca3463654c76aa7ea50c4eac5d0a2710df10414ac64d9e0f8 |
| Drivers/CMSIS/Include/cmsis_gcc.h | 1fba3e120e5c0a439e0982bd37fae04bd033dbd14187428214462a1707c56252 |
| Drivers/CMSIS/Include/cmsis_iccarm.h | 820bcd68b0a92f97abf5cdde6413e5e842fb286e0514be2c1be1547bb2dd2e7d |
| Drivers/CMSIS/Include/cmsis_version.h | 1b2c47a90e9ae741c8fd4d0f323495327b25ff5d555a6c7afd6749a65ba1bb8e |
| Drivers/CMSIS/Include/core_armv81mml.h | b4bc83a6ff10fd2fb7e4c0abefdbd447eb7f78f3db898ca9aac5676307e2b862 |
| Drivers/CMSIS/Include/core_armv8mbl.h | 74b92c994af7355e222b45a0342754840b7221b611ef95bd206345879cc2caaa |
| Drivers/CMSIS/Include/core_armv8mml.h | 77255f30508cf0d225f8b72207e28bf81353f8b353e8b120132c6db7e9bead9e |
| Drivers/CMSIS/Include/core_cm0.h | 52e8826259be4fcb10d0de2d3655c24d52436e54cd1509f3beb1a31a45524f38 |
| Drivers/CMSIS/Include/core_cm0plus.h | 36e9aec815eef944241e5fed7085845346de6bcb5ba30561f956356b393ba5f0 |
| Drivers/CMSIS/Include/core_cm1.h | 944bf0c4f940eef30a632cd99d5dcfe8270e8a8431eb98c0f081bcf4f61f728e |
| Drivers/CMSIS/Include/core_cm23.h | f04d3e5f385b0ff219a75f6648acabf7debb779a72f0628573b64dac5ad49fdc |
| Drivers/CMSIS/Include/core_cm3.h | 0191499015f3af7b37ce83092df29d1d63dbba9fa5f31b6d2341a2e850b20cb4 |
| Drivers/CMSIS/Include/core_cm33.h | 3566c5bf835091b1cc46baf6e9b6ce94d83dd0885e0db122f43e901e9326945a |
| Drivers/CMSIS/Include/core_cm35p.h | 1f6da4c033261293acb8f0a7da42a0e9b6ab392c9462cc26f5e3e35d4db4e2d3 |
| Drivers/CMSIS/Include/core_cm4.h | 53d609b900c262650fcaf5a48eabd0fa299c3ac4d1426eb0f20e2aab053f8abb |
| Drivers/CMSIS/Include/core_cm7.h | 34d91327a5f1b251c54fa6bb532d9f5ec0423bf3b7f2b679c8f7b3df95af0fc8 |
| Drivers/CMSIS/Include/core_sc000.h | cc9037b32401824436d251de860799fcdbab3cf69e2321438c8f446f3f342526 |
| Drivers/CMSIS/Include/core_sc300.h | e6a78d85b3ca77e35dc512e32e535028a46e156adab8629360fa4b7fe6731827 |
| Drivers/CMSIS/Include/mpu_armv7.h | c9ad7cc99ba0481365550ae997c63cde9732d067d9ec0de28b0d6ac39280656d |
| Drivers/CMSIS/Include/mpu_armv8.h | 53f9e848af62ea8704f463992e4d65c7c79fc72cfd8fab6fa14adc73478b0afd |
| Drivers/CMSIS/Include/tz_context.h | 45565c8cb04c653aabaf3cceddfc848b6a0de28c20d4f7e3d475a992c7eee788 |
| Drivers/CMSIS/LICENSE.txt | b40930bbcf80744c86c46a12bc9da056641d722716c378f5659b9e555ef833e1 |
| Drivers/STM32G0xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h | 417342f37c87e8a96b28980f0cd616dda6100b3220279e8a248299a54f226ba1 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal.h | 2f1ab28aadd725db91cc313a31d55fd237c5304f40a741511598ea09d36f8ee3 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_cortex.h | 87a38744d4f5c868f6ec143d90be71bdfb15ec11fbac177d0db6ff54da8aff8a |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_def.h | b584b0eaa6f621c9ec270511cc0543333c3a616fe9351f5929614db0973f6a42 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_dma.h | 1fbea3b91861bd184bda3bad2078524e974b6e0695feff0cbb266255028ab864 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_dma_ex.h | df38b9267576aee372d41fb0cb8bf68fa1db35cf600973edb3f014023a3bd4fd |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_exti.h | 6e5e5946bbb8f8d2f484c30f07963f6f87f615fb39d291c82146f3b0c2c8ca39 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_fdcan.h | 425f74398399fc47f21798694d86a67d2cf1f43bc2204db0c117cb1d5d30c08c |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_flash.h | 50fb80882ca457aa544b612e9218131ecb3d8a13f6cf60415ee23898a29809d1 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_flash_ex.h | 0df14428c6d09c95da0386e1bd976c47efd13e0dc20e358ba7d796e9b3360fc8 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_gpio.h | 6683a01cba5e441f0e3e9a6fb8c2918fb98c4d86d20873a1f84e376d0b3c14ae |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_gpio_ex.h | e7824f003de5e934332427b629d8d04745f97e919e259feb593a28cee6039ef4 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_i2c.h | 3dd6dd724c667a18d9897307431c084a2a8a7d5abfd2a252b2e93112a9462d2d |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_i2c_ex.h | e87a93403bfbbe1a08b781943677693f3f7e895882945d2fd71b5be8dc29f11b |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_iwdg.h | 2e0b66139d84be9f622e0faa3aa4d80afed838fa20e158fa7a62f7ef0985b3e2 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_pwr.h | 50cf088e85ed75fe43e6c23faf3dfd1d649b29111d2a4d50612ba64d78675cb9 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_pwr_ex.h | 0f3e441d0e7bc216da3c5d23acd88001bcc9c5d5b8403767ad2796e02f33946c |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_rcc.h | 1a51d8ee0e9c08a3e24b60440a07244f28ec2f2f79c4508d1bf2359e21764f2b |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_rcc_ex.h | 6bba10209e50b22c372563f1a66b7653031df1f622a3e8301767dd7c4c26d8d2 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_tim.h | e9ce25ffa2ebdfa9c6f8f83146432ccfb17f641621e00eb9642791be0a693e07 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_tim_ex.h | 300ba7761ec95ac76e1f10110da65c7384bb4e217236c61b8c50886fdcb61ded |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_uart.h | fddafad3f431ad73da39925b4df35f4231bb3bb3b863fdedfc4e15f3f00eff77 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_uart_ex.h | cdd4eda9096943818e8d116a87b150fc88b41a23453524a080bde3ab88d7eb73 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_usart.h | 18cd3b35c6e6bc604c9fb8b9c558675bfdc14c22b4b46ed91fa921301760f072 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal_usart_ex.h | dd47bda76adf2ce81558f3aeb802718d5b875a9968cef625a6a9289bbf41e072 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_ll_dma.h | bc8077bcef746a10a9386be35d986795f296a0f62fc5e38c0332336455c448b3 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_ll_dmamux.h | 78f520cdd51d7d8e4258890aee2b282bdb28b5153a90d378f530080fbf36ba14 |
| Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_ll_rcc.h | 6d517a2a6fce26f0fa07a6e20dc9a6605cff90e42b8c0a62a3a828c6d70870d5 |
| Drivers/STM32G0xx_HAL_Driver/LICENSE.txt | d5a162f3eaf2b7b6762f00700017cae5695ebbdce7932fead8316448baafd9c1 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal.c | 55a7cb62ee4af66288af056569fd138d1e5d5ae107d2a62460e686b29f9e47f3 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_cortex.c | cdde2003ac16aca5af8e4b82cc604bce6e0e528db85e442400519592836e693a |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma.c | 16865d9d198d1862d31365a899b3b5f614eb85a143335fc712e78cf8878d7bfc |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_dma_ex.c | c611236c3bf301b1ab4e7cbd271d809897ebc376e8c24244908875c8d1c046ed |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_exti.c | 75d1c05eb554325770518c2d660c0ab2f2b7d168e7e18156ae8592d7f5558ae1 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_fdcan.c | a525e547dfdca57c0ad379234555c87fac927d825b633796534daa4740d5e3c1 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash.c | 7080740308fdbf2b270d34469e5a0a45f2740aad10e5105925f855ae762fce4b |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_flash_ex.c | fc656e5a69ba25baacf9c693940dab5a00c4cc84bd19b8e114c1d01a747b98ed |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_gpio.c | 7159ab33622ddb7a0d9a6511f824516ea35af8396f294a2bf60cfe4dc96516ad |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c.c | 96547e118131db660929c235cdbe0db732998b49fbfa55e6bf0fc45d7ac3f68d |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_i2c_ex.c | 7481700f3e6a12bd2d5c7f36c9daca96109a4996e0b266ee0c2232e2600f201a |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_iwdg.c | 808269f5f6a65c867de565abb66dc1180ae9901ba8c75ab9d443bc06c877a866 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr.c | 4ba4274462bc7fb8ff23c1bf64f8401f9f04472fe9f12ec1042b9eafa1282f2c |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_pwr_ex.c | 763ef62c979643395289867c20c4b1b9f23c149b3ab99d20ad4cbb25daee6799 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc.c | 4cc6e355a1d3a2d7565b082ddea7b024f3a192ae6571fd350e2f4c25c657e5b4 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_rcc_ex.c | 0fb2a70e91f50ebd08d144c2f627cf08582b4c532d38c58518b091323265a0db |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim.c | 93ec37694887795b7c7010b1130006007e6f137282c58797ef1ae8c249affbe1 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_tim_ex.c | fe4d6d0722ba1179bf90816d8bb28abd14186b33e9ef022c6a4b23382614ff95 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart.c | 286df00b6c6995f4953d740f4a86f3f3542dead1deab77266f393dca5dfe5812 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_uart_ex.c | be4f09aa66f8debfd436919035e492639a7fcb190ce49851363defc22057543a |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_usart.c | f7f908c1218bf44f3fb6624ade48d07bcefe036a2b328fe76e9166f4aa5ef1ab |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_hal_usart_ex.c | 3476881c97e8feab696a6c45537b433b38461973d8c32605b042f5a5e973fe63 |
| Drivers/STM32G0xx_HAL_Driver/Src/stm32g0xx_ll_rcc.c | 191f704710d29995950dc6458bb2b73b2e86d07e3b64f062ac66663782c0811d |
| Inc/can.h | 04442123f4f69cfbe4cc4746089145357b47904d58c102a801e4e9fedece1ef5 |
| Inc/flash.h | 56a18362ad30d0bc01a26acfa27f87399bf530899a1152a0cef9045ef90d1596 |
| Inc/gpio.h | cf85313b82bdad074d77ea94ed4582b316a0b85eab11f7b8b0829e5d605af0a4 |
| Inc/i2c.h | e53d1f9761e363a062c87aabdb404046b6b93002786587191c0bd09620446c76 |
| Inc/iwdg.h | a089ad387db7045c797c0a465bd6517648d32a8bbcb0f71db8d34871c7be0d22 |
| Inc/main.h | f0978ea1fff35bc6e3614108b5d4cb622982b9a3adf4fdddb7811581010b7a57 |
| Inc/rm1002.h | e0a05c61ddbad5bb8bb4f6bfeaf5f7cdf4cf7e85960185a845b472def70fef65 |
| Inc/stm32g0xx_hal_conf.h | cfde756c25b4f215149dd447d4906e0fdd363cc8c7fbe680569c495d3b9b79bd |
| Inc/stm32g0xx_it.h | e878c43ab3398ad5f6325bfd602adeb3e0e4e4f5c8bba9b52159f7e3f1526640 |
| Inc/tim.h | ccd03db9f1d6e362aaa4546cb526bce12b0be3bfab466d5f9d16ed9d2325c6cd |
| Inc/usart.h | 83542699b6d73311f87c1f863d952aa8f05891a774b360152afa929249340d94 |
| MDK-ARM/EventRecorderStub.scvd | 2482a3601234c1c796dd7be18c0aa71f194088cecdf47f641eef11aa734ad5c3 |
| MDK-ARM/RTE/Device/STM32G0B1CBTx/STM32G0x1.dbgconf | b5eda5dae86ac13c7241189173179bc876cc8248d4f9aff1295331d5569c9b8b |
| MDK-ARM/RTE/Device/STM32G0B1CBTx/STM32G0x1.dbgconf.base@1.0.1 | b5eda5dae86ac13c7241189173179bc876cc8248d4f9aff1295331d5569c9b8b |
| MDK-ARM/RTE/_project/RTE_Components.h | 40764d588c1450edf3790f6329c7749c10f2dba4f451090ee35969ba9d725bfe |
| MDK-ARM/project.uvprojx | 5b431b086af02909b41827a796e95fd3dbd236c626722b826ec17e79a0e633a1 |
| MDK-ARM/startup_stm32g0b1xx.s | f1e2713ed1190986fa29cb017c189daea1d9bc3c44801170c9f617dbcf571823 |
| Src/can.c | 6037cd4e3f2950d091408cf50f44685e8b8eb8bd12854d6a1bc00640bd670b33 |
| Src/flash.c | ac1cb44cc2bb25fa0f9e3b53430bb06d91fe0020a825fa215aa0dc692b51ebab |
| Src/gpio.c | b4e8e050797761f865bec82d628f1b117d05a81729671f17d88656a5166fac70 |
| Src/i2c.c | b0d3edede06aea688dbdd30da10f976303fab52bd65faf2582c22cd972d205b3 |
| Src/iwdg.c | 05616e7b1f9ac208eb94db64cac46923fe5584e1387ce8d35893716b15f154e8 |
| Src/main.c | e37a394d457aae4b3cddcc5d374067aa4d6f1e591acff841122af98bd6f196a6 |
| Src/rm1002.c | c1c7bc717358da56bb744fbfde203a040b89e53a42bec6b85cd974ea3de1c836 |
| Src/stm32g0xx_hal_msp.c | cff097770a5c4deea7d53f1c857ecfd8b8d4ef58b9e5a00e055a4086d0d1ed43 |
| Src/stm32g0xx_it.c | f70db2ad13a87deb367d714ef615474884c26801fa3dd3478e31bac2e7f4c170 |
| Src/system_stm32g0xx.c | 7fe8a9390964b76db1feab6297c1a97252dc022948b28365748be4048b230540 |
| Src/tim.c | 2e90672511f1807d3eedc832a73465fd497e3d55444912e1c815963e38269214 |
| Src/usart.c | f5508ca928a7270f8d3357ec09516e3c45e4246db2dcca5a47a0456fbf9dead9 |
| project.ioc | 86cc72b32949c9b59e683c06f7db90e79f7087119baef533a0f44db15c0aa184 |

</details>


<details>
<summary>MCU2 SHA-256 manifest (26 files)</summary>

| Path relative to MCU2 | SHA-256 |
|---|---|
| CMakeLists.txt | 7A42292649F38CD2DA2960F4676A31C3A9F1640B08C6E5A9585E7063C29FD9EB |
| components/BSP/CMakeLists.txt | 7DADFA4BE027EEB53670399DB95C6E828F2994ACDED3519EAD8DA7F0DB2F1C14 |
| components/BSP/LCD/lcd.c | 1F8300A4679C277111741879534F194FC9595039193680FC15C42862FB70C805 |
| components/BSP/LCD/lcd.h | C471A7F440AAAA6019F4C283FC7B24C1E4036972E321A992AC31614AEEA65DDF |
| components/BSP/LVGL/hand_map.c | 6C7BDB765A192C61C3E429D8A3FB3334A80BDED3FAA50C46C666BD4B5F4CA5C9 |
| components/BSP/LVGL/hand_map_right.c | 37E1330E3FFDC886B7461466081AA66FDF350761C747685008D97267B54177DF |
| components/BSP/LVGL/lvgl_ui.c | 2A852C95EBA9ED161F46B03A9772D48B091C7565C28C0D203FCBDA5C204F6ACC |
| components/BSP/LVGL/lvgl_ui.h | 2DF3810A745B8954381A0B4AD56A528B20DC710B0D2570A0D280BC3B5BD0E85C |
| components/BSP/LVGL/ui_matrix.c | F9CE9E58522BC256D1DDD1EABA8F28C2986FAE3EBCE7103AC63D807133C14621 |
| components/BSP/LVGL/ui_matrix.h | 09BADB2F40C619A9481C69A2D51B84E770D524D7E34DB5319E3998A614CED186 |
| components/BSP/Stim/dac80502.c | 072EA5F465B7DACC7D5B295577E8CB0D4393B3BC631240F75BF62F11EF57A5A5 |
| components/BSP/Stim/dac80502.h | 9CDEF22B401356EFCB707160207279865779233FB272B74650FE3DC75BCFEC87 |
| components/BSP/Stim/stim.c | 0B585F5CB6D18B670ACAECFED90A14888B74B81C78EDDCD5EAE02887CFC6363C |
| components/BSP/Stim/stim.h | 0073C145A141CB912541E6D7721D74533B49441FAD8BA69704EBF91CA60BC362 |
| components/BSP/Stim/stim_adc.c | 401AE0C4CEC3ED5EAE83633B5DBBA286AF4D6E8CBCB7247DEC92485EBA5A519A |
| components/BSP/Stim/stim_adc.h | F0512DDDD4BEB659266CB02DCE5DC6BCD9BD4A990B9EAA6B61C41B3EE552E693 |
| components/BSP/UART/uart.c | AB20B895DB0E9D7ED3ABAF9029BA2CCF0000B69F17722C49AD32A101184CEFC7 |
| components/BSP/UART/uart.h | 004909A18198018E30994155895F6C7C3F7FF03EE511913FF236733D9EF2759C |
| components/BSP/UART/uart_receive.c | 551BA81160EE07860E9AAF81631CCBA298FF88C2EE21C728F556B8EBEBC8C702 |
| components/BSP/UART/uart_receive.h | 7429015C35DA942744C6EDD792F8AF654667EBF799279DD9FBF72720F3930251 |
| dependencies.lock | 43DC946D1F587D8A339FB8FDC7652C96D95018943B2B22DE1B10082784AAFC29 |
| main/CMakeLists.txt | 4DC0761CF956649158A61DE7BE787532FBC027685E2685EAD5BB058BF66151B4 |
| main/idf_component.yml | 9DB33665622152C5428E85E93B3E48E8FAB424ED2B252B7DF9B655DF41A0DC75 |
| main/main.c | B0CCC214ABB5EECCE81BF6C06ABAE52F64F3FC9489AC3BB62C4A8D2CC38CB882 |
| partitions.csv | 6F624393444F4706D2BE898336CC6040A60CCEC3681DA00EE92522F1039B2CC3 |
| sdkconfig | 6DA8BB85BAE9A4B70D7A250FC4FB3B4BF4FBC73D521C47525900ACF51760E629 |

</details>


