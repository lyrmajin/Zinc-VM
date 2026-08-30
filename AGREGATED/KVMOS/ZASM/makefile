# Compiler and flags
CC      := clang
CFLAGS  := -I./src/assambler/frontend -I./src/assambler/backend
#CFLAGS  := -Wall -Wextra -I./src/assambler/frontend -I./src/assambler/backend
LDFLAGS := 

# Project structure
SRC_DIR := src/assambler
FRONTEND_DIR := $(SRC_DIR)/frontend
BACKEND_DIR  := $(SRC_DIR)/backend
BUILD_DIR := build

# Source files
SRCS := $(SRC_DIR)/main.c \
        $(FRONTEND_DIR)/parser.c \
        $(FRONTEND_DIR)/tokenizer.c \
        $(BACKEND_DIR)/codegen.c \
		$(BACKEND_DIR)/error.c \
		$(BACKEND_DIR)/utils.c

# Object files in build/
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Output binary
TARGET := $(BUILD_DIR)/nzasm

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile step
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean
