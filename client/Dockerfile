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
COPY client/ /build/client/

# Build usrsctp
WORKDIR /build/usrsctp

RUN ./bootstrap && \
    ./configure && \
    make -j"$(nproc)"

# Build the C client.
RUN gcc -Wall -Wextra -std=c11 \
        -I/build/usrsctp/usrsctplib \
        /build/client/client.c \
        -L/build/usrsctp/usrsctplib/.libs \
        -Wl,-rpath,/usr/local/lib \
        -lusrsctp \
        -o /build/client-bin

FROM ubuntu:24.04

WORKDIR /app

COPY --from=builder /build/client-bin /app/client

COPY --from=builder \
    /build/usrsctp/usrsctplib/.libs/libusrsctp.so.2 \
    /usr/local/lib/

RUN ldconfig

CMD ["./client"]
