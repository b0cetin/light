
#pragma once

#include <stddef.h>
#include <stdint.h>

#define SYS_SUCCESS 0

#define SYS_ERR_ARGUMENT_POINTER_INVALID -1
#define SYS_ERR_INVALID_RANGE -2
#define SYS_ERR_ARGUMENT_UNALIGNED -3
#define SYS_ERR_CANNOT_FIND_SPACE -4
#define SYS_ERR_ARGUMENT_INVALID -5
#define SYS_ERR_OUT_OF_MEMORY -6
#define SYS_ERR_UNRESOLVED_PID -7
#define SYS_ERR_ADDRESS_RANGE_CLASH -8
#define SYS_ERR_INSUFFICIENT_PERMISSIONS -9
#define SYS_ERR_HANDLE_INVALID -10
#define SYS_ERR_HANDLE_DESTROYED -11

#define SYS_ERR_UNKNOWN_SYSCALL INT64_MIN

typedef uint64_t pid_t;
typedef uint64_t handle_t;

typedef struct {
    uint64_t rax, rdx;
} syscall_result;
extern syscall_result syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

static inline int64_t sys_print(const char* buf) {
    return syscall(0, (uint64_t) buf, 0, 0, 0, 0, 0).rax;
}

static inline int64_t sys_create_thread(void *function, void *arg, uint64_t *out_thread_id) {
    return syscall(1, (uint64_t) function, (uint64_t) arg, (uint64_t) out_thread_id, 0, 0, 0).rax;
}

static inline void sys_exit(int64_t status) {
    syscall(2, status, 0, 0, 0, 0, 0);
}

static inline void sys_exit_thread(void *result) {
    syscall(3, (uint64_t) result, 0, 0, 0, 0, 0);
}

static inline int64_t sys_wait_thread(uint64_t id, void **out_result) {
    return syscall(4, id, (uint64_t) out_result, 0, 0, 0, 0).rax;
}

static inline void sys_yield() {
    syscall(5, 0, 0, 0, 0, 0, 0);
}

static inline uint64_t sys_get_thread_id() {
    return syscall(6, 0, 0, 0, 0, 0, 0).rax;
}

static inline uint64_t sys_get_pid() {
    return syscall(7, 0, 0, 0, 0, 0, 0).rax;
}

static inline int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, uint64_t* out_pid) {
    return syscall(8, (uint64_t) content, (uint64_t) content_len, (uint64_t) name, (uint64_t) name_len, (uint64_t) out_pid, 0).rax;
}

#define SYS_ERR_IRQCTL_IRQ_NOT_RESERVED -4096
#define SYS_ERR_IRQCTL_AWAIT_DUPLICATE -4095
#define SYS_ERR_IRQCTL_AWAIT_CANCELLED -4094
#define SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED -4093
#define SYS_ERR_IRQCTL_IRQ_IN_USE -4092
#define SYS_ERR_IRQCTL_IRQ_OUT_OF_RANGE -4091
#define SYS_ERR_IRQCTL_UNSET_IRQ_AWAITING -4090
typedef enum {
    IRQCTL_SET    = 0x0,
    IRQCTL_AWAIT  = 0x1,
    IRQCTL_CANCEL = 0x2,
    IRQCTL_UNSET  = 0x3,
} IRQCTLRequest;
static inline int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t irq) {
    return syscall(9, request, irq, 0, 0, 0, 0).rax;
}

typedef enum {
    PIO_SIZE_BYTE  = 0x0,
    PIO_SIZE_SHORT = 0x1,
    PIO_SIZE_INT   = 0x2,
} PORTIOSize;
static inline uint32_t sys_port_io_in(uint16_t port, PORTIOSize size) {
    return syscall(10, port, size, 0, 0, 0, 0).rax;
}
static inline void sys_port_io_out(uint16_t port, PORTIOSize size, uint32_t out) {
    syscall(11, port, size, out, 0, 0, 0);
}

#define SYS_ERR_RPC_INVOKE_TOO_MANY_CALLS -4096
typedef struct {
    int64_t error_code;
    uint64_t result;
} rpc_result_t;
static inline rpc_result_t sys_rpc_invoke(pid_t target, uint64_t call_number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    // NOTE: Perhaps use own method to not use the stack?

    syscall_result ret = syscall(12, (uint64_t) target, (uint64_t) call_number, (uint64_t) arg0, (uint64_t) arg1, (uint64_t) arg2, (uint64_t) arg3);
    return (rpc_result_t) { .error_code = ret.rax, .result = ret.rdx };
}
#define SYS_ERR_RPC_RETURN_PID_NOT_CALLER -4096
static inline int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3) {
    return syscall(13, (uint64_t) out_caller, (uint64_t) out_call_number,
                   (uint64_t) out_arg0, (uint64_t) out_arg1, (uint64_t) out_arg2, (uint64_t) out_arg3).rax;
}
static inline int64_t sys_rpc_return(pid_t caller, uint64_t result) {
    return syscall(14, (uint64_t) caller, (uint64_t) result, 0, 0, 0, 0).rax;
}
static inline uint64_t sys_rpc_awaken(uint64_t count) {
    return syscall(15, count, 0, 0, 0, 0, 0).rax;
}


static inline int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address) {
    return syscall(16, physical_page_base, size, (uint64_t) virtual_address, 0, 0, 0).rax;
}


typedef uint64_t MemoryAccessFlags;
#define MMAP_ACCESS_READ 0x1
#define MMAP_ACCESS_WRITE 0x2
#define MMAP_ACCESS_EXEC 0x4
static inline int64_t sys_memory_map(void **address, size_t length, MemoryAccessFlags access) {
    return syscall(17, (uint64_t) address, length, access, 0, 0, 0).rax;
}

#define SYS_MUNMAP_SUCCESS_NOOP 1
typedef uint64_t MemoryUnmapFlags;
#define MUNMAP_FLAGS_INCLUSIVE 1
static inline int64_t sys_memory_unmap(void *address, size_t length, MemoryUnmapFlags flags) {
    return syscall(18, (uint64_t) address, length, flags, 0, 0, 0).rax;
}

static inline int64_t sys_memory_share_create(void **address, size_t length, MemoryAccessFlags access, handle_t *out_handle) {
    return syscall(19, (uint64_t) address, length, access, (uint64_t) out_handle, 0, 0).rax;
}
static inline int64_t sys_memory_share_map(handle_t handle, void **address, MemoryAccessFlags access) {
    return syscall(20, handle, (uint64_t) address, access, 0, 0, 0).rax;
}
static inline int64_t sys_memory_share_remove(handle_t handle) {
    return syscall(21, handle, 0, 0, 0, 0, 0).rax;
}
