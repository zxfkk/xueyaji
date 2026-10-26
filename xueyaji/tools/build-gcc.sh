#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cc=${ARM_CC:-arm-none-eabi-gcc}
# Keep fresh outputs separate from the historical, tracked Keil Objects directory.
out=${BUILD_DIR:-/tmp/smartbp-gcc}
mkdir -p "$out"
flags=(-mcpu=cortex-m3 -mthumb -std=c99 -Os -g -ffunction-sections -fdata-sections -fstack-usage -Wall -Wextra -Werror -DSTM32F10X_MD -ICMSIS -ICore/Inc -IAlgorithm/Inc)
libs=()
# Optional relocated Debian toolchain (normal system installations need neither).
if [[ -n ${NEWLIB_ROOT:-} ]]; then
 flags+=(-isystem "$NEWLIB_ROOT/usr/include/newlib")
 libs+=(-L"$NEWLIB_ROOT/usr/lib/arm-none-eabi/newlib/thumb/v7-m/nofp")
fi
objects=()
for src in CMSIS/system_stm32f10x.c Core/Src/*.c Algorithm/Src/*.c Project/GCC/syscalls.c; do
 obj="$out/$(basename "${src%.c}").o"
 "$cc" "${flags[@]}" -c "$src" -o "$obj"
 objects+=("$obj")
done
"$cc" -mcpu=cortex-m3 -mthumb -c Project/GCC/startup.S -o "$out/startup.o"
"$cc" -mcpu=cortex-m3 -mthumb -nostartfiles -T Project/GCC/stm32f103.ld \
 -Wl,--gc-sections,-Map="$out/smartbp.map" "${objects[@]}" "$out/startup.o" \
 "${libs[@]}" -Wl,--start-group -lc_nano -lm -lgcc -lnosys -Wl,--end-group \
 -u _printf_float -o "$out/smartbp.elf"
"${cc%gcc}size" "$out/smartbp.elf"
"${cc%gcc}objcopy" -O ihex "$out/smartbp.elf" "$out/smartbp.hex"
printf 'Firmware: %s/smartbp.hex\n' "$out"
