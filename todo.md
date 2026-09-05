
The kernel is desperately lacking some features/implementations/bugfixes as of with its current state.

- [x] Hash tables. `processes.c` and `shared_memory.c` need them.
- [x] Userspace process VAS tracking.
- [x] VMM needs a reframing for CR3 management (see `context_switching.c`)
- [x] APIC support. We're still using the PIC and the PIT.
- [x] `is_valid_mapped_user_range` still needs to check for range.
- [x] Directing thread execution to specific thread support. `rpc.c` needs it.
- [ ] RPC fastpath
- [ ] Kernel-side ELF loading still has issues. (will ELF loading ever transition to userspace?) (needs VAS)
- [ ] `sys_map_mmio` is unsafe. Very.
- [ ] A capability system (or rather any security enforcement system) is needed.
- [x] A global error ID system is needed.
- [ ] ACPI will require custom memory addresses for kernel processes. (And yes, ACPI needs to be a kernel-space process.)
- [ ] Perhaps add global error printing to userspace printf?

Known bug:
- [x] Interrupts don't keep track of user thread GS, only the kernel's! (Syscall entry/exit does it though. Probably need a central system?)
