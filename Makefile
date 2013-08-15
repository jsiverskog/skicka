LIB_SRC = $(wildcard src/skicka/*.c) $(wildcard src/extern/*/**.c)
LIB_OBJS = $(patsubst %.c,%.o,$(LIB_SRC)) 

LIB_HEADERS = $(wildcard src/skicka/*.h) $(wildcard src/extern/*/**.h)
LIB_DIR = build
LIB_NAME = libskicka.a

AR = ar
ARFLAGS = rcs
CC = gcc
CFLAGS = -Wall -O3 -std=c99 -Isrc/extern/jansson -c
LOADLIBES = -L./

all: $(LIB_OBJS) $(LIB_HEADERS)
	mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $(LIB_DIR)/$(LIB_NAME) $(LIB_OBJS)

clean:
	rm -rf $(LIB_DIR)
	rm -f $(LIB_OBJS)