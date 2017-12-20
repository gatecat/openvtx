src = $(wildcard src/*.cpp src/6502/*.cpp)
obj = $(src:.cpp=.o)

CXXFLAGS = -std=c++11 -g -O3 `wx-config --cxxflags`
LDFLAGS = -lSDL2 -lpthread `wx-config --libs`
all: openvtx

openvtx: $(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) openvtx
