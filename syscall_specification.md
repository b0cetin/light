# Syscalls

As the light kernel is actively being developed, these syscalls may change at any time. Always compile your applications to the newest specification.

## Global Error Codes

These are error codes that all syscall's share. These start from *-1* and count down. Syscall-specific error codes start from *-4096* and count up.

* **SYS_ERR_ARGUMENT_POINTER_INVALID** -1: One or more pointer arguments given by the caller was invalid/not mapped.
* **SYS_ERR_INVALID_RANGE** -2: The range specified is not valid. (e.g. `address` -> `length`)
* **SYS_ERR_ARGUMENT_UNALIGNED** -3: An unaligned argument was provided for a parameter that expects a value aligned to the system's page size (*4096*).
* **SYS_ERR_CANNOT_FIND_SPACE** -4: The kernel cannot find a block large enough to fit what is requested. (e.g. a mapping in the process address space)
* **SYS_ERR_ARGUMENT_INVALID** -5: A generic catch-all for any problematic argument. (e.g. an unknown set flag)
* **SYS_ERR_OUT_OF_MEMORY** -6: There isnt't enough free memory in the system to perform the requested operation.
* **SYS_ERR_UNRESOLVED_PID** -7: Any given `pid_t` cannot be resolved to a running process.
* **SYS_ERR_ADDRESS_RANGE_CLASH** -8: The given requested range mapping clashes with a prior mapping.
* **SYS_ERR_INSUFFICIENT_PERMISSIONS** -9: The requested operation could not be allowed as the calling context is not authorized to do so.
* **SYS_ERR_HANDLE_INVALID** -10: A given handle either didn't exist or was pointing to an unexpected object.
* **SYS_ERR_HANDLE_DESTROYED** -11: A given handle no longer exists.

---

* **SYS_ERR_UNKNOWN_SYSCALL** *Minimum signed 64-bit integer*: The requested syscall is unknown.

## Types

* **pid_t**: type-alias `uint64_t`
* **handle_t**: type-alias `uint64_t`, `NULL_HANDLE` set to 0

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

## 9: int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t irq)
Configures how the kernel reacts to the specified interrupt request at the given `irq`. The functionality of this syscall differs based on the `request` given, see below. If an unrecognized `request` is given, the function will return **SYS_ERR_INVALID_ARGUMENT** immediately. The only accepted range the parameter `irq` starts from 0 and expands up to the amount of I/O APIC relocation entries on the system, which can be as low as (and usually is) 24. If this range is not respected, the function will return **SYS_ERR_IRQCTL_IRQ_OUT_OF_RANGE** immediately.

### enum IRQCTLRequest

1. **IRQCTL_SET (0x0)**: If the interrupt request is in use by another process (or even the calling process), then returns **SYS_ERR_IRQCTL_IRQ_IN_USE**. If not, the interrupt, if applicable, will be reserved to the calling process. If for any reason the kernel can't reserve this interrupt for the process, then the function will return -1. Being "reserved" doesn't mean that the interrupt control has been diverted to the process immediately. That functionality is reserved to **IRQCTL_AWAIT**. If the interrupt has been reserved successfully, then the function will return SYS_SUCCESS. If the process is terminated without unreserving the interrupt request, the kernel will forcefully unreserve it. It is always recommended to keep track of your reserved interrupt requests.

2. **IRQCTL_AWAIT (0x1)**: Pauses thread execution until the interrupt request is received. It is recommended to make this request on another thread created specifically for this interrupt. If this request is called on an unreserved interrupt request, then the function will return **SYS_ERR_IRQCTL_IRQ_NOT_RESERVED** immediately. The function will return SYS_SUCCESS if thread execution had been paused and now is unpaused due to the interrupt being triggered. An interrupt can only be awaited once at a time within a process. It is also important to note that the EOI signal will be delivered just before resuming the invoking thread. *(important: see **IRQCTL_CANCEL**)*

3. **IRQCTL_CANCEL (0x2)**: Forcefully terminates the await being performed for the specified interrupt request. The IRQCTL_AWAIT call will return **SYS_ERR_IRQCTL_AWAIT_CANCELLED**. As IRQCTL_AWAIT blocks the invoking thread, naturally IRQCTL_CANCEL can only be called from another thread. If the requested interrupt request is not reserved, **SYS_ERR_IRQCTL_IRQ_NOT_RESERVED** is returned. If the requested interrupt request is not being actively awaited, **SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED** is returned.

