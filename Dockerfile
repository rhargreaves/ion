FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libssl-dev \
    python3 \
    python3-pip \
    python3-venv \
    openssl \
    make \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install CMake 4.0+ (detect architecture)
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

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . .

# Create and activate virtual environment, then build and test
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN make build
RUN make test

CMD ["./ion-server/cmake-build-debug/ion-server"]
