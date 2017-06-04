CPPFLAGS = -DGTREEEX_DEBUG
CFLAGS = -g -O0 -Wall $(shell pkg-config --cflags glib-2.0)
LDFLAGS = $(shell pkg-config --libs glib-2.0)

all: test

test: test.c