4. **IRQCTL_UNSET (0x3)**: Will unreserve the interrupt request if reserved for and not being awaited by this process and return SYS_SUCCESS. If the interrupt request is not reserved, returns **SYS_ERR_IRQCTL_IRQ_NOT_RESERVED**. If the interrupt request is actively being awaited, then returns **SYS_ERR_IRQCTL_UNSET_IRQ_AWAITING**.

### Error Codes

* **SYS_ERR_IRQCTL_IRQ_NOT_RESERVED (-4096)**: Indicates that the requested interrupt request has not been reserved for the calling process.

* **SYS_ERR_IRQCTL_AWAIT_DUPLICATE (-4095)**: Indicates that the interrupt request to be awaited is already being awaited in the process.

* **SYS_ERR_IRQCTL_AWAIT_CANCELLED (-4094)**: Indicates that the await ended prematurely because of a call using IRQCTL_CANCEL.

* **SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED (-4093)**: Indicates that the interrupt request is currently not being awaited within the process.

* **SYS_ERR_IRQCTL_IRQ_IN_USE (-4092)**: Indicates that the requested interrupt request is currently reserved for another process.

* **SYS_ERR_IRQCTL_IRQ_OUT_OF_RANGE (-4091)**: Indicates that the requested interrupt request is not within the allowed limits.

* **SYS_ERR_IRQCTL_UNSET_IRQ_AWAITING (-4090)**: Indicates that the requested interrupt request cannot be unset because an IRQCTL_AWAIT operation is underway.

* SYS_ERR_INVALID_ARGUMENT for `request`.

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

* **SYS_ERR_RPC_INVOKE_TOO_MANY_CALLS (-4096)**: The process tried to make a second call to the target process before the target process replied to the previous call.

## 13: int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3)
Returns **SYS_ERR_ARGUMENT_POINTER_INVALID** if the given pointers are invalid. Pauses thread execution until a **sys_rpc_invoke** is called for this process. The thread cannot be resumed for any other reason. Can be called by multiple threads, and only one will receive the RPC in a *oldest created thread to newest created thread* fashion.

## 14: int64_t sys_rpc_return(pid_t caller, uint64_t result)
Execution immediately returns back to the calling thread (which therefore pauses the execution of the calling thread), with the given result being passed to it. Replying to the `sys_rpc_awaken` syscall is no-op and returns **SYS_ERR_RPC_PID_NOT_CALLER**.

### Error Codes

2. **SYS_ERR_RPC_RETURN_PID_NOT_CALLER (-4096)**: The process tried to reply to a process that didn't make an RPC to it.

## 15: uint64_t sys_rpc_awaken(uint64_t count)
Resumes `count` number of threads waiting with `sys_rpc_receive`. The call number passed into the receive call is `UINT64_MAX`, with args being `0` and `out_caller` being the current process. `count` argument is not limited in any way. The syscall returns the amount of receive calls successfully awakened. `sys_rpc_return` for this syscall will return **SYS_ERR_RPC_PID_NOT_CALLER**.

> Developer's FIXME: Why does it return `SYS_ERR_RPC_PID_NOT_CALLER`? Isn't `SYS_SUCCESS` preferred?

## 16: int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address)
Tries to map the continuous physical memory to the given virtual address in a continuous way. If the data at `virtual_address` is `0`, then the kernel will pick an unused location and write out the selected location to the address specified by the pointer. If `virtual_address` points to an invalid region of memory and is not `0`, then the call with return **SYS_ERR_ARGUMENT_POINTER_INVALID** immediately. All arguments must be aligned to the system's page size (which is *4 KiB*.) The mapping will be declared in a way that skips the processor cache.

## 17: int64_t sys_memory_map(void \*\*address, size_t length, MemoryAccessFlags access)
Locates enough free pages in memory and maps them to the requested address in a continious way with the requested protection flags. If the value at given `address` is `0`, then the kernel will pick a suitable location in the caller's address space and write this address to the `address` argument. The length (and address if provided) must be aligned to the system's page size. If the argument `address` itself is not a valid mapped address in the caller's address space, the call will return **SYS_ERR_ARGUMENT_POINTER_INVALID** immediately.

### Type-alias MemoryAccessFlags: uint64_t
A 64-bit bitmask field defining the memory protection info for the mapping.

