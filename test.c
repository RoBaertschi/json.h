#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


void local_assert(bool assertion, char *message);

#define ARENA_ASSERT(cond) local_assert(cond, "ARENA ASSERTION FAILED " #cond)
#define ARENA_IMPLEMENTATION
#include "arena.h"

static Arena default_arena = {0};
// static Arena temp_arena = {0};
static Arena *context_arena = &default_arena;

static jmp_buf on_assert;
static char* failed_assertion = NULL;

void local_assert(bool assertion, char *message) {
    if (!assertion) {
        failed_assertion = message;
        longjmp(on_assert, -1);
    }
}


void *context_alloc(size_t size);

void local_assert_eq(bool result, char *left, char *right) {
    if (!result) {
        local_assert(context_arena, "invalid context pointer");
        failed_assertion = arena_sprintf(context_arena, "%s == %s", left, right);
        longjmp(on_assert, -1);
    }
}

#define ASSERT(assertion) local_assert(assertion, "ASSERTION " #assertion " FAILED")
#define ASSERT_EQ(left, right) local_assert_eq(left == right, #left, #right)
#define ASSERT_EQ_INT(left, right) local_assert_eq((left) == (right), arena_sprintf(context_arena, "%d", (left)), arena_sprintf(context_arena, "%d", (right)))
#define ASSERT_EQ_SIZE_T(left, right) local_assert_eq((left) == (right), arena_sprintf(context_arena, "%zu", (left)), arena_sprintf(context_arena, "%zu", (right)))


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

#define HELLO "hello"

void object_bulk_test(void);

void loc_assert_eq(struct json_loc expected, struct json_loc actual) {
    ASSERT_EQ_SIZE_T(expected.row, actual.row);
    ASSERT_EQ_SIZE_T(expected.col, actual.col);
    ASSERT_EQ_SIZE_T(expected.pos, actual.pos);
}

void token_assert_eq(struct json_token expected, struct json_token actual) {
    ASSERT_EQ_INT(expected.type, actual.type);
    ASSERT_EQ_SIZE_T(expected.len, actual.len);
    switch (expected.type) {
    case JSON_TOKEN_IDENTIFIER:
        ASSERT(json_string_eq(expected.data.string, actual.data.string));
        break;
    case JSON_TOKEN_NUMBER:
        ASSERT_EQ(expected.data.number, actual.data.number);
    default:
        break;
    }
}

void token_loc_assert_eq(struct json_token expected, struct json_token actual) {
    token_assert_eq(expected, actual);
    loc_assert_eq(expected.loc, actual.loc);
}

void lexer_tests(void) {
    struct json_lexer l = {0};

    { // Test simple
        json_lexer_init(&l, JSON_STR("{}"), JSON_STR("test.json"), JSON_SPEC_JSON);
        local_assert(l.pos == 0, "pos is not initalized correctly");
        local_assert(l.read_pos == 1, "read_pos is not initalized correctly");
        local_assert(l.ch == '{', "ch is not initalized correctly");
        local_assert(l.row == 1, "row has to be 1");
        local_assert(l.col == 1, "col has to be 1");
        struct json_token expected[] = {
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 1,
                    .col = 1,
                    .pos = 0,
                },
                .type = JSON_TOKEN_LBRACE,
                .len = 1,
            },
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 1,
                    .col = 2,
                    .pos = 1,
                },
                .type = JSON_TOKEN_RBRACE,
                .len = 1,
            },
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 1,
                    .col = 3,
                    .pos = 2,
                },
                .type = JSON_TOKEN_EOF,
                .len = 1,
            },
        };

        bool ok = true;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
            struct json_token token = json_lexer_next_token(&l, &ok);
            local_assert(ok, "should be ok");
            token_loc_assert_eq(expected[i], token);
        }
        json_lexer_deinit(&l);
    }

    { // Test whitespace
        json_lexer_init(&l, JSON_STR("{\n}"), JSON_STR("test.json"), JSON_SPEC_JSON);
        struct json_token expected[] = {
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 1,
                    .col = 1,
                    .pos = 0,
                },
                .type = JSON_TOKEN_LBRACE,
                .len = 1,
            },
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 2,
                    .col = 1,
                    .pos = 2,
                },
                .type = JSON_TOKEN_RBRACE,
                .len = 1,
            },
            (struct json_token){
                .loc = (struct json_loc) {
                    .row = 2,
                    .col = 2,
                    .pos = 3,
                },
                .type = JSON_TOKEN_EOF,
                .len = 1,
            },
        };

        bool ok = true;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
            struct json_token token = json_lexer_next_token(&l, &ok);
            local_assert(ok, "should be ok");
            token_loc_assert_eq(expected[i], token);
        }
    }

    json_lexer_deinit(&l);
}

