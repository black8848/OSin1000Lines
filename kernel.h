#pragma once

// `fmt` 对应传入的参数如 PANIC("out of range");
// `, ##__VA_ARGS__` 如 PANIC("bad value: %d", value);
#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)


struct sbiret {
    long error;
    long value;
};