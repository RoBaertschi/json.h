

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static jmp_buf on_assert;
static char* failed_assertion = NULL;

void local_assert(bool assertion, char *message) {
    if (!assertion) {
        failed_assertion = message;
        longjmp(on_assert, -1);
    }
}

#define ARENA_ASSERT(cond) local_assert(cond, "ARENA ASSERTION FAILED " #cond)
#define ARENA_IMPLEMENTATION
#include "arena.h"

static Arena default_arena = {0};
// static Arena temp_arena = {0};
static Arena *context_arena = &default_arena;

void *context_alloc(size_t size) {
    local_assert(context_arena, "invalid context pointer");
    return arena_alloc(context_arena, size);
}

void *context_realloc(void *ptr, size_t old_size, size_t size) {
    local_assert(context_arena, "invalid context pointer");
    return arena_realloc(context_arena, ptr, old_size, size);
}

void *context_calloc(size_t count, size_t size) {
    local_assert(context_arena, "invalid context pointer");
    void *ptr = arena_alloc(context_arena, size*count);
    if (ptr != NULL) {
        memset(ptr, 0, size*count);
    }
    return ptr;
}

// Does nothing, this is a arena
void context_free(void *ptr) {}

#define JSON_MALLOC(size) context_alloc(size)
#define JSON_CALLOC(count, size) context_calloc(count, size)
#define JSON_REALLOC(ptr, old_size, size) context_realloc(ptr, old_size, size)
#define JSON_FREE(ptr) context_free(ptr)
#define JSON_ASSERT(cond, message) local_assert(cond, message)
#include "json.h"

int main(void) {
    int result = setjmp(on_assert);
    if (result != 0) {
        if (failed_assertion != NULL) {
            fprintf(stderr, "TEST FAILED: ASSERTION FAILED %s\n", failed_assertion);
        } else {
            fprintf(stderr, "TEST FAILED: ASSERTION FAILED\n");
        }
        return 1;
    }

    // -------
    // Objects
    // -------
    bool ok = true;
    struct json_object obj = json_object_create(&ok);
    local_assert(ok, "could not create object");

    bool found = true;
    json_object_get(&obj, JSON_STR("deez"), &found);
    local_assert(!found, "found non existing key in object");

    #define HELLO "hello"
    json_object_set(&obj, JSON_STR("deez"), json_string_create_cstrv(HELLO, &ok));
    local_assert(ok, "could net set json_string('hello') for key 'deez'");
    json_object_get(&obj, JSON_STR("deez"), &found);
    local_assert(found, "could not find existing key in object");

    ok = true;
    struct json_object obj2 = json_object_copy(obj, &ok);
    local_assert(ok, "could not make a copy of obj");
    json_object_get(&obj2, JSON_STR("deez"), &found);
    local_assert(found, "could not find existing key in copied object");
    json_object_delete(obj2);

    struct json_value val = json_object_get(&obj, JSON_STR("deez"), &found);
    local_assert(found, "could not find existing key in object");
    local_assert(val.type == JSON_STRING, "value from object is not string after copy");
    local_assert(val.data.string.data[0] == HELLO[0], "");

    found = json_object_del(&obj, JSON_STR("deez"));
    local_assert(found, "could not delete existing key in object");

    json_object_delete(obj);

    arena_free(context_arena);
}
