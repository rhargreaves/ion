PYTEST_ARGS=
#PYTEST_ARGS=--log-cli-level=DEBUG
#PYTEST_ARGS=
CERT_PEM=cert.pem
KEY_PEM=key.pem
BUILD_SUFFIX?=make
BUILD_DIR=build/$(BUILD_SUFFIX)
BUILD_TYPE=RelWithDebInfo
SERVER_PORT=8443
LOG_LEVEL=info

GIT_SHA=$(shell git rev-parse --short HEAD 2>/dev/null)
export BASE_VERSION=0.1.4
ifneq ($(GIT_SHA),)
	export BUILD_VERSION=$(BASE_VERSION)-$(GIT_SHA)
else
	export BUILD_VERSION=$(BASE_VERSION)
endif
export ION_PATH=$(BUILD_DIR)/app/ion-server
export ION_TLS_CERT_PATH=cert.pem
export ION_TLS_KEY_PATH=key.pem
export SSLKEYLOGFILE=/tmp/ion-tls-keys.log

build:
	cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel
.PHONY: build

build-docker-app:
	docker build -f app.Dockerfile -t ion-app .
.PHONY: build-docker-app

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
	python3 -m pytest test/system/ $(PYTEST_ARGS)
.PHONY: test-system

test-h2spec:
	./test/system/test_h2spec.sh
PHONY: test-h2spec

run: build $(CERT_PEM) $(KEY_PEM)
	$(ION_PATH) -p $(SERVER_PORT) \
		-l $(LOG_LEVEL) \
		$(ARGS)
.PHONY: run

benchmark:
	h2load https://localhost:$(SERVER_PORT)/ -n 1000 -c 10 -t 8
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
