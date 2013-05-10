SLASHPATH="D:/dev/_libz/slash-a/lib"

CC=gcc
CFLAGS=-std=gnu11 -O3 -Wall -Wextra -DWINVER=0x0501
CXX=g++
CXXFLAGS=-std=gnu++11 -O3 -Wall -Wextra -I$(SLASHPATH) -DWINVER=0x0501

LDFLAGS=-L$(SLASHPATH) -static -static-libgcc -static-libstdc++
LIBS=-lm -lslasha -lws2_32

SOURCES=genetic_client_main.cpp genetic_client_net.cpp client_protocol.c protocol_utils.c landscape.c vector.c matrix.c
OBJECTS=$(patsubst %.c,build/%.o,$(filter %.c,$(SOURCES))) $(patsubst %.cpp,build/%.o,$(filter %.cpp,$(SOURCES)))

TARGET=bin/morrigan_genetic_client

all: dirs $(TARGET)

dirs:
	@mkdir -p bin
	@mkdir -p build

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

build/%.o: %.c	
	@$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f build/*.o ./bin/morrigan_genetic_client