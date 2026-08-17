# Syscalls

> Even if the return value is annotated as `void`,
> the syscall will always return a 64-bit integer.

## 0: int64_t sys_print(const char* str)
Prints a string to the kernel debug log.

## 1: uint64_t sys_create_thread(void *function)
Creates a new thread that executes from the given address
and returns the local thread id. Functions are expected
to have the signature `void *function()`.

## 2: void sys_exit(int64_t status)
Terminates the whole process with given status code.

## 3: void sys_exit_thread(void *result)
Terminates the thread with the given result.

## 4: void *sys_wait_thread(uint64_t id)
Pauses thread execution to wait for another thread and
returns the result from that thread.

## 5: void sys_yield()
Pauses thread execution to yield control to another
thread within or out the current process.

## 6: uint64_t sys_get_thread_id()
Returns the local thread id of the thread invoking
the call.
