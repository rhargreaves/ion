build-ion:
	cd .. && \
	make build-and-test-in-docker \
		build-docker-app BUILD_SUFFIX=docker
.PHONY: build-ion

up:
	docker compose up
.PHONY: up

up-grafana:
	docker compose -f docker-compose-grafana.yml up
.PHONY: up-grafana

down:
	docker compose down
.PHONY: down

down-grafana:
	docker compose -f docker-compose-grafana.yml down
.PHONY: down-grafana

clean:
	docker compose rm -f
.PHONY: clean
