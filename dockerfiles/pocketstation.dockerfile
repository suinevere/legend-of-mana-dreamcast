#----------------------
# | pocketstation.dockerfile
# | Description: Image lom-pocket: arm-none-eabi GCC and newlib for OpenPSK and the budget build, g++ for the reference runner, Python with Capstone and Unicorn.
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
    make \
    python3 \
    python3-pip \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages capstone pytest unicorn

RUN git config --global --add safe.directory '*'

WORKDIR /work
