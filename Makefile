#PYTEST_ARGS=-rP
PYTEST_ARGS=
CERT_PEM=cert.pem
KEY_PEM=key.pem
BUILD_DIR=ion-server/cmake-build-debug

build:
	cmake -S ion-server -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)
.PHONY: build

test: cert.pem
	pip3 install -q -r test/requirements.txt
	python3 -m pytest test/ $(PYTEST_ARGS)
.PHONY: test

$(CERT_PEM) $(KEY_PEM):
	openssl req -x509 -newkey rsa:4096 \
		-keyout $(KEY_PEM) \
		-out $(CERT_PEM) \
		-days 365 \
		-nodes -subj "/CN=localhost"