1. **MMAP_ACCESS_READ (0x1)**: Allows read.
1. **MMAP_ACCESS_WRITE (0x2)**: Allows write.
1. **MMAP_ACCESS_EXEC (0x4)**: Allows code execution.

## 18: int64_t sys_memory_unmap(void \*address, size_t length, MemoryUnmapFlags flags)
Looks at the range specified and unmaps all the memory mapped inside. This can include multiple pages. This function cannot divide memory regions. This call can only free mappings mapped per the process' request prior, as such, a process cannot nuke their address space with this call. This function does allow unaligned values. Unmapped regions are skipped.

### Type-alias MemoryUnmapFlags: uint64_t
A 64-bit bitmask field defining additional functionality for the unmapping.

2. **MUNMAP_FLAGS_INCLUSIVE (1):** The function will unmap any region that intersects with the range, not just the ones fully covered.

### Success Codes

Any return code above or equal to 0 is success, and means at least one region was unmapped. A 63-bit (last bit reserved for two's complement) bitmask is returned to report additional information about the operation.

2. **SYS_MUNMAP_SUCCESS_NOOP (1):** While there were no errors, no regions could be found and unmapped during the call, making the call no-op.

## 19: int64_t sys_memory_share_create(void \*\*address, size_t length, MemoryAccessFlags access, handle_t \*out_handle)
Performs `sys_memory_map`, and outputs the handle. The shared memory will not be destroyed until all handles have been destroyed and all mappings have been unmapped. The out argument `out_handle` is expected to be valid memory.

## 20: int64_t sys_memory_share_map(handle_t handle, void \*\*address, MemoryAccessFlags access)
Resolves the shared memory handle and maps it to the given address. If the value at `address` is `0`, then the kernel will pick a suitable location and write it out. If a value is provided, the value at `address` must be aligned to the system page size. Returns the amount of bytes mapped if successful. If `address` is invalid, **SYS_ERR_ARGUMENT_POINTER_INVALID** will be returned immediately.

## 21: int64_t sys_memory_share_remove(handle_t handle)
Removes the shared memory handle from the caller process' handle table. This operation does not unmap the memory, and per **sys_memory_share_map**'s documentation, for the memory to be invalid the caller must unmap it as well.

## 22: int64_t sys_port_create(handle_t *out_handle)
Creates an IPC port belonging to the calling process.

## 23: int64_t sys_port_send(handle_t port, IPCMessage *message)
Sends an IPC message to the port. If *fastpath* isn't available, the kernel will copy the buffer into kernel memory, and then write it out to the receiver's memory. The `size` value dictates how many bytes are copied, and whether *fastpath* is available. The `handles` array is expected to be a linear array without holes in it, and the `handle_count` value is respected, and not implicitly assumed. Every handle will be copied and transfered to the receiving process. The sender's handles are left intact.

> Developer's Note: The kernel will free the buffer once the message is received.

### struct IPCMessage
```c
#define IPC_MESSAGE_LIMIT 256
#define IPC_HANDLE_LIMIT 8

struct IPCMessage {
    size_t size;
    uint8_t buffer[IPC_MESSAGE_LIMIT];
    size_t handle_count;
    handle_t handles[IPC_HANDLE_LIMIT];
};
```

### Error Codes

* Beware for **SYS_ERR_OUT_OF_MEMORY**, as if the port exhausted its message queue limit, this error code will be returned.
* Beware for **SYS_ERR_HANDLE_DESTROYED**, as if the port was terminated before/while sending, this error code will be returned.

## 24: int64_t sys_port_receive(handle_t port, IPCMessage *out_message, uint64_t wait)
Receives a queued message from the port, and writes it out to `out_message`. The buffer is copied inside the message. If there is no message pending, but `wait` is set (>0) the thread execution will be paused until a message is received. The syscall will not touch the given `out_message` if there is no message and/or the port was terminated. A port is allowed a message queue to 8960 bytes (10 full-sized messages), before being refused to add more.

### Error Codes

* **SYS_ERR_PORT_NO_MESSAGE (-4096)**: No message was outputted.
* Beware for **SYS_ERR_HANDLE_DESTROYED**, as if the port was terminated while waiting, this error code will be returned.

## 25: int64_t sys_port_terminate(handle_t port)
Clears all messages from a port and shuts it down. This operation is only permitted to the process which created the port. If the process terminates before calling this syscall, the port will be closed shut nevertheless.
