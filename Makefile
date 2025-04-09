CFLAGS=-std=c99 -pedantic -Wall -Werror -DJSON_IMPLEMENTATION

.PHONY: ALL

ALL: tests comp_hashes

test: tests
	./tests

tests: test.c json.h
	$(CC) $(CFLAGS) test.c -o tests

tests_san: test.c json.h
	$(CC) $(CFLAGS) -fsanitize=address,undefined,leak,integer test.c -o tests_san

comp_hashes: comp_hashes.c

.PHONY: clean

clean:
	rm -f tests*
