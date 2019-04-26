CC := g++
CO := -c -std=c++17 -O3 -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing

LN := g++
LO =

SRCS := $(wildcard src/*.cpp src/tests/*.cpp)
OBJS := $(patsubst %.cpp, bin/release/%.o, $(SRCS))

TARGET := bin/release/simulator

first: all

all: $(TARGET)
debug: $(TARGET_DBG)

$(TARGET): $(OBJS)
	$(LN) $(LO) -o $@ $^ #-lpthread  -lboost_system
	
bin/release/%.o: %.cpp
	mkdir -p bin/release/src/tests
	$(CC) $(CO) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
