CC = cc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

OBJS = src/main.o src/filesystem.o src/options.o src/path.o src/entries.o src/sorting.o src/display.o

.PHONY: all clean

all: myls

myls: $(OBJS); $(CC) $(CFLAGS) -o myls $(OBJS)

src/main.o: src/main.c include/filesystem.h include/options.h include/entries.h include/sorting.h include/display.h; $(CC) $(CFLAGS) -c src/main.c -o src/main.o

src/filesystem.o: src/filesystem.c include/filesystem.h include/options.h include/entries.h include/sorting.h include/display.h; $(CC) $(CFLAGS) -c src/filesystem.c -o src/filesystem.o

src/options.o: src/options.c include/options.h; $(CC) $(CFLAGS) -c src/options.c -o src/options.o

src/path.o: src/path.c include/filesystem.h include/options.h include/display.h; $(CC) $(CFLAGS) -c src/path.c -o src/path.o

src/entries.o: src/entries.c include/entries.h; $(CC) $(CFLAGS) -c src/entries.c -o src/entries.o

src/sorting.o: src/sorting.c include/sorting.h include/entries.h include/options.h; $(CC) $(CFLAGS) -c src/sorting.c -o src/sorting.o

src/display.o: src/display.c include/display.h include/entries.h include/options.h; $(CC) $(CFLAGS) -c src/display.c -o src/display.o

clean:; rm -f myls $(OBJS)
