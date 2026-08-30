# Compiler
CC = clang
CFLAGS = -Wall -Wextra -O2

# Source files
SRC = $(wildcard *.c)

# Build directory
BUILD_DIR = build

# Object files (in build/)
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

# Executable name
TARGET = $(BUILD_DIR)/zcc

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files into executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

# Compile .c files into .o files in build/
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
