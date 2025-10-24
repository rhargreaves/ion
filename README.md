# Ion :zap:

[![Build & Test](https://github.com/rhargreaves/ion/actions/workflows/build.yml/badge.svg)](https://github.com/rhargreaves/ion/actions/workflows/build.yml)

Light-weight HTTP/2 server in C++

## Goals

I'm using this as a learning exercise to learn more about HTTP/2 and modern C++.
As such, I am intentionally using low-level APIs and not libraries.
It is not intended to be a production-ready server!

## Progress

* Basic HTTP/2 over TLS
* For each request:
    * Returns 200 OK, empty body, for any request
* Close server using Ctrl+C or SIGTERM

## Build

### macOS

Install CMake (4.0+), CLion, Python (3.13+) and GCC/Clang:

```sh
brew upgrade cmake
make build test
```

### Linux

Use Docker:

```sh
docker build -t ion .
docker run -p 8443:8443 -it ion
```

Use Make:

```sh
make build test
```

## Usage

```sh
./ion-server/cmake-build-debug/ion-server &
curl -k --http2 -v https://localhost:8443/
```

## References

* [RFC 9113 - HTTP/2](https://datatracker.ietf.org/doc/html/rfc9113)
* [RFC 7541 - HPACK: Header Compression for HTTP/2](https://datatracker.ietf.org/doc/html/rfc7541)
