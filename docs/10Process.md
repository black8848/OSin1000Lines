## 最终目标


## 知识
1. 加括号就可以在sw和lw中（sw是寄存器存储到内存，lw是内存加载到寄存器），充当“内存”的角色

eg.

```c
"sw sp, (a0)\n" // 存储 sp 寄存器的值到内存 (a0)
"lw sp, (a1)\n" // 加载内存 (a1) 的地址到寄存器 sp 中
```

2. `[]`代表C的输入参数，，如下

```c
    __asm__ __volatile__(
        "csrw sscratch, %[sscratch]\n"
        :
        : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );
```

指的是将C参数[sscratch]，赋值给寄存器sscratch，且这个[sscratch]是即时定义的。