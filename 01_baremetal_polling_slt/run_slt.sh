#!/bin/bash
set -e
echo "[STEP 1] Cross-compiling Bare-metal ARM Cortex-M7 Firmware..."
arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -g -O0 -nostdlib -T linker.ld main.c -o firmware.elf

echo "[STEP 2] Flashing Target SoC via OpenOCD (SWD)..."
openocd -f board/st_nucleo_f7.cfg -c "program firmware.elf verify reset exit"

echo "[STEP 3] Executing Automated SLT Test Vectors..."
python3 slt_linux.py
