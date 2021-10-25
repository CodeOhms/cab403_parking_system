CC = gcc
CFLAGS = -g -I./include -Wall -pedantic # Show all reasonable warnings
LDFLAGS = -lrt -pthread
BUILD_DIR ?= ./build
OBJECTS = shared_memory.o linked_list.o thread_pool.o car_park_simulator.o # Object files for building simulator
OBJECTS2 = shared_memory.o car_park_manager.o # Object files for building manager
TARGET = car_park_simulator
TARGET2 = car_park_manager

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET).out $(OBJECTS) $(LDFLAGS)

$(TARGET2): $(OBJECTS2)
	$(CC) $(CFLAGS) -o $(TARGET2).out $(OBJECTS2) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(OBJECTS2) $(TARGET).out $(TARGET2).out

.PHONY: all clean