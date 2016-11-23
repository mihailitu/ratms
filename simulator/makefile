CC := g++
CO := -c -ggdb -O0 -pipe -ansi -W -Wall -Wextra -pedantic -std=c++11 -Wno-non-virtual-dtor -Wwrite-strings -Wno-unknown-pragmas -fPIC -fno-strict-aliasing

LN := g++
LO =

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

TARGET := simulator

first: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LN) $(LO) -o $@ $^ #-lpthread  -lboost_system

%.o: %.cpp
	$(CC) $(CO) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
