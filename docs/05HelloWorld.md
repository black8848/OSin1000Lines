## 最终目标

实现 `printf` 函数

## 知识

1. SBI 是与内核硬件层的沟通 API
2. OpenSBI 相当于 X86 平台的 UEFI/BIOS，是启动系统的运行时
3. ecall 是 SBI 与 OpenSBI 之间调用的具体实现
4. 回忆一下，__asm__ __volatile__("assembly" : output operands : input operands : clobbered registers); 

    “=r 是输出操作数、 r 是输入操作数”，clobbered registers (破坏寄存器)的意思是这块区域会被汇编写入，编译器不应该再信任这块区域的值
    > 假如这里存放关键变量的值，但是汇编会写入这块区域，所以编译器不能再信任这块关键变量



```c
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

//回忆一下，__asm__ __volatile__("assembly" : output operands : input operands : clobbered registers); “=r 是输出操作数、 r 是输入操作数”
    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1) // 输出到 a0、a1 中
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7) // // 输入a0···a7
                         : "memory"); // 破坏寄存器的意思是：该块区域被汇编修改，编译器不可再信任这个区域的值。
    return (struct sbiret){.error = a0, .value = a1};
}

```

关于这段代码，并不是将 a0···a7 的值赋给 a0、a1，而是将 a0···a7 传递给 ecall，然后将内核传回的值写入error message a0 与 value a1 

```c
void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}
```

关于这个调用，fid = 0 eid = 1 => 对应内核的Putchar，代表将 char `ch` 打印出来

> |   功能   |   fid   |   eid   |
> | ---- | ---- | ---- |
> |   sbi_console_putchar   |   0   |   1   |
> |   sbi_console_getchar   |   0   |   2   |

`printf` 函数就是调用该功能打印出日志内容