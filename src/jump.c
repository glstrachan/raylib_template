#include "dialog.h"

#if defined(__x86_64__) || defined(_M_X64)

void __attribute__((naked)) my_longjmp(volatile __jmp_buf buf, int value) {
// asm volatile("lea (%%rip), %%rax\n push %%rax" ::: "rax", "memory");
asm volatile (
	"xor %eax,%eax\n"
	"cmp $1,%rdx\n"       /* CF = val ? 0 : 1 */
	"adc %edx,%eax\n"       /* eax = val + !val */
	"mov (%rcx),%rbx\n"       /* rcx is the jmp_buf, restore regs from it */
	"mov 8(%rcx),%rbp\n"
	"mov 16(%rcx),%r12\n"
	"mov 24(%rcx),%r13\n"
	"mov 32(%rcx),%r14\n"
	"mov 40(%rcx),%r15\n"
    // "pop %rdx\n"
	"mov 48(%rcx),%rsp\n"
    // "push %rdx\n"
	"jmp *56(%rcx)"       /* goto saved address without altering rsp */
);
}

int __attribute__((naked)) my_setjmp(volatile __jmp_buf buf) {
asm volatile (
	"mov %rbx,(%rcx)\n"    /* rcx is jmp_buf, move registers onto it */
	"mov %rbp,8(%rcx)\n"
	"mov %r12,16(%rcx)\n"
	"mov %r13,24(%rcx)\n"
	"mov %r14,32(%rcx)\n"
	"mov %r15,40(%rcx)\n"
	"lea 8(%rsp),%rdx\n"    /* this is our rsp WITHOUT current ret addr */
	"mov %rdx,48(%rcx)\n"
	"mov (%rsp),%rdx\n"    /* save return addr ptr for new rip */
	"mov %rdx,56(%rcx)\n"
	"xor %eax,%eax\n"           /* always return 0 */
	"ret"
    );
}

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)

void __attribute__((naked)) my_longjmp(volatile __jmp_buf buf, int value) {
// asm volatile("lea (%%rip), %%rax\n push %%rax" ::: "rax", "memory");
asm volatile (
	"ldp x19, x20, [x0,#0]\n"
	"ldp x21, x22, [x0,#16]\n"
	"ldp x23, x24, [x0,#32]\n"
	"ldp x25, x26, [x0,#48]\n"
	"ldp x27, x28, [x0,#64]\n"
	"ldp x29, x30, [x0,#80]\n"
	"ldr x2, [x0,#104]\n"
	"mov sp, x2\n"
	"ldp d8 , d9, [x0,#112]\n"
	"ldp d10, d11, [x0,#128]\n"
	"ldp d12, d13, [x0,#144]\n"
	"ldp d14, d15, [x0,#160]\n"
	"cmp w1, 0\n"
	"csinc w0, w1, wzr, ne\n"
	"br x30\n"
);
}

int __attribute__((naked)) my_setjmp(volatile __jmp_buf buf) {
asm volatile (
	"stp x19, x20, [x0,#0]\n"
	"stp x21, x22, [x0,#16]\n"
	"stp x23, x24, [x0,#32]\n"
	"stp x25, x26, [x0,#48]\n"
	"stp x27, x28, [x0,#64]\n"
	"stp x29, x30, [x0,#80]\n"
	"mov x2, sp\n"
	"str x2, [x0,#104]\n"
	"stp  d8,  d9, [x0,#112]\n"
	"stp d10, d11, [x0,#128]\n"
	"stp d12, d13, [x0,#144]\n"
	"stp d14, d15, [x0,#160]\n"
	"mov x0, #0\n"
	"ret\n"
    );
}
#else
#error "Unsupported arch"
#endif
