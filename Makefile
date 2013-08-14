LIB_SRC = $(wildcard src/skicka/*.c) $(wildcard src/extern/*/**.c)
LIB_OBJS = $(patsubst %.c,%.o,$(LIB_SRC)) 

LIB_HEADERS = $(wildcard src/skicka/*.h) $(wildcard src/extern/*/**.h)
LIB_NAME = libskicka.a


AR = ar
ARFLAGS = rcs
CC = gcc
CFLAGS = -Wall -O3 -std=c99 -Isrc/extern/jansson -c
LOADLIBES = -L./

all: $(LIB_OBJS) $(LIB_HEADERS)
	$(AR) $(ARFLAGS) $(LIB_NAME) $(LIB_OBJS)

clean:
	rm -f $(LIB_OBJS)