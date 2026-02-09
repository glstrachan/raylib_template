#pragma once

typedef unsigned long long __jmp_buf[8];
typedef __jmp_buf jmp_buf;
void __attribute__((naked)) my_longjmp(volatile __jmp_buf buf, int value);
int __attribute__((naked)) my_setjmp(volatile __jmp_buf buf);


#if defined(__x86_64__)
#define arch_get_stack_pointer(out_sp) asm volatile("mov %%rsp, %0" : "=r" (out_sp));
#define arch_set_stack_pointer(in_sp) asm volatile("mov %0, %%rsp" :: "r" (in_sp) : "rsp");
#elif defined(__arch64__)
#define arch_get_stack_pointer(out_sp) asm volatile("mov %0, sp" : "=r" (out_sp));
#define arch_set_stack_pointer(in_sp) asm volatile("mov sp, %0" :: "r" (in_sp) : "sp");
#else
#error "Unsupported arch"
#endif