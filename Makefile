
test:
	pip3 install -e test
	python3 -m pytest test/ -v
.PHONY: test