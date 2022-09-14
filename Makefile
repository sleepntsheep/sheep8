CFLAGS := -std=c11
INCLUDE := -Iinclude
LIBS := -lraylib -lSDL2 -lm
SRCS := *.c

all: sheep8

sheep8: *.c *.h
	cc $(SRCS) -o $@ $(CFLAGS) $(LIBS) $(INCLUDE)
