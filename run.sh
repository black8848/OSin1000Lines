#!/bin/bash
set -xue

# QEMU 文件路径
QEMU=qemu-system-riscv32

# clang 路径和编译器标志
CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"
# -std=c11	使用 C11。
# -O2	启用优化以生成高效的机器代码。
# -g3	生成最大量的调试信息。
# -Wall	启用主要警告。
# -Wextra	启用额外警告。
# --target=riscv32-unknown-elf	编译为 32 位 RISC-V。
# -ffreestanding	不使用主机环境（你的开发环境）的标准库。
# -fuse-ld=lld	使用 LLVM linker（ld.lld）。
# -fno-stack-protector	禁用栈保护功能（#31 参考)
# -nostdlib	不链接标准库。
# -Wl,-Tkernel.ld	指定链接器脚本。
# -Wl,-Map=kernel.map	输出映射文件（链接器分配结果）。
# -Wl, 表示将选项传递给链接器而不是 C 编译器。clang 命令执行 C 编译并在内部执行链接器。

# 构建内核
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c

# 启动 QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf

# QEMU 为开源的虚拟化工具，用于运行虚拟机
# -machine virt：启动一个 virt 机器。你可以用 -machine '?' 选项查看其他支持的机器。
# -bios default：使用默认固件（在本例中是 OpenSBI）。
# -nographic：启动 QEMU 时不使用 GUI 窗口。
# -serial mon:stdio：将 QEMU 的标准输入/输出连接到虚拟机的串行端口。指定 mon: 允许通过按下 Ctrl+A 然后 C 切换到 QEMU 监视器。
# --no-reboot：如果虚拟机崩溃，停止模拟器而不重启（对调试有用）。