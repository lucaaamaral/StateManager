# StateManager Dockerfile
# Multi-stage build for C++ library with Redis dependencies

FROM alpine:3.19 AS lib_build
WORKDIR /app
RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    tree

COPY . .
RUN mkdir -p /app/build && \
    cd /app/build && \
    cmake .. && \
    cmake --build .

FROM lib_build AS test
WORKDIR /app

# Build tests
RUN cd /app/build && \
    cmake -DBUILD_TESTING=ON .. && \
    cmake --build . --target StateManagerTests

CMD ["/app/build/bin/StateManagerTests"]

FROM lib_build AS examples
WORKDIR /app

# Build tests
RUN cd /app/build && \
    cmake -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON .. && \
    cmake --build . --target basic_demo && \
    cmake --build . --target multithreaded_demo

CMD ["/app/build/bin/basic_demo"]