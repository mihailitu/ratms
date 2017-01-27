CC := g++
CO := -c -ggdb -std=c++1z -O0 -W -Wall -Wextra -pedantic -Wno-unknown-pragmas -fPIC -fno-strict-aliasing

LN := g++
LO =

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

TARGET := crack

first: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LN) $(LO) -o $@ $^ #-lpthread  -lboost_system

%.o: %.cpp
	$(CC) $(CO) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
