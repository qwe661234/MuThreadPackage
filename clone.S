#include <asm/unistd_64.h> // for syscall numbers

// We need to make a function callable from C that calls the clone syscall and
// and starts the user function on the new task. We do this in assembly
// to properly set up the call frame information (CFI) for debugging,
// backtraces, exceptions and such.
//
// The C call has the following format:
//
// int muclone(int (*fn)(void *arg), void *arg, int flags, void *child_stack
//             pid_t *parent_tidptr, pid_t *child_tidptr, void *tls),
//
// This results with the registers having the following values:
//
//   rdi: fn
//   rsi: arg
//   rdx: flags
//   rcx: child_stack
//   r8:  parent TID pointer
//   r9:  child  TID pointer
//   %rsp+8: TLS pointer
//
//  The syscall has the following interface:
//
//  SYSCALL_DEFINE5(clone,
//                  unsigned long, clone_flags,
//                  unsigned long, newsp,
//                  int __user *, parent_tidptr,
//                  int __user *, child_tidptr,
//                  unsigned long, tls)
//
//   So we need the registers to be:
//
//     rax: __NR_clone
//     rdi: flags
//     rsi: child_stack
//     rdx: parent TID pointer
//     r10: child TID pointer
//     r8:  TLS pointer
//
//------------------------------------------------------------------------------

  .text

  .global muclone           // sets the symbol as externally linkable
  .type   muclone,@function // declare this symbol as a function in ELF
  .align  16                // place the next instruction at an address
                            // divisible by 16 in the resulting ELF binary
muclone:
  .cfi_startproc            // cfi_* is the stuff for exception frames
  subq $16, %rcx            // decrement the new stack pointer by 16 because
                            // we will pop stuff from it later
  movq %rsi, 8(%rcx)        // store the function argument in the child's stack
  movq %rdi, 0(%rcx)        // store the function pointer in the child's stack

  movq $__NR_clone, %rax    // clone syscall number to rax
  movq %rdx, %rdi           // flags to rdi
  movq %rcx, %rsi           // child_stack to rsi
  movq %r8, %rdx            // parent_tid to rdx
  movq %r9, %r10            // child_tid to r10
  movq 8(%rsp), %r8         // TLS pointer to r8
  .cfi_endproc              // end FDE now to get proper unwind info in the
                            // child
  syscall                   // call sys_clone
  testq %rax, %rax          // compare %rax && %rax to zero
  jz .Lstart_thread         // 0 == we're in the child
  ret                       // we're in the parent with either an error or
                            // the child's TID

.Lstart_thread:             // ELF local label
  .cfi_startproc
  .cfi_undefined rip        // previous value of the instruction pointer cannot
                            // be restored anymore
  xorq %rbp, %rbp           // clear the frame pointer

  popq %rax                 // pop the function pointer
  popq %rdi                 // pop the argument
  call *%rax                // call the function

  movq %rax, %rdi           // whatever the function returned goes to rdi
  movq $__NR_exit, %rax     // exit syscall number to %rax
  syscall                   // call sys_exit
  .cfi_endproc