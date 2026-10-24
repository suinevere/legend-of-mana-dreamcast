#----------------------
# | pocketstation.dockerfile
# | Description: Image lom-pocket: arm-none-eabi GCC and newlib for OpenPSK and the budget build, g++ for the reference runner, Python with Capstone and Unicorn, Rust/Cargo with the Waterbear LC86K assembler for the VMU harness.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
# Build from the repo root: docker build -t lom-pocket -f dockerfiles/pocketstation.dockerfile .
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    binutils-arm-none-eabi \
    git \
    ca-certificates \
    curl \
    make \
    patch \
    python3 \
    python3-pip \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages capstone pytest unicorn

# Ubuntu 24.04's apt `cargo` (1.75.0) is too old to build waterbear's own
# dependency tree (unicode-segmentation needs rustc 1.85+), so Rust comes
# from rustup instead, as waterbear's own README recommends. Pinned to an
# exact rustc so a later image rebuild can't silently pick up a newer
# toolchain and, since waterbear ships no Cargo.lock, different transitive
# dependency versions with it.
ENV RUSTUP_HOME=/usr/local/rustup CARGO_HOME=/usr/local/cargo PATH=/usr/local/cargo/bin:$PATH
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.99.1

# Pinned to the only version Waterbear has published to crates.io; see
# pocketstation/harness/Makefile for the exact assemble command this installs.
# waterbear 0.12.0 ships no Cargo.lock, so --locked still resolves fresh here,
# but it stops cargo from silently relaxing that resolution afterward.
RUN cargo install waterbear --version 0.12.0 --locked --root /usr/local

RUN git config --global --add safe.directory '*'

WORKDIR /work
