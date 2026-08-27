# Syscalls

As the light kernel is actively being developed, these syscalls may change at any time. Always compile your applications to the newest specification.

## Types

* **pid_t**: type-alias `uint64_t`

## 0: int64_t sys_print(const char\* str)
Prints a string to the kernel debug log.

## 1: int64_t sys_create_thread(void \*function, void \*arg, uint64_t \*out_thread_id)
Creates a new thread that executes from the given address. Functions are expected to have the signature `void *function(void *arg)`. Returns -1 for failure and 0 for success. If function succeeds, then the thread's local id is written to the `out_thread_id` parameter, but only if the parameter isn't null. If `function` is an invalid address, then upon the switching to it for the first time will crash the application.

> Optional parameter: `out_thread_id`

## 2: void sys_exit(int64_t status)
Terminates the whole process with given status code.

## 3: void sys_exit_thread(void \*result)
Terminates the thread with the given result.

## 4: int64_t sys_wait_thread(uint64_t id, void \*\*out_result)
Pauses thread execution to wait for another thread and writes the result of the thread into the `out_result` parameter. As thread IDs are local per process, only threads within the calling process can be waited. Returns -1 for failures and 0 for successes.

## 5: void sys_yield()
Pauses thread execution to yield control to another thread within or out the current process.

## 6: uint64_t sys_get_thread_id()
Returns the local thread id of the thread invoking the call.

## 7: pid_t sys_get_pid()
Returns the process id of the thread invoking the call.

## 8: int64_t sys_create_process(void \*content, size_t content_len, const char\* name, size_t name_len, pid_t *out_pid)
Only reserved for the init process. Loads ELF executable from given memory and gives it the name provided. Returns negative values for errors and 0 for success. If success, the newly created process' pid is written to the `out_pid` parameter. If any of the pointers/ranges given (`content`, `name`, `out_pid`) are invalid, the function will return -1 with no changes.

## 9: int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector)
Configures how the kernel reacts to the specified interrupt at the given `vector`. The functionality of this syscall differs based on the `request` given, see below. If an unrecognized `request` is given, the function will return **SYS_ERR_IRQCTL_REQUEST_INVALID** immediately. The only accepted range the parameter `vector` can be is within `(0, 15]` (where `0` is not allowed) and not `2`. If this range is not respected, the function will return **SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE** immediately. The vector index is relative to where the PIC was remapped to. For example, using a vector of 14 will not override the kernel's page fault handler.

> *Developer's Note:* The range constraints were selected that way because that's all the PIC can offer. When APIC is implemented, this syscall will need revision.

### enum IRQCTLRequest

1. **IRQCTL_SET (0x0)**: If the interrupt vector is in use by another process (or even the calling process), then returns **SYS_ERR_IRQCTL_VECTOR_IN_USE**. If not, the interrupt, if applicable, will be reserved to the calling process. If for any reason the kernel can't reserve this interrupt for the process, then the function will return -1. Being "reserved" doesn't mean that the interrupt control has been diverted to the process immediately. That functionality is reserved to **IRQCTL_AWAIT**. If the interrupt has been reserved successfully, then the function will return SYS_SUCCESS. If the process is terminated without unreserving the interrupt, the kernel will forcefully unreserve it. It is always recommended to keep track of your reserved interrupt vectors.

2. **IRQCTL_AWAIT (0x1)**: Pauses thread execution until the interrupt is delivered. It is recommended to make this request on another thread created specifically for this interrupt. If this request is called on an unreserved interrupt vector, then the function will return **SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED** immediately. The function will return SYS_SUCCESS if thread execution had been paused and now is unpaused due to the interrupt being triggered. An interrupt can only be awaited once at a time within a process. It is also important to note that the EOI signal will be delivered just before resuming the invoking thread. *(important: see **IRQCTL_CANCEL**)*

3. **IRQCTL_CANCEL (0x2)**: Forcefully terminates the await being performed for the specified interrupt vector. The IRQCTL_AWAIT call will return **SYS_ERR_IRQCTL_AWAIT_CANCELLED**. As IRQCTL_AWAIT blocks the invoking thread, naturally IRQCTL_CANCEL can only be called from another thread. If the requested vector is not reserved, **SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED** is returned. If the requested vector is not being actively awaited, **SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED** is returned.

4. **IRQCTL_UNSET (0x3)**: Will unreserve the interrupt vector if reserved for and not being awaited by this process and return SYS_SUCCESS. If the vector is not reserved, returns **SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED**. If the vector is actively being awaited, then returns **SYS_ERR_IRQCTL_UNSET_VECTOR_AWAITING**.

### Error Codes

1. **Generic (-1)**: Unspecified error.

2. **SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED (-2)**: Indicates that the requested vector has not been reserved for the calling process.

