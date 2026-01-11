FROM ubuntu:26.04 AS builder
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
    lld \
    zip \
    unzip \
    tar \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Install CMake
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
ARG CC
ARG CXX
RUN cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_BUILD_TYPE=release \
    -S . -B build && \
    cmake --build build --target ion-server --parallel
RUN ldd /build-context/build/app/ion-server # for troubleshooting subsequent failures
RUN strip --strip-unneeded build/app/ion-server

# create minimal filesystem
RUN mkdir -p /runtime-root/etc /runtime-root/lib /runtime-root/usr/lib /runtime-root/app
RUN ldd build/app/ion-server | grep "=>" \
        | awk '{print $3}' \
        | xargs -r cp -L --parents -t /runtime-root/
RUN ldd build/app/ion-server | grep -v "=>" \
        | grep -v "linux-vdso" \
        | awk '{print $1}' \
        | xargs -r cp -L --parents -t /runtime-root/
RUN cp /etc/ssl/certs/ca-certificates.crt /runtime-root/etc/
RUN echo "ion:x:1000:1000:ion:/app:/usr/sbin/nologin" > /runtime-root/etc/passwd
RUN echo "ion:x:1000:" > /runtime-root/etc/group
RUN cp build/app/ion-server /runtime-root/app/ion-server

FROM scratch
COPY --from=builder /runtime-root /
USER ion
WORKDIR /app
EXPOSE 8443
ENTRYPOINT ["/app/ion-server"]