int main(void) {
    int result = setjmp(on_assert);
    if (result != 0) {
        if (failed_assertion != NULL) {
            fprintf(stderr, "TEST FAILED: ASSERTION FAILED %s\n", failed_assertion);
        } else {
            fprintf(stderr, "TEST FAILED: ASSERTION FAILED\n");
        }

        arena_free(context_arena);
        return 1;
    }

    // -------
    // Objects
    // -------
    
    object_bulk_test();

    bool ok = true;
    struct json_object obj = json_object_create(&ok);
    local_assert(ok, "could not create object");

    bool found = true;
    json_object_get(&obj, JSON_STR("deez"), &found);
    local_assert(!found, "found non existing key in object");

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

    struct json_value value = json_object_get(&obj, JSON_STR("deez"), &found);
    local_assert(found, "could not find existing key in object");
    local_assert(value.type == JSON_STRING, "not json string");

    struct json_object_iterator it = json_object_iterator_create(&obj);
    struct json_object_entry entry = json_object_iterator_next(&it);
    local_assert(entry.found, "the iterator should find one entry");
    local_assert(json_string_eq(entry.key, JSON_STR("deez")), "invalid key from iterator entry");

    found = json_object_del(&obj, JSON_STR("deez"));
    local_assert(found, "could not delete existing key in object");

    json_object_delete(obj);

    // ------
    // Arrays
    // ------

    struct json_array arr = json_array_create();
    local_assert(arr.len == 0 && arr.items == NULL, "an array initalized with array_create should be completly empty");
    json_array_delete(arr);

    arr = JSON_CREATE_ARRAY(&ok, json_null(), json_boolean(true), json_null());
    local_assert(ok, "array should be successfully created");
    local_assert(arr.items[0].type == JSON_NULL, "member 0 should be null");
    local_assert(arr.items[1].type == JSON_BOOLEAN
                 && arr.items[1].data.boolean, "member 1 should be true");
    local_assert(arr.items[2].type == JSON_NULL, "member 2 should be null");

    struct json_array arr2 = json_array_concat(JSON_CREATE_TEMP_ARRAY(json_number(2)), arr, &ok);
    local_assert(ok, "concat should succeed");
    local_assert(arr2.items[0].type == JSON_NUMBER, "member 0 should be number after concat");
    local_assert(arr2.items[0].data.number == 2, "member 0 should have value of 2 after concat");

    json_array_delete(arr2);
    json_array_delete(arr);

    // -----
    // Lexer
    // -----
    
    lexer_tests();

    arena_free(context_arena);
}