3. **SYS_ERR_IRQCTL_AWAIT_DUPLICATE (-3)**: Indicates that the vector to be awaited is already being awaited in the process.

4. **SYS_ERR_IRQCTL_AWAIT_CANCELLED (-4)**: Indicates that the await ended prematurely because of a call using IRQCTL_CANCEL.

5. **SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED (-5)**: Indicates that the requested vector is currently not being awaited within the process.

6. **SYS_ERR_IRQCTL_VECTOR_IN_USE (-6)**: Indicates that the requested vector is currently reserved for another process.

7. **SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE (-7)**: Indicates that the requested vector is not within the allowed limits.

8. **SYS_ERR_IRQCTL_REQUEST_INVALID (-8)**: Indicates that the request code is unrecognized.

9. **SYS_ERR_IRQCTL_UNSET_VECTOR_AWAITING (-9)**: Indicates that the requested vector cannot be unset because an IRQCTL_AWAIT operation is underway.

## 10: uint32_t sys_port_io_in(uint16_t port, PORTIOSize size)

Reads the processor port at `port` and then zero-extends the result to 32-bits if the requested size is too small. Returns 0 if an invalid `size` is given. *(see: **PORTIOSize**)*

## 11: void sys_port_io_out(uint16_t port, PORTIOSize size, uint32_t out)

Writes out the value at the given `size` to the `port`. If `size` is not **PIO_SIZE_INT**, the function will discard the upper bits. Beware for signed integers. Does nothing if an invalid `size` is given.

### enum PORTIOSize

1. **PIO_SIZE_BYTE (0x0)**: Uses `inb/outb`.

2. **PIO_SIZE_SHORT (0x1)**: Uses `inw/outw`.

3. **PIO_SIZE_INT (0x2)**: Uses `inl/outl`.

## 12: rpc_result_t sys_rpc_invoke(pid_t target, uint64_t call_number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
Tries to call the target process' RPC handler with given arguments immediately. This pauses thread execution until a reply is given. This call will only work if the target process is actively receiving RPCs. If the call is successful (the syscall returns **SYS_SUCCESS**), the result of the RPC will be written back to the address at `out_result`. (If the pointers are invalid, the syscall will fail with -1.)

> Developer's Note: More info about how the call failed is not provided to the caller for security - as this may expose the state of the target process.

> Developer's FIXME: While this syscall was designed for *fastpath*, in the actual implementation, this is not yet the case. Only *slowpath* is implemented.

### struct rpc_result_t

```c
struct rpc_result_t {
    int64_t error_code;
    uint64_t result;
}
```

### Error Codes

1. **Generic (-1)**: Unspecified error.

2. **SYS_ERR_RPC_PID_NOT_FOUND (-2)**: The target process cannot be resolved from the provided pid.

3. **SYS_ERR_RPC_TOO_MANY_CALLS (-3)**: The process tried to make a second call to the target process before the target process replied to the previous call.

## 13: int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3)
Returns -1 if the given pointers are invalid. Pauses thread execution until a **sys_rpc_invoke** is called for this process. The thread cannot be resumed for any other reason. Can be called by multiple threads, and only one will receive the RPC in a *oldest created thread to newest created thread* fashion.

## 14: int64_t sys_rpc_return(pid_t caller, uint64_t result)
Execution immediately returns back to the calling thread (which therefore pauses the execution of the calling thread), with the given result being passed to it. Replying to the `sys_rpc_awaken` syscall is no-op and returns **SYS_ERR_RPC_PID_NOT_CALLER**.

### Error Codes

1. **Generic (-1)**: Unspecified error.

2. **SYS_ERR_RPC_PID_NOT_CALLER (-4)**: The process tried to reply to a process that didn't make an RPC to it.

3. **SYS_ERR_RPC_CALLER_DEAD (-5)**: The process replied to a call from a process that no longer exists. This isn't necessarily an error on the responder's part.

## 15: uint64_t sys_rpc_awaken(uint64_t count)
Resumes `count` number of threads waiting with `sys_rpc_receive`. The call number passed into the receive call is `UINT64_MAX`, with args being `0` and `out_caller` being the current process. `count` argument is not limited in any way. The syscall returns the amount of receive calls successfully awakened. `sys_rpc_return` for this syscall will return **SYS_ERR_RPC_PID_NOT_CALLER**.

> Developer's FIXME: Why does it return `SYS_ERR_RPC_PID_NOT_CALLER`? Isn't `SYS_SUCCESS` preferred?

## 16: int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address)
Tries to map the continuous physical memory to the given virtual address in a continuous way. If the data at `virtual_address` is `0`, then the kernel will pick an unused location and write out the selected location to the address specified by the pointer. If `virtual_address` points to an invalid region of memory and is not `0`, then the call with return -1 immediately. All arguments must be aligned to the system's page size (which is *4 KiB*.) The mapping will be declared in a way that skips the processor cache.

