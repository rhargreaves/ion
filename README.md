# Ion :zap:

[![Build & Test](https://github.com/rhargreaves/ion/actions/workflows/build.yml/badge.svg)](https://github.com/rhargreaves/ion/actions/workflows/build.yml)

Light-weight HTTP/2 server in C++

## Goals

I'm using this as a learning exercise to learn more about HTTP/2 and modern C++.
As such, I am intentionally using low-level APIs and avoiding libraries for the core flow (but using modern C++ features
to wrap the POSIX interfaces).
It is not intended to be a production-ready server!
I'm documenting non-obvious stuff I've learnt along the way in [LEARNINGS.md](docs/LEARNINGS.md).

## Progress

* HTTP/2 over TLS or cleartext (h2c)
* Support for HPACK (headers):
    * Static table entries
    * Dynamic table entries
    * Huffman encoded & plain text strings
* Supports response body, status codes
* Route registration
* Middleware support for manipulating requests/responses
* Static file serving (GET, HEAD requests)
* Combined Log Format (CLF) access logs
* Close server using Ctrl+C (`SIGINT`) or `SIGTERM`

<p align="center">
<img src="docs/screenshot.png" alt="ion screenshot" title="ion screenshot">
</p>

## Example Usage

```c++
#include "ion/http2_server.h"

int main() {
    ion::Http2Server server{};
    auto& router = server.router();

    router.add_route("/", "GET", [] {
        const std::string body_text = "hello";
        const std::vector<uint8_t> body_bytes(body_text.begin(), body_text.end());

        return ion::HttpResponse{
            .status_code = 200,
            .body = body_bytes,
            .headers = {{"content-type", "text/plain"}}};
    });

    server.start(8443);
    return 0;
}
```

See [app/main.cpp](app/main.cpp) for a more complete example, including signal handling.

### HTTP/2 cleartext (h2c) support

Use `--cleartext` to run the server in h2c mode. The HTTP/1.1 upgrade mechanism is not supported, so you will
need to ensure the client behaves accordingly (e.g. use `curl --http2-prior-knowledge`).

## Build

Requirements:

* CMake 4.0+
* Clang 17+
* OpenSSL 3+
* Python 3.13+

Development is typically done using JetBrains CLion.

### macOS

```sh
make build
```

### Linux

Docker:

```sh
docker build -f build.Dockerfile -t ion .
docker run -it \
  -e BUILD_SUFFIX=docker \
  -p 8443:8443 \
  -w /workspace \
  -v $(pwd):/workspace ion \
  make build
```

Make:

```sh
make build
```

## Standalone App Usage

You can also run the server as a standalone app.

Local:

```sh
./build/make/app/ion-server &
curl -k --http2 -v https://localhost:8443/_test/ok
```

Docker:

```sh
# build server image
docker build -f app.Dockerfile -t ion-server .

# run server
docker run -v $(pwd):/certs \
  -p 8443:8443 \
  -e ION_TLS_CERT_PATH=/certs/cert.pem \
  -e ION_TLS_KEY_PATH=/certs/key.pem \
  -it \
  ion-server
```

### Command Line Options

```
ion: the light-weight HTTP/2 server ⚡️

build/make/app/ion-server [OPTIONS]

OPTIONS:
  -h,     --help              Print this help message and exit
  -p,     --port UINT:INT in [1 - 65535] [8443]
                              Port to listen on
  -l,     --log-level TEXT:{trace,debug,info,warn,error,critical,off} [info]
                              Set logging level (trace, debug, info, warn, error, critical,
                              off)
  -s,     --static TEXT x 2   Map URL prefix to directory. Usage: --static /url/path ./fs/path
          --access-log TEXT
          --cleartext         Disables TLS and handles requests in HTTP/2 cleartext (h2c)
          --tls-cert-path TEXT (Env:ION_TLS_CERT_PATH)
                              Path to certificate file for TLS
          --tls-key-path TEXT (Env:ION_TLS_KEY_PATH)
                              Path to private key file for TLS
  -v,     --version           Display program version information and exit
```

## Test

Run all tests using `make test`

### Unit & Integration Tests

C++-based Catch2 tests:

```
make test-unit
make test-integration
```

### System Tests

Python-based tests employing `curl`, `hyperh2` & `httpx` to test the server end-to-end:

```
make test-system
```

### HTTP/2 Spec Tests

Using [h2spec](https://github.com/summerwind/h2spec):

```
make test-h2spec
```

## Benchmarks

Using `h2load`. See [benchmark/results.md](benchmark/results.md) for tests ran during development.

## References

* [RFC 9113 - HTTP/2](https://datatracker.ietf.org/doc/html/rfc9113)
* [RFC 7541 - HPACK: Header Compression for HTTP/2](https://datatracker.ietf.org/doc/html/rfc7541)
