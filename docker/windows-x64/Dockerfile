# syntax=docker/dockerfile:1

# Cross-compile AzaharPlus for Windows x64 from Linux using MXE (llvm-mingw).
#
# Usage (from repository root with submodules initialized):
#   Export Windows x64 bundle directly to a local directory:
#     docker build -f docker/windows-x64/Dockerfile --output type=local,dest=./dist-win64 .
#   Or build as a Docker image:
#     docker build -f docker/windows-x64/Dockerfile -t azahar-windows-x64 .

FROM opensauce04/azahar-build-environment:latest AS builder

ARG BUILD_TYPE=Release
ARG ENABLE_QT_TRANSLATION=ON
ARG USE_DISCORD_PRESENCE=OFF
ARG ENABLE_TESTS=OFF
ARG ENABLE_LTO=ON
ARG ENABLE_SSE42=ON
ARG TARGET_ARCH=""
ARG EXTRA_CMAKE_FLAGS=""

ENV PATH="/opt/mxe/usr/bin:${PATH}" \
    CCACHE_DIR="/ccache" \
    CCACHE_COMPILERCHECK="content" \
    CCACHE_SLOPPINESS="time_macros"

# Pre-fetch RapidJSON for discord-rpc so CMake does not need network access during configure
ADD https://github.com/Tencent/rapidjson/archive/7c73dd7de7c4f14379b781418c6e947ad464c818.tar.gz /tmp/rapidjson.tar.gz
RUN mkdir -p /opt/rapidjson \
    && tar -xzf /tmp/rapidjson.tar.gz -C /opt/rapidjson --strip-components=1 \
    && rm -f /tmp/rapidjson.tar.gz

WORKDIR /src
COPY . .

RUN git config --global --add safe.directory '*'

RUN --mount=type=cache,target=/ccache \
    ARCH_FLAGS="" \
    && if [ -n "${TARGET_ARCH}" ]; then ARCH_FLAGS="-march=${TARGET_ARCH}"; fi \
    && x86_64-w64-mingw32.shared-cmake -S /src -B /src/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DCMAKE_AR=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ar \
        -DCMAKE_RANLIB=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ranlib \
        -DCMAKE_C_COMPILER_AR=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ar \
        -DCMAKE_CXX_COMPILER_AR=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ar \
        -DCMAKE_C_COMPILER_RANLIB=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ranlib \
        -DCMAKE_CXX_COMPILER_RANLIB=/opt/mxe/usr/x86_64-pc-linux-gnu/bin/llvm-ranlib \
        -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG ${ARCH_FLAGS}" \
        -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG ${ARCH_FLAGS}" \
        -DENABLE_QT_TRANSLATION="${ENABLE_QT_TRANSLATION}" \
        -DUSE_DISCORD_PRESENCE="${USE_DISCORD_PRESENCE}" \
        -DENABLE_TESTS="${ENABLE_TESTS}" \
        -DENABLE_LTO="${ENABLE_LTO}" \
        -DENABLE_SSE42="${ENABLE_SSE42}" \
        -DRAPIDJSON=/opt/rapidjson \
        -DRAPIDJSONTEST=/opt/rapidjson \
        ${EXTRA_CMAKE_FLAGS} \
    && cmake --build /src/build --parallel \
    && cmake --build /src/build --target bundle \
    && x86_64-w64-mingw32.shared-strip -s /src/build/bundle/*.exe \
    && ccache -s

FROM scratch AS export
COPY --from=builder /src/build/bundle/ /
