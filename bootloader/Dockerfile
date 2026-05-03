
# Stage 1: Build gnu-efi

FROM ubuntu:24.04 AS gnu-efi-builder

RUN apt update && apt install -y \
    build-essential git make \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ncroxon/gnu-efi.git \
    /gnu-efi && make -C /gnu-efi

# Stage 2: Build

FROM ubuntu:24.04

RUN apt update && apt install -y \
    gcc binutils make \
    && rm -rf /var/lib/apt/lists/*

COPY --from=gnu-efi-builder /gnu-efi /gnu-efi

COPY bootloader.mk /build/Makefile

VOLUME ["/src", "/out"]

ENTRYPOINT ["make", "-f", "/build/Makefile"]
CMD ["all"]
