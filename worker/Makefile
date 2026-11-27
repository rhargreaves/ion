WORKER_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
CERTS_DIR := $(WORKER_DIR)/certs
CERT_PEM := $(CERTS_DIR)/cert.pem
KEY_PEM := $(CERTS_DIR)/key.pem

$(CERT_PEM) $(KEY_PEM):
	mkdir -p $(CERTS_DIR)
	openssl req -x509 -newkey rsa:2048 \
		-keyout $(KEY_PEM) -out $(CERT_PEM) \
		-days 365 -nodes \
		-subj "/C=US/ST=Local/L=Local/O=Dev/CN=localhost" \
		-addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

certs: $(CERT_PEM) $(KEY_PEM)
.PHONY: certs

clean-certs:
	rm -f $(CERT_PEM) $(KEY_PEM)
.PHONY: clean-certs

run: certs
	cd $(WORKER_DIR) && \
	npx workerd serve config.capnp --experimental --watch
.PHONY: run
