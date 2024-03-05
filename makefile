CXX = g++
CXXFLAGS = -std=c++11 -O3
TARGET = cachesim
SRCS = cachesim.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $<

clean:
    rm -f $(OBJS) $(TARGET)