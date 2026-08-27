
#pragma once

#include <stddef.h>
#include <stdint.h>

#define SYS_SUCCESS 0

typedef uint64_t pid_t;

int64_t sys_print(const char* str);
int64_t sys_create_thread(void *function, void *arg, uint64_t *out_thread_id);
void sys_exit(int64_t status);
void sys_exit_thread(void *result);
int64_t sys_wait_thread(uint64_t id, void **out_result);
void sys_yield();
uint64_t sys_get_thread_id();
uint64_t sys_get_pid();
int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, uint64_t *out_pid);

#define SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED -2
#define SYS_ERR_IRQCTL_AWAIT_DUPLICATE -3
#define SYS_ERR_IRQCTL_AWAIT_CANCELLED -4
#define SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED -5
#define SYS_ERR_IRQCTL_VECTOR_IN_USE -6
#define SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE -7
#define SYS_ERR_IRQCTL_REQUEST_INVALID -8
#define SYS_ERR_IRQCTL_UNSET_VECTOR_AWAITING -9
typedef enum {
    IRQCTL_SET    = 0x0,
    IRQCTL_AWAIT  = 0x1,
    IRQCTL_CANCEL = 0x2,
    IRQCTL_UNSET  = 0x3,
} IRQCTLRequest;
int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector);

typedef enum {
    PIO_SIZE_BYTE  = 0x0,
    PIO_SIZE_SHORT = 0x1,
    PIO_SIZE_INT   = 0x2,
} PORTIOSize;
uint32_t sys_port_io_in(uint16_t port, PORTIOSize size);
void sys_port_io_out(uint16_t port, PORTIOSize size, uint32_t out);

#define SYS_ERR_RPC_PID_NOT_FOUND -2
#define SYS_ERR_RPC_TOO_MANY_CALLS -3
#define SYS_ERR_RPC_PID_NOT_CALLER -4
#define SYS_ERR_RPC_CALLER_DEAD -5
typedef struct {
    int64_t error_code;
    uint64_t result;
} rpc_result_t;
rpc_result_t sys_rpc_invoke(pid_t target, uint64_t call_number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3);
int64_t sys_rpc_return(pid_t caller, uint64_t result);
uint64_t sys_rpc_awaken(uint64_t count);

#define SYS_ERR_MAP_MMIO_INVALID_RANGE -2
#define SYS_ERR_MAP_MMIO_USED -3
#define SYS_ERR_MAP_MMIO_ARG_UNALIGNED -4
#define SYS_ERR_MAP_MMIO_CANNOT_FIND_SPACE -5
int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address);

#define SYS_ERR_MMAP_INVALID_RANGE -2
#define SYS_ERR_MMAP_USED -3
#define SYS_ERR_MMAP_ARG_UNALIGNED -4
#define SYS_ERR_MMAP_CANNOT_FIND_SPACE -5
#define SYS_ERR_MMAP_INVALID_ACCESS -6
#define SYS_ERR_MMAP_OUT_OF_MEMORY -7
typedef uint64_t MemoryAccessFlags;
#define MMAP_ACCESS_READ 0x1
#define MMAP_ACCESS_WRITE 0x2
#define MMAP_ACCESS_EXEC 0x4
int64_t sys_memory_map(void **address, size_t length, MemoryAccessFlags access);

#define SYS_ERR_MSHARE_INVALID_RANGE -2
#define SYS_ERR_MSHARE_UNMAPPED -3
#define SYS_ERR_MSHARE_ARG_UNALIGNED -4
typedef uint64_t Sys_SharedMemoryID;
int64_t sys_memory_share(void *address, size_t length, Sys_SharedMemoryID *out_id);
#define SYS_ERR_MSHARE_USED -3
#define SYS_ERR_MSHARE_CANNOT_FIND_SPACE -5
int64_t sys_memory_share_map(Sys_SharedMemoryID id, void **address);
