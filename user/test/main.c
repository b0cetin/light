
#include <stdint.h>
#include <syscalls.h>

int main() {
    sys_print("This is a message from userspace.");

    return 0;
}
