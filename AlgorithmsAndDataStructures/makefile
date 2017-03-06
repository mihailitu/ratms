CC := g++
CO := -c -ggdb -std=c++1z -O0 -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing -Iinclude/

LN := g++
LO =

SRCS := $(wildcard *.cpp) $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp, obj/%.o, $(SRCS))

TARGET := segwick

first: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LN) $(LO) -o $@ $^ #-lpthread  -lboost_system

obj/%.o: %.cpp
	$(CC) $(CO) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
