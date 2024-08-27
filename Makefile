.PHONY: build

CC = g++
CFLAGS = -Wall -Wextra -Werror -std=c++2b -O3

build:
	$(CC) $(CFLAGS) -o build/cpp-bundler main.cpp

clean:
	rm -rf build/cpp-bundler