void object_bulk_test(void) {
    bool ok = true;
    struct json_object obj = json_object_create(&ok);
    local_assert(ok, "could not create object");

    json_object_set(&obj, JSON_STR("deez0"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez0 = json_object_get(&obj, JSON_STR("deez0"), &ok);
    local_assert(ok, "deez0 not found");
    local_assert(json_string_eq(deez0.data.string, JSON_STR(HELLO)), "deez0 invalid string content");
    json_object_set(&obj, JSON_STR("deez1"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez1 = json_object_get(&obj, JSON_STR("deez1"), &ok);
    local_assert(ok, "deez1 not found");
    local_assert(json_string_eq(deez1.data.string, JSON_STR(HELLO)), "deez1 invalid string content");
    json_object_set(&obj, JSON_STR("deez2"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez2 = json_object_get(&obj, JSON_STR("deez2"), &ok);
    local_assert(ok, "deez2 not found");
    local_assert(json_string_eq(deez2.data.string, JSON_STR(HELLO)), "deez2 invalid string content");
    json_object_set(&obj, JSON_STR("deez3"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez3 = json_object_get(&obj, JSON_STR("deez3"), &ok);
    local_assert(ok, "deez3 not found");
    local_assert(json_string_eq(deez3.data.string, JSON_STR(HELLO)), "deez3 invalid string content");
    json_object_set(&obj, JSON_STR("deez4"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez4 = json_object_get(&obj, JSON_STR("deez4"), &ok);
    local_assert(ok, "deez4 not found");
    local_assert(json_string_eq(deez4.data.string, JSON_STR(HELLO)), "deez4 invalid string content");
    json_object_set(&obj, JSON_STR("deez5"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez5 = json_object_get(&obj, JSON_STR("deez5"), &ok);
    local_assert(ok, "deez5 not found");
    local_assert(json_string_eq(deez5.data.string, JSON_STR(HELLO)), "deez5 invalid string content");
    json_object_set(&obj, JSON_STR("deez6"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez6 = json_object_get(&obj, JSON_STR("deez6"), &ok);
    local_assert(ok, "deez6 not found");
    local_assert(json_string_eq(deez6.data.string, JSON_STR(HELLO)), "deez6 invalid string content");
    json_object_set(&obj, JSON_STR("deez7"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez7 = json_object_get(&obj, JSON_STR("deez7"), &ok);
    local_assert(ok, "deez7 not found");
    local_assert(json_string_eq(deez7.data.string, JSON_STR(HELLO)), "deez7 invalid string content");
    json_object_set(&obj, JSON_STR("deez8"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez8 = json_object_get(&obj, JSON_STR("deez8"), &ok);
    local_assert(ok, "deez8 not found");
    local_assert(json_string_eq(deez8.data.string, JSON_STR(HELLO)), "deez8 invalid string content");
    json_object_set(&obj, JSON_STR("deez9"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez9 = json_object_get(&obj, JSON_STR("deez9"), &ok);
    local_assert(ok, "deez9 not found");
    local_assert(json_string_eq(deez9.data.string, JSON_STR(HELLO)), "deez9 invalid string content");
    json_object_set(&obj, JSON_STR("deez10"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez10 = json_object_get(&obj, JSON_STR("deez10"), &ok);
    local_assert(ok, "deez10 not found");
    local_assert(json_string_eq(deez10.data.string, JSON_STR(HELLO)), "deez10 invalid string content");
    json_object_set(&obj, JSON_STR("deez11"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez11 = json_object_get(&obj, JSON_STR("deez11"), &ok);
    local_assert(ok, "deez11 not found");
    local_assert(json_string_eq(deez11.data.string, JSON_STR(HELLO)), "deez11 invalid string content");
    json_object_set(&obj, JSON_STR("deez12"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez12 = json_object_get(&obj, JSON_STR("deez12"), &ok);
    local_assert(ok, "deez12 not found");
    local_assert(json_string_eq(deez12.data.string, JSON_STR(HELLO)), "deez12 invalid string content");
    json_object_set(&obj, JSON_STR("deez13"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez13 = json_object_get(&obj, JSON_STR("deez13"), &ok);
    local_assert(ok, "deez13 not found");
    local_assert(json_string_eq(deez13.data.string, JSON_STR(HELLO)), "deez13 invalid string content");
    json_object_set(&obj, JSON_STR("deez14"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez14 = json_object_get(&obj, JSON_STR("deez14"), &ok);
    local_assert(ok, "deez14 not found");
    local_assert(json_string_eq(deez14.data.string, JSON_STR(HELLO)), "deez14 invalid string content");
    json_object_set(&obj, JSON_STR("deez15"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez15 = json_object_get(&obj, JSON_STR("deez15"), &ok);
    local_assert(ok, "deez15 not found");
    local_assert(json_string_eq(deez15.data.string, JSON_STR(HELLO)), "deez15 invalid string content");
    json_object_set(&obj, JSON_STR("deez16"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez16 = json_object_get(&obj, JSON_STR("deez16"), &ok);
    local_assert(ok, "deez16 not found");
    local_assert(json_string_eq(deez16.data.string, JSON_STR(HELLO)), "deez16 invalid string content");
    json_object_set(&obj, JSON_STR("deez17"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez17 = json_object_get(&obj, JSON_STR("deez17"), &ok);
    local_assert(ok, "deez17 not found");
    local_assert(json_string_eq(deez17.data.string, JSON_STR(HELLO)), "deez17 invalid string content");
    json_object_set(&obj, JSON_STR("deez18"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez18 = json_object_get(&obj, JSON_STR("deez18"), &ok);
    local_assert(ok, "deez18 not found");
    local_assert(json_string_eq(deez18.data.string, JSON_STR(HELLO)), "deez18 invalid string content");
    json_object_set(&obj, JSON_STR("deez19"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez19 = json_object_get(&obj, JSON_STR("deez19"), &ok);
    local_assert(ok, "deez19 not found");
    local_assert(json_string_eq(deez19.data.string, JSON_STR(HELLO)), "deez19 invalid string content");
    json_object_set(&obj, JSON_STR("deez20"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez20 = json_object_get(&obj, JSON_STR("deez20"), &ok);
    local_assert(ok, "deez20 not found");
    local_assert(json_string_eq(deez20.data.string, JSON_STR(HELLO)), "deez20 invalid string content");
    json_object_set(&obj, JSON_STR("deez21"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez21 = json_object_get(&obj, JSON_STR("deez21"), &ok);
    local_assert(ok, "deez21 not found");
    local_assert(json_string_eq(deez21.data.string, JSON_STR(HELLO)), "deez21 invalid string content");
    json_object_set(&obj, JSON_STR("deez22"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez22 = json_object_get(&obj, JSON_STR("deez22"), &ok);
    local_assert(ok, "deez22 not found");
    local_assert(json_string_eq(deez22.data.string, JSON_STR(HELLO)), "deez22 invalid string content");
    json_object_set(&obj, JSON_STR("deez23"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez23 = json_object_get(&obj, JSON_STR("deez23"), &ok);
    local_assert(ok, "deez23 not found");
    local_assert(json_string_eq(deez23.data.string, JSON_STR(HELLO)), "deez23 invalid string content");
    json_object_set(&obj, JSON_STR("deez24"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez24 = json_object_get(&obj, JSON_STR("deez24"), &ok);
    local_assert(ok, "deez24 not found");
    local_assert(json_string_eq(deez24.data.string, JSON_STR(HELLO)), "deez24 invalid string content");
    json_object_set(&obj, JSON_STR("deez25"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez25 = json_object_get(&obj, JSON_STR("deez25"), &ok);
    local_assert(ok, "deez25 not found");
    local_assert(json_string_eq(deez25.data.string, JSON_STR(HELLO)), "deez25 invalid string content");
    json_object_set(&obj, JSON_STR("deez26"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez26 = json_object_get(&obj, JSON_STR("deez26"), &ok);
    local_assert(ok, "deez26 not found");
    local_assert(json_string_eq(deez26.data.string, JSON_STR(HELLO)), "deez26 invalid string content");
    json_object_set(&obj, JSON_STR("deez27"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez27 = json_object_get(&obj, JSON_STR("deez27"), &ok);
    local_assert(ok, "deez27 not found");
    local_assert(json_string_eq(deez27.data.string, JSON_STR(HELLO)), "deez27 invalid string content");
    json_object_set(&obj, JSON_STR("deez28"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez28 = json_object_get(&obj, JSON_STR("deez28"), &ok);
    local_assert(ok, "deez28 not found");
    local_assert(json_string_eq(deez28.data.string, JSON_STR(HELLO)), "deez28 invalid string content");
    json_object_set(&obj, JSON_STR("deez29"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez29 = json_object_get(&obj, JSON_STR("deez29"), &ok);
    local_assert(ok, "deez29 not found");
    local_assert(json_string_eq(deez29.data.string, JSON_STR(HELLO)), "deez29 invalid string content");
    json_object_set(&obj, JSON_STR("deez30"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez30 = json_object_get(&obj, JSON_STR("deez30"), &ok);
    local_assert(ok, "deez30 not found");
    local_assert(json_string_eq(deez30.data.string, JSON_STR(HELLO)), "deez30 invalid string content");
    json_object_set(&obj, JSON_STR("deez31"), json_string_create_cstrv(HELLO, &ok));
    struct json_value deez31 = json_object_get(&obj, JSON_STR("deez31"), &ok);
    local_assert(ok, "deez31 not found");
    local_assert(json_string_eq(deez31.data.string, JSON_STR(HELLO)), "deez31 invalid string content");

    json_object_delete(obj);
}
