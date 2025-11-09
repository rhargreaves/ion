PYTEST_ARGS=-rP
#PYTEST_ARGS=--log-cli-level=DEBUG
#PYTEST_ARGS=
CERT_PEM=cert.pem
KEY_PEM=key.pem
BUILD_SUFFIX?=make
BUILD_DIR=build/$(BUILD_SUFFIX)
SERVER_PORT=8443

export ION_PATH=$(BUILD_DIR)/app/ion-server

build:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel
.PHONY: build

test: test-unit test-system
.PHONY: test

test-unit:
	$(BUILD_DIR)/test/unit/unit-test
.PHONY: test-unit

test-system: $(CERT_PEM) $(KEY_PEM)
	pip3 install -q -r test/system/requirements.txt
	python3 -m pytest test/system/ $(PYTEST_ARGS)
.PHONY: test-system

test-h2spec:
	./test/system/test_h2spec.sh
PHONY: test-h2spec

run: build $(CERT_PEM) $(KEY_PEM)
	$(ION_PATH) -p $(SERVER_PORT)
.PHONY: run

clean:
	-rm -rf $(BUILD_DIR) $(CERT_PEM) $(KEY_PEM)
.PHONY: clean

$(CERT_PEM) $(KEY_PEM):
	openssl req -x509 -newkey rsa:4096 \
		-keyout $(KEY_PEM) \
		-out $(CERT_PEM) \
		-days 365 \
		-nodes -subj "/CN=localhost"
