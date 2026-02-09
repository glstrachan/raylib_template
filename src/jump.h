#pragma once

#if defined(__x86_64__) || defined(_M_X64)
typedef unsigned long long __jmp_buf[8];
typedef __jmp_buf jmp_buf;
#define arch_get_stack_pointer(out_sp) asm volatile("mov %%rsp, %0" : "=r" (out_sp));
#define arch_set_stack_pointer(in_sp) asm volatile("mov %0, %%rsp" :: "r" (in_sp) : "rsp");
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
typedef unsigned long long __jmp_buf[22];
typedef __jmp_buf jmp_buf;
#define arch_get_stack_pointer(out_sp) asm volatile("mov %0, sp" : "=r" (out_sp));
#define arch_set_stack_pointer(in_sp) asm volatile("mov sp, %0" :: "r" (in_sp) : "sp", "memory");
#else
#error "Unsupported arch"
#endif

void __attribute__((naked)) my_longjmp(volatile __jmp_buf buf, int value);
int __attribute__((naked)) my_setjmp(volatile __jmp_buf buf);
