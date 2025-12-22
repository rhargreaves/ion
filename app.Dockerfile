FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    clang clang-tools libc++-dev libc++abi-dev \
    build-essential \
    libssl-dev \
    python3 \
    python3-pip \
    python3-venv \
    openssl \
    make \
    wget \
    curl \
    net-tools \
    git \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
        CMAKE_ARCH="linux-x86_64"; \
    elif [ "$ARCH" = "aarch64" ]; then \
        CMAKE_ARCH="linux-aarch64"; \
    else \
        echo "Unsupported architecture: $ARCH" && exit 1; \
    fi && \
    wget https://github.com/Kitware/CMake/releases/download/v4.1.1/cmake-4.1.1-${CMAKE_ARCH}.sh -q -O /tmp/cmake.sh && \
    sh /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm /tmp/cmake.sh
WORKDIR /build-context
COPY . .
ARG GIT_SHA
RUN cmake -DCMAKE_BUILD_TYPE=Release -S . -B build && \
    cmake --build build --target ion-server --parallel

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    libc++1 \
    && rm -rf /var/lib/apt/lists/*
RUN useradd --system --shell /usr/sbin/nologin ion

WORKDIR /app
RUN chown ion:ion /app
COPY --from=builder --chown=ion:ion \
    /build-context/build/app/ion-server /app/ion-server
USER ion
EXPOSE 8443
ENTRYPOINT ["/app/ion-server"]
