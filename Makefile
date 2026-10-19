# Medislot: Hospital Appointment & Resource Management System
# Build: make | Run: make run | Test: make test
CXX      ?= g++
PG_INC   := $(shell pg_config --includedir 2>/dev/null)
PG_LIB   := $(shell pg_config --libdir 2>/dev/null)

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread -MMD -MP -Ithird_party -Isrc
ifneq ($(PG_INC),)
  CXXFLAGS += -I$(PG_INC)
endif

LDLIBS   ?= -lpq
ifneq ($(PG_LIB),)
  LDLIBS   := -L$(PG_LIB) $(LDLIBS)
endif

ifeq ($(OS),Windows_NT)
  CXXFLAGS += -D_WIN32_WINNT=0x0A00
  LDLIBS   += -lws2_32
endif

APP_OBJS := $(patsubst src/%.cpp,build/%.o,$(filter-out src/main.cpp,$(wildcard src/*.cpp)))

all: medislot

medislot: build/main.o $(APP_OBJS)
	$(CXX) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

medislot_tests: build/tests.o $(APP_OBJS)
	$(CXX) $^ -o $@ $(LDLIBS)

build/tests.o: tests/tests.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: medislot
	./medislot

init: medislot
	./medislot --init

demo-race: medislot
	./medislot --demo-race

demo-deadlock: medislot
	./medislot --demo-deadlock

test: medislot_tests
	./medislot_tests

format:
	clang-format -i src/*.cpp src/*.hpp tests/*.cpp

clean:
	rm -rf build medislot medislot.exe medislot_tests medislot_tests.exe test_runner

-include build/*.d
.PHONY: all run init demo-race demo-deadlock test format clean
