# Third-Party Licenses

EdgeFlex AI's own code (`EdgeFlex_Runtime/`, `Baseline/`, `Models/`, `Tests/`,
`Core/Src/main.c`, `Core/Src/stm32f4xx_it.c`, `Core/Src/syscalls.c`,
`Core/Src/sysmem.c`, `Core/Inc/*`, `STM32F401CCUX_FLASH.ld`) is original and
licensed under MIT — see `LICENSE`.

`Drivers/` contains **unmodified, official STMicroelectronics sources**,
fetched directly from ST's public GitHub repositories, used here under their
own upstream licenses (full text copied into this `Docs/` folder):

| Directory | Upstream project | License | Full text |
|---|---|---|---|
| `Drivers/CMSIS/Include` (core_cm4.h, cmsis_gcc.h, m-profile/*, etc.) | [STMicroelectronics/cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | Apache License 2.0 | `Docs/LICENSE-cmsis_core.md` |
| `Drivers/CMSIS/Device/ST/STM32F4xx` (stm32f401xc.h, system_stm32f4xx.c, startup_stm32f401xc.s) | [STMicroelectronics/cmsis_device_f4](https://github.com/STMicroelectronics/cmsis_device_f4) | Apache License 2.0 | `Docs/LICENSE-cmsis_device_f4.md` |
| `Drivers/STM32F4xx_HAL_Driver` | [STMicroelectronics/stm32f4xx_hal_driver](https://github.com/STMicroelectronics/stm32f4xx_hal_driver) | BSD 3-Clause | `Docs/LICENSE-stm32f4xx_hal_driver.md` |

All three are permissive licenses compatible with redistributing this
repository under MIT for EdgeFlex's own code; the vendored files themselves
retain their original ST/Apache-2.0/BSD-3-Clause headers and are **not**
relicensed. No third-party code outside of these three official ST
repositories was copied into this project.
