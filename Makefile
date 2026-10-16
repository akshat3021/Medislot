# Medislot Makefile: Links PostgreSQL (-lpqxx -lpq) and Threading (-pthread)
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc -Ithird_party
LDFLAGS ?= -lpqxx -lpq -pthread

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = medislot

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: tests/tests.cpp $(filter-out src/main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o test_runner $^ $(LDFLAGS)
	./test_runner

clean:
	rm -f src/*.o $(TARGET) test_runner

.PHONY: all test clean
