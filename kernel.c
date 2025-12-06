#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

#define PROCS_MAX 8       // 最大进程数量
#define PROC_UNUSED   0   // 未使用的进程控制结构
#define PROC_RUNNABLE 1   // 可运行的进程

struct process {
    int pid;             // 进程 ID
    int state;           // 进程状态: PROC_UNUSED 或 PROC_RUNNABLE
    vaddr_t sp;          // 栈指针
    uint8_t stack[8192]; // 内核栈
};

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
extern char __free_ram[], __free_ram_end[];

paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

__attribute__((naked))
__attribute__((aligned(4)))
void kernel_entry(void) {
    __asm__ __volatile__(
        // 从sscratch中获取运行进程的内核栈
        "csrrw sp, sscratch, sp\n" // 将sscratch的原始值赋给sp，再将sp原来的值也赋给sscratch（就是交换值）
        //sp是异常发生时的指针，sscratch是进程内核栈的基址
        //交换sp和sscratch的值，现在sp是进程内核栈的基址，sscratch是异常时的指针
        //"csrw sscratch, sp\n"
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
        //将进程内核栈sp的寄存器全部保存

        // 获取并保存异常发生时的sp（sscratch's stack pointer）
        // 总结也就是将异常的指针sscratch放在4 * 30(sp)的位置上
        "csrr a0, sscratch\n" //将sscratch的值赋给a0
        "sw a0, 4 * 30(sp)\n" //将a0的值赋值给地址 4*30 + 内核栈基址sp

        // 重置内核栈
        // 总结就是将异常指针的值sscratch改为基址+4*31的地址值，即 sscratch 现在等于内核栈的顶端地址
        "addi a0, sp, 4 * 31\n" //a0 = sp + 4 * 31
        "csrw sscratch, a0\n" // sscratch = a0

        "mv a0, sp\n" //复制a0和sp，a0现在等于基址
        "call handle_trap\n" //传给 handle_trap

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

__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
    __asm__ __volatile__(
        // 将被调用者保存寄存器保存到当前进程的栈上
        "addi sp, sp, -13 * 4\n" // 为13个4字节寄存器分配栈空间 
        "sw ra,  0  * 4(sp)\n"   // 仅保存被调用者保存的寄存器
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // 切换栈指针
        "sw sp, (a0)\n"         // *prev_sp = sp;
        "lw sp, (a1)\n"         // 在这里切换栈指针(sp)

        // 从下一个进程的栈中恢复被调用者保存的寄存器
        "lw ra,  0  * 4(sp)\n"  // 仅恢复被调用者保存的寄存器
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  // 我们已从栈中弹出13个4字节寄存器
        "ret\n"
    );
}

struct process procs[PROCS_MAX]; // 所有进程控制结构
struct process *current_proc; // 当前运行的进程
struct process *idle_proc;    // 空闲进程

void yield(void) {
    // 搜索可运行的进程
    struct process *next = idle_proc; //定义寻找下一个可运行进程的结构体指针
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX]; //定义一个新的结构体指针，指向进程[(pid + i)%MAX]，库库就是找
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc; //找到了，下一个进程就是你啦！
            break;
        }
    }

    // 如果除了当前进程外没有可运行的进程，返回并继续处理
    if (next == current_proc) //最后查到了自己身上
        return;

    __asm__ __volatile__(
        "csrw sscratch, %[sscratch]\n"
        :
        : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)]) //将 next 的低层地址赋值给sscratch
    );

    // 上下文切换
    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}

struct process *create_process(uint32_t pc) {
    // 查找未使用的进程控制结构
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // 设置被调用者保存的寄存器。这些寄存器值将在 switch_context 
    // 中的第一次上下文切换时被恢复。
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) pc;          // ra

    // 初始化字段
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    return proc;
}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // 什么都不做
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        // switch_context(&proc_a->sp, &proc_b->sp);
        delay();
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        // switch_context(&proc_b->sp, &proc_a->sp);
        delay();
        yield();
    }
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

    paddr_t paddr0 = alloc_pages(2);
    paddr_t paddr1 = alloc_pages(1);
    printf("alloc_pages test: paddr0=%x\n", paddr0);
    printf("alloc_pages test: paddr1=%x\n", paddr1);

    idle_proc = create_process((uint32_t) NULL);
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);
    // proc_a_entry();

    yield();
    PANIC("switched to idle process");

    // WRITE_CSR(stvec, (uint32_t) kernel_entry); 
    // __asm__ __volatile__("unimp"); // unimp 是"伪指令"，含义是写入一个x0到一个只读寄存器来诱发异常

    // printf("\n\nHello %s\n", "World!");
    // printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

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