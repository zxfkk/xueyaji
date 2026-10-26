#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
flags=(-std=c99 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -ICMSIS -ICore/Inc -IAlgorithm/Inc)
python3 tests/test_algorithm.py
gcc "${flags[@]}" tests/test_sensor.c Core/Src/xgzp6847d.c -lm -o "$build/sensor"
"$build/sensor"
gcc "${flags[@]}" -DBP_HOST_TEST tests/test_app.c Core/Src/main.c Algorithm/Src/bp_algorithm.c -lm -o "$build/app"
"$build/app"
