
# light OS

light is a hobby from-scratch 64-bit OS designed to run on the x86_64 processor architecture using UEFI. This repository is a monorepo for the entire OS, containing kernel, bootloader, and userspace programs.

## Developing & Building

The OS dev environment is structured inside a Docker devcontainer (see `.devcontainer/`). I personally use the "Dev Containers" extension with Visual Studio Code to develop the project. Setting the container up for the first time will take a long time as it has to: A) Download and make EFI libraries and linker target. B) Download and make a bare `x86_64-elf` cross-compiler (gcc, binutils, gdb.)

The tasks and launch options (with extension settings for the "clangd" extension) for Visual Studio Code will already be set up for you.

On the host system, `qemu-system-x86_64` is needed. The `run.sh` launch script utilizes it. For the devcontainer to launch the script, `mac-host-runner.sh` needs to be running in the background for it to pick up the launch request. This gets to the platform support of the dev environment.

**Platform Support**

| | macOS | Linux | Windows |
| --- | --- | --- | --- |
| devcontainer | ✅ | ✅ | ✅ |
| Run script | ✅ `run.sh` | ✅ `run.sh` | ❌ |
| Host runner service | ✅ `mac-host-runner.sh` | ❌ | ❌ |

After getting into the devcontainer, run `bear -- make all` (`all` is necessary) to generate `compile_commands.json`.

Running `make run` inside the devcontainer will compile the OS and trigger the run script, regardless of the editor.

### QEMU and EDK II

As mentioned above, the x86_64 target of QEMU is needed for the run script to work. In addition, the run script loads UEFI code and variables from under the (gitignored, create it yourself) `qemu` directory: `edk2-x86_64-code.fd`, `ovmf-vars.fd`. On macOS, these files come prebuilt and ready with QEMU on Homebrew. On Linux, some distros ship it with the system and some don't, you can look up what's the case for your own distro. On Windows... no clue. Don't forget to *copy* and *not move* these files into the project.

### Building from the command line

You'll still need Docker, or any other implementation compatible with it. Running `make host-build` from the host machine will set the container up, run `make build` inside it, and then leave it as that. You'll need to learn a little about Docker if you want to then remove the container from your system.

**NOTE:** The build system currently does not support packaging ISOs. Only `run.sh` will work, as the executablr files will just be laid bare on your filesystem, under `dist`.

## Project Layout

The OS has three sections: **Bootloader `bootloader`**, **the light kernel `kernel`**, and the **userspace programs & libraries `user`**.

### Bootloader

The bootloader is nothing special. It is an increadibly basic EFI OS loader. It has 5 jobs:

1. Load the kernel and userspace boot modules into memory.
2. Parse the kernel ELF file.
3. Store the UEFI memory map, ACPI pointer and the GOP framebuffer in memory.
4. Switch to own memory map with both identical and higher-half memory mapping.
5. Pass execution to the kernel's entry point.

### Kernel

This is where the majority of the codebase lies. The kernel tries to follow the principles of a microkernel, trying to keep policy out of it as much as possible. Of course, sometimes it has to break that rule, but it is few and far in between. It has various jobs, most of which I won't go in detail here, but to summarize it aims to do this: Launch into userspace, mediate access to hardware, and provide ways to communicate between processes.

The kernel currently features a single-core pre-emptive task scheduler, with a syscall (`sys_yield`) that allows processes to give up execution at any point.

The boot routine ends with the kernel parsing the `init` ELF file (read into memory by the bootloader), creating a process for it, passing along some initialization info (see `user/init/init_info.h`) and switching to userspace.

#### The multitastking system

As mentioned above, the kernel uses a pre-emptive multitasking system where the execution control is manually taken away from running processes in regular intervals. The process/thread system goes against Linux's task system and follows a more traditional process-thread hierarchy are relation. The system has processes, the processes have threads local to themselves which share address space and more with each other, and threads are blocks of execution. A process must have at least one thread (otherwise it is terminated) and is free to create as many threads as it wants.

Thread switching currently uses a *round-robin* scheduler, but this is bound to change in the future.

#### The kernel-userspace interface layer (Syscalls)

All userspace processes interact with the kernel and hardware through amd64 syscalls. The syscalls use a modified version of the *x86_64 Linux Syscall ABI* with *the `rdx` register being used as a secondary return value or if not used **always being clobbered***. You can read the syscall specification in the `syscall_specification.md` Markdown file.

### Userspace Programs

This is where the OS is defined. This is the most incomplete section of the operating system.

> [!WARNING]
> This section is purposefully left undocumented as it is very bare and changes every couple of commits. Despite that, the information written is *mostly* permanent and won't change.

#### `user/init`: The light system init executable

The userspace can be seen as "the init program and *everything else*". This is the first process started, and it always has the PID `0`. This init process launches other programs and has some special privileges.

#### `user/keyboard`: The keyboard stack / PS/2 keyboard driver

#### `user/graphics`: The graphics subsystem

A Wayland-like graphics subsystem.

#### `user/test`: The test program for the graphics subsystem

### Userspace Libraries

#### `user/libc`: A barebones C standard library implementation

As the entire project is written from scratch, the C standard library is too. It serves two purposes: Provide the C standard functions necessary, and provide functions to invoke syscalls (`<syscalls.h>`). It *(as of the time writing this)* uses the same allocator the kernel uses, only modified to use the `sys_memory_map` syscall instead of the `pmm_alloc` function the kernel uses.

#### `user/libg`: A primitive interaction library to the graphics subsystem

This is a library with the sole purpose of providing convenient ways to interact with the graphics subsystem, instead of just invoking raw RPC calls. User programs include this library by adding `USE_LIBG := 1` into their Makefile. This library is not a UI framework, that will be the job of a future library that will act something to the similar of GTK, WinUI, or SwiftUI.

## Current State

The project is slowly being developed. I prioritize getting *something* on the screen properly before anything (as that is the most visible functionality), so I am currently mainly developing the graphics subsystem. After that, I'll most likely aim for user I/O or the disk.

## License

The project is MIT licensed.
