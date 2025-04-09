#include "json.h"
#include "stdint.h"
#include <stdio.h>

#define FALSE "false"
#define TRUE "true"
#define null "null"

unsigned char* p(char* ptr) {
    return (unsigned char*) p;
}

int main(void) {
    printf(
        "static size_t false_hash = %zu;\n"
        "static size_t true_hash = %zu;\n"
        "static size_t null_hash = %zu;\n",
        json__hash(p(FALSE), sizeof(FALSE) - 1),
        json__hash(p(TRUE), sizeof(TRUE) - 1),
        json__hash(p(NULL), sizeof(NULL) - 1)
    );


    return 0;
}
