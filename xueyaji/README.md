# STM32F103C8T6 电子血压计实验固件

现有硬件引脚保持：PA7 气泵 PWM、PB0 常开电磁阀、PB6/PB7 压力传感器与 OLED、PA9/PA10 串口（9600 8N1）。也支持在 PB10/PB11 上探测 I²C 设备。使用板载 RESET 启停，或 PA0/PA4 对地按键（30 ms 轮询消抖）。串口 `S` 表示开始，`E` 表示停止，重复开始不会停止测量。

这是需要台架验证和标定的实验原型，不是已验证测量精度的医疗设备。修复了软件故障路径和测量算法，不代表现有阀门参数已经完成实物验证。

## 编译

- **Keil**：打开 `Project/SmartBP_STM32F103C8T6.uvprojx`，使用现有 ARMCC 5 工程，执行 **Rebuild all target files**，再下载新生成的 HEX。不要直接烧录仓库中旧的 `Project/Objects/*.hex`；它们是历史产物，本次未覆盖。
- **GNU Arm**：安装 `gcc-arm-none-eabi`、`binutils-arm-none-eabi` 和 newlib 后，运行 `bash tools/build-gcc.sh`。默认将 ELF、HEX、map 和栈使用信息写到 `/tmp/smartbp-gcc`，可用 `BUILD_DIR` 覆盖。
- 云端已验证 GNU Arm 完整编译、链接和 HEX 生成；没有执行 Windows Keil 编译或物理下载。

## 自动测试

在 Linux 上准备 GCC、Python 3：

```bash
bash tools/test.sh
```

包含合成脉搏回放、压力驱动故障注入和主状态机测试；C 测试启用 AddressSanitizer、UndefinedBehaviorSanitizer。通过测试不能替代真实压力标定。

## 调试

参见 [固件调试与验证](docs/固件调试与验证.md)，包括参数、错误码、串口输出、内存预算和上板顺序。旧技术方案的放气时序和采样描述已落后于代码，不能用作当前固件行为的依据。
