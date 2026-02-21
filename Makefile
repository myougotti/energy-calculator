# Makefile for Power Consumption Calculator

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -O2

# Target executable name
TARGET = power_calculator

# Source files
SOURCES = power_calculator.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete! Run with: ./$(TARGET)"

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete!"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean run rebuild
