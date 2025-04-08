CFLAGS=-xc-header -std=c99 -pedantic -Wall -Werror -DJSON_IMPLEMENTATION

test: test.c json.h
	$(CC) $(CFLAGS) test.c -o test

.PHONY: clean

clean:
	rm -f test
