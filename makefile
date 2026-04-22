CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
SRCDIR = src
SRCS = $(SRCDIR)/main.c $(SRCDIR)/cache.c $(SRCDIR)/policies.c
TARGET = sim.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

clean:
	del /f sim.exe

test_trace:
	python3 scripts/gen_test_trace.py

test: $(TARGET) test_trace
	./$(TARGET) lru traces/test.txt

.PHONY: all clean test test_trace