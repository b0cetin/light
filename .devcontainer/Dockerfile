# ──────────────────────────────────────────────────────────────────────────────
# Stage 1: Build gnu-efi (for bootloader)
# ──────────────────────────────────────────────────────────────────────────────

FROM --platform=linux/amd64 ubuntu:24.04 AS gnu-efi-builder

RUN apt-get update && apt-get install -y \
    build-essential git make \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ncroxon/gnu-efi.git \
    /gnu-efi && make -C /gnu-efi

# ──────────────────────────────────────────────────────────────────────────────
# Stage 2: Build x86_64-elf cross-compiler (for kernel)
# ──────────────────────────────────────────────────────────────────────────────

FROM --platform=linux/amd64 ubuntu:24.04 AS cross-builder

ARG TARGET=x86_64-elf
ARG BINUTILS_VERSION=2.45.1
ARG GCC_VERSION=15.2.0
ARG GDB_VERSION=15.2
ARG PREFIX=/cross

RUN apt-get update && apt-get install -y \
    build-essential wget \
    libgmp-dev libmpfr-dev libmpc-dev \
    texinfo bison flex \
    libncurses-dev zlib1g-dev libexpat1-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

RUN wget -q https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz \
    && tar xf binutils-${BINUTILS_VERSION}.tar.gz \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-${BINUTILS_VERSION}/configure \
        --target=${TARGET} \
        --prefix=${PREFIX} \
        --with-sysroot \
        --disable-nls \
        --disable-werror \
    && make -j$(nproc) && make install

RUN wget -q https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz \
    && tar xf gcc-${GCC_VERSION}.tar.gz \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-${GCC_VERSION}/configure \
        --target=${TARGET} \
        --prefix=${PREFIX} \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers \
    && make -j$(nproc) all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc

RUN wget -q https://ftp.gnu.org/gnu/gdb/gdb-${GDB_VERSION}.tar.gz \
    && tar xf gdb-${GDB_VERSION}.tar.gz \
    && mkdir build-gdb && cd build-gdb \
    && ../gdb-${GDB_VERSION}/configure \
        --target=${TARGET} \
        --prefix=${PREFIX} \
        --disable-nls \
        --disable-werror \
    && make -j$(nproc) all-gdb && make install-gdb

# ──────────────────────────────────────────────────────────────────────────────
# Stage 3: Final unified image
# ──────────────────────────────────────────────────────────────────────────────

FROM --platform=linux/amd64 ubuntu:24.04

RUN apt-get update && apt-get install -y \
    gcc binutils make \
    bear git gdb \
    && rm -rf /var/lib/apt/lists/*

COPY --from=gnu-efi-builder /gnu-efi /gnu-efi
COPY --from=cross-builder   /cross   /cross
ENV PATH="/cross/bin:$PATH"

COPY bootloader/bootloader.mk /build/bootloader.mk
COPY kernel/kernel.mk         /build/kernel.mk

RUN ln -s /build/kernel.mk /build/Makefile

WORKDIR /workspace