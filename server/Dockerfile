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

# Build the callback-free C server.
RUN gcc -Wall -Wextra -std=c11 \
        -I/build/usrsctp/usrsctplib \
        /build/server/server.c \
        -L/build/usrsctp/usrsctplib/.libs \
        -Wl,-rpath,/usr/local/lib \
        -lusrsctp \
        -o /build/server-bin

FROM ubuntu:24.04

WORKDIR /app

COPY --from=builder /build/server-bin /app/server

COPY --from=builder \
    /build/usrsctp/usrsctplib/.libs/libusrsctp.so.2 \
    /usr/local/lib/

RUN ldconfig

CMD ["./server"]
