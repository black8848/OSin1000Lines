#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;


// SBI 调用：RISC-V 中 S 模式（内核模式）向 M 模式（固件/运行时/硬件/Bootloader/OpenSBI）请求服务的一种调用机制
// SBI 调用：S-mode 调用 ecall，硬件陷入 M-mode，M-mode 的 SBI 解析参数、执行对应功能、返回S-mode
// OpenSBI 对应 X86 平台的BIOS/UEFI

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

//获取链接器脚本符号
extern char __bss[], __bss_end[], __stack_top[];

void *memset(void *buf, char c,size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}


void kernel_main(void) {
    //初始化__bss段
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); 

    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    // const char *s = "\n\nHello World!\n";
    // for (int i = 0; s[i] != '\0'; i++) {
    //     putchar(s[i]);
    // }

    for(;;) {//keep in this function
        __asm__ __volatile__("wfi");
    }
}

__attribute__((section(".text.boot")))
//将boot函数放在0x80200000
__attribute__((naked))
//__attribute__((naked)) 属性告诉编译器不要在函数体前后生成不必要的代码，比如返回指令。这确保内联汇编代码就是确切的函数体。
//什么叫不要生成不必要的代码，如果不指定的话会生成什么代码？ fzzz（用于search）
void boot(void) { //在链接器脚本中指定为入口
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" //将stack pointer设置为栈的结束点
        "j kernel_main\n" //jump to kernel_main()
        :
        : [stack_top] "r" (__stack_top)
    );
}