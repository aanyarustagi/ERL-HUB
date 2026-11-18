# ERL-HUB build file.  Usage: make | make run | make test | make clean
CXX      ?= g++
STD      ?= c++11
CXXFLAGS ?= -Wall -Wextra -pedantic -O2
CPPFLAGS += -Iinclude

LIB_SRC := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
LIB_OBJ := $(patsubst src/%.cpp,build/%.o,$(LIB_SRC))

all: erl_hub

erl_hub: $(LIB_OBJ) build/main.o
	$(CXX) -std=$(STD) $(CXXFLAGS) $^ -o $@

erl_hub_tests: $(LIB_OBJ) build/test_erl_hub.o
	$(CXX) -std=$(STD) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp $(wildcard include/*.h) | build
	$(CXX) -std=$(STD) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/test_erl_hub.o: tests/test_erl_hub.cpp $(wildcard include/*.h) | build
	$(CXX) -std=$(STD) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build:
	mkdir -p build

run: erl_hub
	./erl_hub

test: erl_hub_tests
	./erl_hub_tests

clean:
	rm -rf build erl_hub erl_hub_tests *.bin *.tmp

.PHONY: all run test clean
