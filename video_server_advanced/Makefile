# Makefile for Video Streaming Server

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGET = video_server
SOURCES = video_server.c video_index.c strops.c login.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete! Run with: ./$(TARGET)"

# Compile individual object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned build files"

# Install (copy to /usr/local/bin - requires sudo)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET)"

# Debug build with symbols
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete"

# Run the server
run: $(TARGET)
	./$(TARGET)

# Check for common issues
check:
	@echo "Checking dependencies..."
	@which gcc >/dev/null || echo "ERROR: gcc not found"
	@ls main_pages/ >/dev/null 2>&1 || echo "WARNING: main_pages/ directory not found"
	@ls movies/ >/dev/null 2>&1 || echo "WARNING: movies/ directory not found" 
	@ls television/ >/dev/null 2>&1 || echo "WARNING: television/ directory not found"
	@ls users/ >/dev/null 2>&1 || echo "WARNING: users/ directory not found"
	@echo "Check complete"

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build the video server (default)"
	@echo "  clean     - Remove build files"
	@echo "  debug     - Build with debug symbols"
	@echo "  run       - Build and run the server"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  check     - Check for dependencies and directories"
	@echo "  help      - Show this help"

# Declare phony targets
.PHONY: all clean install uninstall debug run check help