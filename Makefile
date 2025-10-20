#PYTEST_ARGS=-rP
PYTEST_ARGS=

build:
	cmake -S ion-server -B ion-server/cmake-build-debug
	cmake --build ion-server/cmake-build-debug
.PHONY: build

test: cert.pem
	pip3 install -q -r test/requirements.txt
	python3 -m pytest test/ -v $(PYTEST_ARGS)
.PHONY: test

cert.pem:
	openssl req -x509 -newkey rsa:4096 \
		-keyout key.pem \
		-out cert.pem \
		-days 365 \
		-nodes -subj "/CN=localhost"
