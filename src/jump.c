#include "dialog.h"

void __attribute__((naked)) my_longjmp(__jmp_buf buf, int value) {
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
	"mov 48(%rcx),%rsp\n"
	"jmp *56(%rcx)"       /* goto saved address without altering rsp */
);
}

int __attribute__((naked)) my_setjmp(__jmp_buf buf) {
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