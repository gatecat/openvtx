src = $(wildcard src/*.cpp src/6502/*.cpp)
obj = $(src:.cpp=.o)

CXXFLAGS = -std=c++11 -g 
LDFLAGS = -lSDL2 -lpthread
all: openvtx

openvtx: $(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) openvtx
