
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
typedef uint64_t sys_handle_t;

int64_t sys_print(const char* str);
int64_t sys_create_thread(void *function, void *arg, uint64_t *out_thread_id);
void sys_exit(int64_t status);
void sys_exit_thread(void *result);
int64_t sys_wait_thread(uint64_t id, void **out_result);
void sys_yield();
uint64_t sys_get_thread_id();
pid_t sys_get_pid();
int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, pid_t *out_pid);

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
int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t irq);

typedef enum {
    PIO_SIZE_BYTE  = 0x0,
    PIO_SIZE_SHORT = 0x1,
    PIO_SIZE_INT   = 0x2,
} PORTIOSize;
uint32_t sys_port_io_in(uint16_t port, PORTIOSize size);
void sys_port_io_out(uint16_t port, PORTIOSize size, uint32_t out);

#define SYS_ERR_RPC_INVOKE_TOO_MANY_CALLS -4096
typedef struct {
    int64_t error_code;
    uint64_t result;
} rpc_result_t;
rpc_result_t sys_rpc_invoke(pid_t target, uint64_t call_number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
#define SYS_ERR_RPC_RETURN_PID_NOT_CALLER -4096
int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3);
int64_t sys_rpc_return(pid_t caller, uint64_t result);
uint64_t sys_rpc_awaken(uint64_t count);

int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address);

typedef uint64_t Sys_MemoryAccessFlags;
#define MMAP_ACCESS_READ 0x1
#define MMAP_ACCESS_WRITE 0x2
#define MMAP_ACCESS_EXEC 0x4
int64_t sys_memory_map(void **address, size_t length, Sys_MemoryAccessFlags access);

#define SYS_MUNMAP_SUCCESS_NOOP 1
typedef uint64_t Sys_MemoryUnmapFlags;
#define MUNMAP_FLAGS_INCLUSIVE 1
int64_t sys_memory_unmap(void *address, size_t length, Sys_MemoryUnmapFlags flags);

int64_t sys_memory_share_create(void **address, size_t length, Sys_MemoryAccessFlags access, sys_handle_t *out_handle);
int64_t sys_memory_share_map(sys_handle_t handle, void **address, Sys_MemoryAccessFlags access);
int64_t sys_memory_share_remove(sys_handle_t handle);

int64_t sys_port_create(sys_handle_t *out_handle);
#define IPC_MESSAGE_LIMIT 256
#define IPC_HANDLE_LIMIT 8
typedef struct {
    size_t size;
    uint8_t buffer[IPC_MESSAGE_LIMIT];
    size_t handle_count;
    sys_handle_t handles[IPC_HANDLE_LIMIT];
} sys_ipc_message_t;
int64_t sys_port_send(sys_handle_t port, sys_ipc_message_t *message);
#define SYS_ERR_PORT_NO_MESSAGE -4096
int64_t sys_port_receive(sys_handle_t port, sys_ipc_message_t *out_message, uint64_t wait);
int64_t sys_port_terminate(sys_handle_t port);
