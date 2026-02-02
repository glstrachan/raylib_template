#pragma once

typedef unsigned long long __jmp_buf[8];
typedef __jmp_buf jmp_buf;
void __attribute__((naked)) my_longjmp(__jmp_buf buf, int value);
void __attribute__((naked)) my_longcall(__jmp_buf buf, int value);
int __attribute__((naked)) my_setjmp(__jmp_buf buf);