# ==============================================================================
# TinyLLVM Makefile
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -O2
DEBUG_FLAGS = -g -DDEBUG

# Source files
AST_SOURCES = tinyllvm_ast.c
TEST_SOURCES = tinyllvm_ast_test.c

# Object files
AST_OBJECTS = $(AST_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Executables
TEST_EXEC = tinyllvm_ast_test

.PHONY: all clean test debug

# Default target
all: $(TEST_EXEC)

# Build test executable
$(TEST_EXEC): $(TEST_OBJECTS) $(AST_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Build object files
%.o: %.c tinyllvm_ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Run tests
test: $(TEST_EXEC)
	@echo "Running AST tests..."
	@./$(TEST_EXEC)
	@echo ""

# Clean build artifacts
clean:
	rm -f $(AST_OBJECTS) $(TEST_OBJECTS) $(TEST_EXEC)
	@echo "Cleaned build artifacts"

# Help
help:
	@echo "TinyLLVM Makefile"
	@echo "================="
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all executables (default)"
	@echo "  test     - Build and run tests"
	@echo "  debug    - Build with debug symbols"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make           # Build everything"
	@echo "  make test      # Build and run tests"
	@echo "  make clean     # Clean up"
	@echo "  make debug     # Debug build"