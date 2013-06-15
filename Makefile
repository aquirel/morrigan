SLASHPATH="D:/dev/_libz/slash-a/lib"

COMMONFLAGS=-g -O0 -Wall -Wextra -DWINVER=0x0501
CC=gcc
CFLAGS=-std=gnu11 $(COMMONFLAGS)
CXX=g++
CXXFLAGS=-std=gnu++11 -I$(SLASHPATH) $(COMMONFLAGS)

LDFLAGS=-L$(SLASHPATH) -static -static-libgcc -static-libstdc++ -Xlinker --export-all-symbols
LIBS=-lm -lslasha -lws2_32

SOURCES=genetic_client_main.cpp genetic_client_net.cpp client_protocol.c protocol_utils.c landscape.c vector.c matrix.c
HEADERS=client_protocol.h debug.h matrix.h minmax.h protocol.h protocol_utils.h tank_defines.h vector.h genetic_client.hpp genetic_client_commands.hpp genetic_client_net.hpp

OBJECTS=$(patsubst %.c,build/%.o,$(filter %.c,$(SOURCES))) $(patsubst %.cpp,build/%.o,$(filter %.cpp,$(SOURCES)))

TARGET=bin/morrigan_genetic_client

all: dirs $(TARGET)

dirs:
	@mkdir -p bin
	@mkdir -p build

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

build/%.o: %.c $(HEADERS)
	@$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.cpp $(HEADERS)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f build/*.o ./bin/morrigan_genetic_client
