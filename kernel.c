typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

//获取链接器脚本符号
extern char __bss[], __bss_end[], __stack_top[];

void *memset(void *buf, char c,size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

void kernel_main(void) {
    //初始化__bss段
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); 

    for(;;); //keep in this function
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