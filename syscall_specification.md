# Syscalls

> Even if the return value is annotated as `void`, the syscall will always return a 64-bit integer.

## 0: int64_t sys_print(const char\* str)
Prints a string to the kernel debug log.

## 1: int64_t sys_create_thread(void \*function, void \*arg, uint64_t \*out_thread_id)
Creates a new thread that executes from the given address. Functions are expected to have the signature `void *function(void *arg)`. Returns -1 for failure and 0 for success. If function succeeds, then the thread's local id is written to the `out_thread_id` parameter. If `function` is an invalid address, then upon the switching to it for the first time will crash the application.

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

## 7: uint64_t sys_get_pid()
Returns the process id of the thread invoking the call.

## 8: int64_t sys_create_process(void \*content, size_t content_len, const char\* name, size_t name_len, uint64_t *out_pid)
Only reserved for the init process. Loads ELF executable from given memory and gives it the name provided. Returns negative values for errors and 0 for success. If success, the newly created process' pid is written to the `out_pid` parameter. If any of the pointers/ranges given (`content`, `name`, `out_pid`) are invalid, the function will return -1 with no changes.

## 9: int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector)
Configures how the kernel reacts to the specified interrupt at the given `vector`. The functionality of this syscall differs based on the `request` given, see below. If an unrecognized `request` is given, the function will return **SYS_ERR_IRQCTL_REQUEST_INVALID** immediately. The only accepted range the parameter `vector` can be is within `(0, 200)` (where `0` and `200` are not allowed). If this range is not respected, the function will return **SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE** immediately. The vector index is relative to where the PIC/LAPIC was remapped to. For example, using a vector of 14 will not override the kernel's page fault handler.

> *Developer's Note:* The number 200 was selected to allow headroom for the CPU exceptions (remap 32).

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
