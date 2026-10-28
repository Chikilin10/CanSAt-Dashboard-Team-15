CC     = gcc
CFLAGS = -O2 -Wall -Wextra -Iinclude
SRC    = src/main.c src/parser.c src/derive.c
OUT    = build/cansat_backend

all: $(OUT)

$(OUT): $(SRC) include/cansat.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) -lm

clean:
	rm -rf build

test: $(OUT)
	python3 ../tools/check_backend.py

.PHONY: all clean test
