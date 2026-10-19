FROM ubuntu:24.04 AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        autoconf \
        automake \
        libtool \
        pkg-config && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY usrsctp/ /build/usrsctp/
COPY server/ /build/server/

# Build usrsctp
WORKDIR /build/usrsctp

RUN ./bootstrap && \
    ./configure && \
    make -j"$(nproc)"

# Build server using CMake
WORKDIR /build/server

RUN cmake -S . -B build \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_PREFIX_PATH=/build/usrsctp && \
    cmake --build build -j"$(nproc)"

FROM ubuntu:24.04

WORKDIR /app

COPY --from=builder /build/server/build/server /app/server

COPY --from=builder \
    /build/usrsctp/usrsctplib/.libs/libusrsctp.so.2 \
    /usr/local/lib/

RUN ldconfig

CMD ["./server"]
