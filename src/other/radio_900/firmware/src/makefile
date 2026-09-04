# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I../inc
LDFLAGS = -shared
BUILD_DIR = ../build
# INTERFACE = eth0

# Source files
SRC = main.c libnetat.c
OBJ = $(SRC:.c=.o)
TARGET = sample

# Shared library target
LIB_NAME = libnetat_api.so
LIB_SRC = libnetat_api.c libnetat.c

# Default build rule: executable
all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build shared library
lib: $(BUILD_DIR)/$(LIB_NAME)

$(BUILD_DIR)/$(LIB_NAME): $(LIB_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# # Run the sample executable
# run: all
# 	cd $(BUILD_DIR) && ./$(TARGET) $(INTERFACE)

# Clean build files
clean:
	rm -f *.o $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(LIB_NAME)

.PHONY: all clean run lib
