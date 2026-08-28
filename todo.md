
The kernel is desperately lacking some features/implementations/bugfixes as of with its current state.

- [x] Hash tables. `processes.c` and `shared_memory.c` need them.
- [ ] Userspace process VAS tracking.
- [x] VMM needs a reframing for CR3 management (see `context_switching.c`)
- [ ] APIC support. We're still using the PIC and the PIT.
- [x] `is_valid_mapped_user_range` still needs to check for range.
- [ ] Directing thread execution to specific thread support. `rpc.c` needs it. Also, can `int 0x81` be called from userspace? It shouldn't.
- [ ] RPC fastpath
- [ ] Kernel-side ELF loading still has issues. (will ELF loading ever transition to userspace?) (needs VAS)
- [ ] `sys_map_mmio` is unsafe. Very.
- [ ] A capability system (or rather any security enforcement system) is needed.
