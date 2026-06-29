_phony: build copy

build: src/main.cpp
	g++ src/main.cpp -o build/krsh -lncurses -lstdc++fs -ldl

copy:
	cp build/krsh /usr/local/bin/krsh