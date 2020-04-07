    // // save user rsp in %r8 register, which is pushed to user stack
    // asm volatile(
    //     "\tpush %%r8\n"
    //     "\tmovq %%rsp,%%r8\n" ::
    //         : "memory");

    // // switch to kernel stack
    // asm volatile(
    //     "\tmovq %0,%%rsp\n" ::"g"(current->kstack + process::process_dispatcher::KERNSTACK_SIZE)
    //     : "memory");

    // // save states
    // asm volatile("\tpush %%r15\n"
    //              "\tpush %%r14\n"
    //              "\tpush %%r13\n"
    //              "\tpush %%r12\n"
    //              "\tpush %%r11\n"
    //              "\tpush %%r10\n"
    //              "\tpush %%r9\n"
    //              "\tpush %%r8\n"
    //              "\tpush %%rdi\n"
    //              "\tpush %%rsi\n"
    //              "\tpush %%rbp\n"
    //              "\tpush %%rdx\n"
    //              "\tpush %%rcx\n"
    //              "\tpush %%rbx\n"
    //              "\tpush %%rax\n" ::
    //                  : "memory");

    // // call the body of syscall
    // [[maybe_unused]] auto ret = syscall_body();

    // // restore states
    // asm volatile(
    //     "\tpop %%rax\n"
    //     "\tpop %%rbx\n"
    //     "\tpop %%rcx\n"
    //     "\tpop %%rdx\n"
    //     "\tpop %%rbp\n"
    //     "\tpop %%rsi\n"
    //     "\tpop %%rdi\n"
    //     "\tpop %%r8\n"
    //     "\tpop %%r9\n"
    //     "\tpop %%r10\n"
    //     "\tpop %%r11\n"
    //     "\tpop %%r12\n"
    //     "\tpop %%r13\n"
    //     "\tpop %%r14\n"
    //     "\tpop %%r15\n" ::
    //         : "memory");

    // // reload user rsp from %r8 register
    // asm volatile(
    //     "\tmovq %%r8,%%rsp\n" ::
    //         : "memory");

    // // restore %r8 from user stack
    // asm volatile(
    //     "\tpop %%r8\n" ::
    //         : "memory");

    // return to user mode

.global syscall_x64_entry
syscall_x64_entry:
    sysretq