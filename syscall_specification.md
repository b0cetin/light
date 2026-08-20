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
