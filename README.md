# Ion :zap:

[![Build & Test](https://github.com/rhargreaves/ion/actions/workflows/build.yml/badge.svg)](https://github.com/rhargreaves/ion/actions/workflows/build.yml)

Light-weight HTTP/2 server in C++

## Goals

I'm using this as a learning exercise to learn more about HTTP/2 and modern C++.
As such, I am intentionally using low-level APIs and avoiding libraries for the core flow (but using modern C++ features
to wrap the POSIX interfaces).
It is not intended to be a production-ready server!

## Progress

* Basic HTTP/2 over TLS
* For each request:
    * Returns 200 OK, empty body, for any request
* Close server using Ctrl+C or SIGTERM

<p align="center">
<img src="docs/screenshot.png" alt="ion screenshot" title="ion screenshot">
</p>

## Build

### macOS

Install CMake (4.0+), CLion, Python (3.13+) and GCC/Clang:

```sh
brew install cmake

make build
```

### Linux

Docker:

```sh
docker build -t ion .

docker run -it \
  -e BUILD_SUFFIX=docker \
  -p 8443:8443 \
  -w /workspace \
  -v $(pwd):/workspace ion \
  make run
```

Make:

```sh
make build
```

## Usage

```sh
./ion-server/cmake-build-debug/ion-server &
curl -k --http2 -v https://localhost:8443/
```

## Test

### Integration Tests

```
make test
```

### HTTP/2 Spec Tests

Using [h2spec](https://github.com/summerwind/h2spec):

```
make test-h2spec
```

## References

* [RFC 9113 - HTTP/2](https://datatracker.ietf.org/doc/html/rfc9113)
* [RFC 7541 - HPACK: Header Compression for HTTP/2](https://datatracker.ietf.org/doc/html/rfc7541)
