CFLAGS=-std=c99 -pedantic -Wall -Werror -DJSON_IMPLEMENTATION

test: test.c json.h
	$(CC) $(CFLAGS) test.c -o test

test_san: test.c json.h
	$(CC) $(CFLAGS) -fsanitize=address,undefined,leak,integer test.c -o

.PHONY: clean

clean:
	rm -f test
