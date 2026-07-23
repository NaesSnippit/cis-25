CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET := half_moon_holdem
SOURCE := src/main.cpp

.PHONY: all run test demo clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --self-test

demo: $(TARGET)
	./$(TARGET) --demo --seed 20260722 --data-dir demo_data

clean:
	$(RM) $(TARGET)
