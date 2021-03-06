LIB_SRC = $(wildcard src/skicka/*.c) $(wildcard src/extern/*/**.c)
LIB_OBJS = $(patsubst %.c,%.o,$(LIB_SRC)) 
LIB_HEADERS = $(wildcard src/skicka/*.h) $(wildcard src/extern/*/**.h)

TEST_SRC = $(wildcard src/test/unittests/*.c)
TEST_OBJS = $(patsubst %.c,%.o,$(TEST_SRC)) 
TEST_HEADERS = $(wildcard src/test/unittests/*.h)

LIB_DIR = build
LIB_NAME = skicka

AR = ar
ARFLAGS = rcs
CC = gcc
CFLAGS = -Wall -O3 -std=c99 -Isrc/extern/jansson -c
LOADLIBES = -L./

all: $(TEST_OBJS) $(LIB_OBJS) $(LIB_HEADERS)
	mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $(LIB_DIR)/lib$(LIB_NAME).a $(LIB_OBJS)
	$(CC) $(TEST_OBJS) -o build/unittests -L$(LIB_DIR) -l$(LIB_NAME) -lcurl

$(LIB_OBJS) : $(LIB_SRC) $(LIB_HEADERS)

$(TEST_OBJS) : $(TEST_SRC) $(TEST_HEADERS)

clean:
	rm -rf $(LIB_DIR)
	rm -f $(LIB_OBJS)
	rm -f $(TEST_OBJS)