#----------------------
# | flycast.Dockerfile
# | Description: Headless Linux Flycast v2.7 (built from source, GDB server on) under Xvfb, for the Dreamcast port's boot check.
# | Author: suinevere
# | Dependencies: github.com/flyinghead/flycast (tag v2.7, commit 5aa091fde632fb332c8d8c34e280d62dc951954c)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential git ca-certificates cmake ninja-build pkg-config python3 \
    libcurl4-openssl-dev libudev-dev libsdl2-dev libzip-dev libgl-dev libx11-dev \
    xvfb xauth libgl1-mesa-dri libglx-mesa0 gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/flyinghead/flycast.git /opt/flycast-src \
    && cd /opt/flycast-src && git checkout 5aa091fde632fb332c8d8c34e280d62dc951954c \
    && git submodule update --init --recursive
WORKDIR /opt/flycast-src
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DUSE_VULKAN=OFF -DUSE_BREAKPAD=OFF -DUSE_LUA=OFF -DUSE_PULSEAUDIO=OFF -DUSE_LIBAO=OFF \
    -DUSE_ALSA=OFF -DENABLE_GDB_SERVER=ON \
    && cmake --build build -j"$(nproc)" \
    && cp build/flycast /usr/local/bin/flycast \
    && rm -rf build
ENV SDL_AUDIODRIVER=dummy
WORKDIR /work
