# Ion :zap:

[![Build & Test](https://github.com/rhargreaves/ion/actions/workflows/build.yml/badge.svg)](https://github.com/rhargreaves/ion/actions/workflows/build.yml)

Light-weight HTTP/2 server in C++

## Progress

* Returns 200 OK, empty body, for all requests from `curl`

## Getting Started

Install CMake (4.0+), CLion, Python (3.13+) and GCC/Clang

### macOS
```sh
brew upgrade cmake
```

### Build

1. Open CMake project in CLion
2. Build project

### Integration Tests

```sh
make test
```

## References

* [RFC 9113 - HTTP/2](https://datatracker.ietf.org/doc/html/rfc9113)
* [RFC 7541 - HPACK: Header Compression for HTTP/2](https://datatracker.ietf.org/doc/html/rfc7541)
