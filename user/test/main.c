
int main() {
    // Trigger a system call
    asm volatile (
        "movq $67, %%rax\n"
        "syscall\n"
        :
        :
        : "rax", "rcx", "r11", "memory"
    );
}
