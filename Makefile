# Makefile for Power Consumption Calculator

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Linker flags
LDFLAGS = -lcomctl32 -lgdi32 -lcomdlg32 -mwindows

# Target executable name
TARGET = power_calculator.exe

# Source files
SOURCES = power_calculator.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@echo "Build complete! Run with: ./$(TARGET)"

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) *.o
	@echo "Clean complete!"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean run rebuild
