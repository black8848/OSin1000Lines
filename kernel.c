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


__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        "csrw sscratch, sp\n"
        "addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        "mv a0, sp\n"
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

void kernel_main(void) {
    //初始化__bss段
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); 

    WRITE_CSR(stvec, (uint32_t) kernel_entry); 
    __asm__ __volatile__("unimp"); // unimp 是"伪指令"，含义是写入一个x0到一个只读寄存器来诱发异常

    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    // const char *s = "\n\nHello World!\n";
    // for (int i = 0; s[i] != '\0'; i++) {
    //     putchar(s[i]);
    // }

    // PANIC("booted!");
    // printf("unreachable here!\n");

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