# syntax=docker/dockerfile:1

ARG TOOLCHAIN_IMAGE=helengine-n64-toolchain
FROM ${TOOLCHAIN_IMAGE}

COPY .deps/libdragon /opt/src/libdragon
COPY .deps/tiny3d /opt/src/tiny3d

RUN cd /opt/src/libdragon && ./build.sh --no-examples
RUN make -C /opt/src/tiny3d -j4 && make -C /opt/src/tiny3d install
RUN make -C /opt/src/tiny3d/examples/00_quad -j4

WORKDIR /workspace
