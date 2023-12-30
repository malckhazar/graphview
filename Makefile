CC=gcc

DEPS=gtk4
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:%.c=%.o)
CFLAGS=-g `pkg-config --cflags $(DEPS)`
LDFLAGS=`pkg-config --libs-only-L $(DEPS)`
LIBS=`pkg-config --libs-only-l $(DEPS)`

TARGET=graphview

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJECTS) -o $(TARGET)

all: $(TARGET)

clean:
	rm -rf $(OBJECTS) $(TARGET)

.PHONY: all
