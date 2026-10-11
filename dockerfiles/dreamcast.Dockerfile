#----------------------
# | dreamcast.Dockerfile
# | Description: KallistiOS v2.1.1 toolchain (sh-elf GCC via dc-chain) plus mkdcdisc for the Legend of Mana Dreamcast port.
# | Author: suinevere
# | Dependencies: github.com/KallistiOS/KallistiOS (tag v2.1.1, commit d6f42aecdfef897de019f8a82345194bec17d1c6), gitlab.com/simulant/mkdcdisc (no tags; pinned to main commit 2b98b0d9480e364e3517355648fbc3b49fb19882)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential git wget curl ca-certificates bzip2 xz-utils texinfo flex bison \
    libgmp-dev libmpfr-dev libmpc-dev libelf-dev libjpeg-dev libpng-dev \
    python3 python3-pip python3-pytest meson ninja-build cmake pkg-config libisofs-dev \
    && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /opt/toolchains/dc && git clone --depth 1 --branch v2.1.1 \
    https://github.com/KallistiOS/KallistiOS.git /opt/toolchains/dc/kos
WORKDIR /opt/toolchains/dc/kos/utils/dc-chain
RUN cp Makefile.default.cfg Makefile.cfg \
    && make -j"$(nproc)" \
    && test -x /opt/toolchains/dc/sh-elf/bin/sh-elf-gcc
WORKDIR /opt/toolchains/dc/kos
RUN cp doc/environ.sh.sample environ.sh \
    && bash -c "source environ.sh && make -j$(nproc)" \
    && echo "source /opt/toolchains/dc/kos/environ.sh" >> /root/.bashrc \
    && echo "source /opt/toolchains/dc/kos/environ.sh" >> /root/.profile
RUN git clone https://gitlab.com/simulant/mkdcdisc.git /opt/mkdcdisc \
    && cd /opt/mkdcdisc && git checkout 2b98b0d9480e364e3517355648fbc3b49fb19882 \
    && meson setup builddir && meson compile -C builddir \
    && cp builddir/mkdcdisc /usr/local/bin/mkdcdisc
ENV KOS_BASE=/opt/toolchains/dc/kos
WORKDIR /work
