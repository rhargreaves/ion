build-ion:
	cd .. && \
	make build-and-test-in-docker \
		build-docker-app BUILD_SUFFIX=docker
.PHONY: build-ion

up:
	docker compose up
.PHONY: up

down:
	docker compose down
.PHONY: down

clean:
	docker compose rm -f
.PHONY: clean
