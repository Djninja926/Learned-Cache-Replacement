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

test_gen: $(TARGET) test_trace
	./$(TARGET) lru traces/test.txt
	./$(TARGET) lfu traces/test.txt
	./$(TARGET) ml traces/test.txt
	./$(TARGET) opt traces/test.txt

test_astar: $(TARGET)
	./$(TARGET) lru traces/astar.txt
	./$(TARGET) lfu traces/astar.txt
	./$(TARGET) ml traces/astar.txt
	./$(TARGET) opt traces/astar.txt

test_mcf: $(TARGET)
	./$(TARGET) lru traces/mcf.txt
	./$(TARGET) lfu traces/mcf.txt
	./$(TARGET) ml traces/mcf.txt
	./$(TARGET) opt traces/mcf.txt

.PHONY: all clean test test_trace