### Error Codes

1. **Generic (-1)**: Unspecified error.

2. **SYS_ERR_MAP_MMIO_INVALID_RANGE (-2)**: The range given with `physical_page_base` and `size` was invalid.

3. **SYS_ERR_MAP_MMIO_USED (-3)**: The provided range (`virtual_address` and `size`) is fully/partially already mapped into the calling process.

4. **SYS_ERR_MAP_MMIO_ARG_UNALIGNED (-4)**: One or more arguments weren't aligned to the system's page size.

5. **SYS_ERR_MAP_MMIO_CANNOT_FIND_SPACE (-5)**: A continuous space large enough to fit the desired size into the calling process' address space cannot be found. This error can only be encountered when `virtual_address` is `0`.

## 17: **RESERVED for sys_unmap_mmio or similar.**

## 18: int64_t sys_memory_map(void \*\*address, size_t length, MemoryAccessFlags access)
Locates enough free pages in memory and maps them to the requested address in a continious way with the requested protection flags. If the value at given `address` is `0`, then the kernel will pick a suitable location in the caller's address space and write this address to the `address` argument. The length (and address if provided) must be aligned to the system's page size. If the argument `address` itself is not a valid mapped address in the caller's address space, the call will return -1 immediately.

### Type-alias MemoryAccessFlags: uint64_t
A 64-bit bitmask field defining the memory protection info for the mapping.

1. **MMAP_ACCESS_READ (0x1)**: Allows read.
1. **MMAP_ACCESS_WRITE (0x2)**: Allows write.
1. **MMAP_ACCESS_EXEC (0x4)**: Allows code execution.

### Error Codes

1. **Generic (-1):** Unspecified error.

2. **SYS_ERR_MMAP_INVALID_RANGE (-2):** The range specified with `address` and `length` is not valid.

3. **SYS_ERR_MMAP_USED (-3):** The provided range (`address` and `size`) is fully/partially already mapped into the calling process.

4. **SYS_ERR_MMAP_ARG_UNALIGNED (-4)**: One or more arguments weren't aligned to the system's page size.

5. **SYS_ERR_MMAP_CANNOT_FIND_SPACE (-5)**: A continuous space large enough to fit the desired size into the calling process' address space cannot be found. This error can only be encountered when the value at `address` is `0`.

5. **SYS_ERR_MMAP_INVALID_ACCESS (-6)**: `access` features unknown/unsupported flags.

5. **SYS_ERR_MMAP_OUT_OF_MEMORY (-7)**: The system doesn't have enough free memory to complete this operation.

## 19: **RESERVED for int64_t sys_memory_unmap(void \*address, size_t length)**

## 20: int64_t sys_memory_share(void \*address, size_t length, SharedMemoryID *out_id)
Marks the region of memory as shared and outputs the newly shared memory ID. Arguments must be aligned to the system's page size. If `out_id` is invalid, returns -1 immediately. The memory region inputted must be mapped beforehand with `sys_memory_map`. The shared memory will not be destroyed until all processes referencing it are terminated or have unmapped it. Performing this syscall back to back with the same range will output the same `SharedMemoryID`.

> Developer's FIXME: No. It will not output the same SharedMemoryID. There is no VAS tracking. We need a better system.

### Type-alias SharedMemoryID: uint64_t
A global system identifier for a shared memory region. Not presistent.

### Error Codes

1. **Generic (-1):** Unspecified error.

2. **SYS_ERR_MSHARE_INVALID_RANGE (-2):** The range specified with `address` and `length` is not valid.

3. **SYS_ERR_MSHARE_UNMAPPED (-3):** The range specified with `address` and `length` isn't fully mapped.

4. **SYS_ERR_MSHARE_ARG_UNALIGNED (-4):** One or more arguments weren't aligned to the system's page size.

## 21: int64_t sys_memory_share_map(SharedMemoryID id, void \*\*address)
Resolves the shared memory reference and maps it to the given address. If the value at `address` is `0`, then the kernel will pick a suitable location and write it out. If a value is provided, the value at `address` must be aligned to the system page size. Returns the amount of bytes mapped if successful. If `address` is invalid, -1 will be returned immediately.

### Error Codes

1. **Generic (-1):** Unspecified error.

2. **SYS_ERR_MSHARE_INVALID_RANGE (-2):** The address given in `address` (or the range with the implicit size of the shared memory region) is not valid.

3. **SYS_ERR_MSHARE_USED (-3):** The address given in `address` (or the range with the implicit size of the shared memory region) clashes with already mapped memory.

4. **SYS_ERR_MSHARE_ARG_UNALIGNED (-4)**

5. **SYS_ERR_MSHARE_CANNOT_FIND_SPACE (-5)**: A continuous space large enough to fit the shared size into the calling process' address space cannot be found. This error can only be encountered when the value at `address` is `0`.
