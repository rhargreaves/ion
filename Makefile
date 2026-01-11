PYTEST_ARGS=
#PYTEST_ARGS=--log-cli-level=DEBUG
PYTEST_ARGS=-v
CERT_PEM=cert.pem
KEY_PEM=key.pem
BUILD_SUFFIX?=make
BUILD_DIR=build/$(BUILD_SUFFIX)
BUILD_TYPE ?= RelWithDebInfo
SERVER_PORT=8443
LOG_LEVEL=info
TTY_ARG := $(shell [ -t 0 ] && echo "-t")
GIT_SHA ?= $(shell git rev-parse --short HEAD 2>/dev/null)
export GIT_SHA
export ION_PATH=$(BUILD_DIR)/app/ion-server
export ION_TLS_CERT_PATH=cert.pem
export ION_TLS_KEY_PATH=key.pem
export SSLKEYLOGFILE=/tmp/ion-tls-keys.log
export CC=clang
export CXX=clang++

build: check-vcpkg
	cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -S . -B $(BUILD_DIR) \
		-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	cmake --build $(BUILD_DIR) --parallel
.PHONY: build

build-scratch-root-dir:
	./build-scratch-root.sh
.PHONY: build-scratch-root-dir

bootstrap-vcpkg:
	@if [ ! -f $(VCPKG_ROOT)/vcpkg ]; then \
		git clone https://github.com/microsoft/vcpkg.git $(VCPKG_ROOT); \
		$(VCPKG_ROOT)/bootstrap-vcpkg.sh -disableMetrics; \
	fi
.PHONY: bootstrap-vcpkg

test: test-unit test-integration test-system
.PHONY: test

test-unit:
	$(BUILD_DIR)/test/unit/unit-test
.PHONY: test-unit

test-integration: $(CERT_PEM) $(KEY_PEM)
	$(BUILD_DIR)/test/integration/int-test
.PHONY: test-integration

test-system: $(CERT_PEM) $(KEY_PEM)
	pip3 install -q -r test/system/requirements.txt
	PYTHONPATH=test/system python3 -m pytest test/system/ $(PYTEST_ARGS)
.PHONY: test-system

test-h2spec:
	./test/system/test_h2spec.sh
PHONY: test-h2spec

run: $(CERT_PEM) $(KEY_PEM)
	$(ION_PATH) -p $(SERVER_PORT) \
		-l $(LOG_LEVEL) \
		--under-test \
		$(ARGS)
.PHONY: run

benchmark:
	h2load https://localhost:$(SERVER_PORT)/_tests/ok -n 10000 -c 10 -t 8
.PHONY: benchmark

clean:
	-rm -rf $(BUILD_DIR) $(CERT_PEM) $(KEY_PEM)
.PHONY: clean

$(CERT_PEM) $(KEY_PEM):
	openssl req -x509 -newkey rsa:4096 \
		-keyout $(KEY_PEM) \
		-out $(CERT_PEM) \
		-days 365 \
		-nodes -subj "/CN=localhost"

build-docker-app:
	docker build -f app.Dockerfile \
		-t ion-app $(BUILD_DIR)/scratch-root
.PHONY: build-docker-app

run-docker-app:
	docker run \
		-p $(SERVER_PORT):$(SERVER_PORT) \
		-v $(PWD)/$(CERT_PEM):/certs/$(CERT_PEM):ro \
		-v $(PWD)/$(KEY_PEM):/certs/$(KEY_PEM):ro \
		-e ION_TLS_CERT_PATH=/certs/$(CERT_PEM) \
		-e ION_TLS_KEY_PATH=/certs/$(KEY_PEM) \
		-it ion-app \
		$(ARGS)
.PHONY: run-docker-app

build-ci:
	docker build -f build.Dockerfile -t ion .
	docker run \
		-e CC \
		-e CXX \
		-e BUILD_SUFFIX=docker \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e GIT_SHA=$(GIT_SHA) \
		-e VCPKG_ROOT=/vcpkg \
		-w /workspace \
		-v $(PWD):/workspace \
		-v $(VCPKG_ROOT):/vcpkg \
		-i $(TTY_ARG) \
		ion \
		make build test build-scratch-root-dir
.PHONY: build-ci

build-and-test-in-docker:
	docker build -f build.Dockerfile -t ion .
	docker run \
		-e CC \
		-e CXX \
		-e BUILD_SUFFIX=docker \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e GIT_SHA=$(GIT_SHA) \
		-e VCPKG_ROOT=/workspace/vcpkg-docker \
		-w /workspace \
		-v $(PWD):/workspace \
		-i $(TTY_ARG) \
		ion \
		make bootstrap-vcpkg build test build-scratch-root-dir
.PHONY: build-and-test-in-docker

check-vcpkg:
ifndef VCPKG_ROOT
	$(error VCPKG_ROOT is not set. Set it to a bootstrapped vcpkg repo directory.)
endif
.PHONY: check-vcpkg
