#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
// Windows x64 ABI
// Arguments are passed in %rcx (prev_sp) and %rdx (next_sp)
// Non-volatile registers: %rbp, %rbx, %rdi, %rsi, %r12, %r13, %r14, %r15
    .text
    .globl vortex_swap
vortex_swap:
    // Save current context to *prev_sp (in %rcx)
    pushq %rbp
    pushq %rbx
    pushq %rdi
    pushq %rsi
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    // Save RSP to *prev_sp
    movq %rsp, (%rcx)

    // Load RSP from *next_sp (in %rdx)
    movq (%rdx), %rsp

    // Restore next context
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rsi
    popq %rdi
    popq %rbx
    popq %rbp

    ret

#else
// System V AMD64 ABI (Linux/macOS)
// Arguments are passed in %rdi (prev_sp) and %rsi (next_sp)
// Non-volatile registers: %rbp, %rbx, %r12, %r13, %r14, %r15
    .text
    .globl vortex_swap
vortex_swap:
    // Save current context to *prev_sp (in %rdi)
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    // Save RSP to *prev_sp
    movq %rsp, (%rdi)

    // Load RSP from *next_sp (in %rsi)
    movq (%rsi), %rsp

    // Restore next context
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp

    ret

#endif
