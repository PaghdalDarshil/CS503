# Makefile for the Text Line Processor

# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = stringfun

# Source files
SRCS = assignment-1.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program with sample input (example usage)
run:
	./$(TARGET) -w "Hello world! This is a test string."

.PHONY: all clean run

