#ifndef JSON_H_INC
#define JSON_H_INC

// This is json.h by RoBaertschi, a simple, single header json parser. Written in C99.
// Define JSON_IMPLEMENTATION to enable the implementation
//
// Defines:
//  JSON_ASSERT(condition, message)
//  Default: assert(condition && message)
//  Includes by default: assert.h
//  Assert function, should check if condition is true. It should abort in debug mode
//
//  JSON_STATIC_ASSERT
//  Default: static_assert
//  Includes by default: assert.h
//  Do some static assert
//
//  JSON_GROWTH_FACTOR
//  Default: 2
//  The multiplier by which the internal hash map capacity increases, when it runs out of capacity. A good value is 1.5 - 2.
//
//  JSON_MAX_LOAD_FACTOR
//  Default: 0.7
//  The precentage at which the internal hash map increases the bucket capacity
//
//  JSON_INITIAL_BUCKET_SIZE
//  Default: 16
//  This is the Default size of a Bucket for a json_object. The larger the Bucket Size, the faster the access but higher Memory Use.
//
//  JSON_{MALLOC,CALLOC,REALLOC,FREE}
//  Defaults: stdlib.h
//  These are functions used to allocate memory. If you want to use some sort of custom allocator, this is the place to add it.
//  NOTE: There might be a point where we add full custom allocator support with user data support.
//  That would probably use new functions with a allocator user data pointer for each allocation or some sort of allocator interface.
//

#include <stdio.h>
#include <string.h>

#ifndef JSON_STATIC_ASSERT
#include <assert.h>
#define JSON_STATIC_ASSERT static_assert
#endif // JSON_STATIC_ASSERT

#ifndef JSON_INITIAL_BUCKET_SIZE
#define JSON_INITIAL_BUCKET_SIZE 16
#endif // JSON_INITIAL_BUCKET_SIZE

// TODO: C11
// JSON_STATIC_ASSERT(JSON_INITIAL_BUCKET_SIZE > 0, "JSON_INITIAL_BUCKET_SIZE has to be larger than 0");

#ifndef JSON_GROWTH_FACTOR
#define JSON_GROWTH_FACTOR 2
#endif // JSON_GROWTH_FACTOR

// TODO: C11
// JSON_STATIC_ASSERT(JSON_GROWTH_FACTOR > 0, "JSON_GROWTH_FACTOR has to be larger than 0");

#ifndef JSON_MAX_LOAD_FACTOR
#define JSON_MAX_LOAD_FACTOR 0.7
#endif // JSON_MAX_LOAD_FACTOR


#ifndef JSON_MALLOC
#include <stdlib.h>
#define JSON_MALLOC(size) malloc(size)
#endif // JSON_MALLOC

#ifndef JSON_CALLOC
#include <stdlib.h>
#define JSON_CALLOC(count,size) calloc(count, size)
#endif // JSON_CALLOC

#ifndef JSON_REALLOC
#include <stdlib.h>
#define JSON_REALLOC(ptr, old_size, size) realloc(ptr, size)
#endif // JSON_REALLOC

#ifndef JSON_FREE
#include <stdlib.h>
#define JSON_FREE(ptr) free(ptr)
#endif // JSON_FREE

#ifndef JSON_ASSERT
#include <assert.h>
#define JSON_ASSERT(condition, message) assert(condition && message)
#endif // JSON_ASSERT 

#ifndef JSON_ASSERT_NO_MSG
#include <assert.h>
#define JSON_ASSERT_NO_MSG(condition) assert(condition && #condition)
#endif // JSON_ASSERT 

#include <stddef.h>
#include <stdbool.h>

enum json_type {
    // Invalid json value, a zero constructed value is invalid
    JSON_INVALID,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_NUMBER,
    JSON_BOOLEAN,
    JSON_STRING,
    JSON_NULL,
};


// array with len 0 do not have any heap memory, items will be set to NULL
struct json_array {
    size_t len;
    struct json_value *items;
};

struct json_string {
    size_t len;
    unsigned char *data;
};

// NOTE: The content of json_object is internal, you should not use it.
struct json_object {
    // NOTE: Internal API, to access please use the public api functions
    struct json__hash_map* _hm;
};

// NOTE: The content of json_object_iterator is internal, you should not use it.
// WARN: Do not modify the object while iterating, some weird stuff can happen if you do.
struct json_object_iterator {
    size_t _bucket_index;
    struct json__hash_map_entry *_current_entry;
    struct json__hash_map *_hm;
};

// WARN: Do not modify the object while iterating, some weird stuff can happen if you do.
struct json_object_entry {
    // The key is not a pointer, because it already is just a pointer with a length
    struct json_string key;
    struct json_value* value;
    bool found;
};

struct json_value {
    enum json_type type;
    // NOTE: JSON_NULL does not have any data.
    union json_data {
        bool boolean;
        double number;
        struct json_object object;
        struct json_array array;
        struct json_string string;
    } data ;
};


//------------------------------
// OBJECT PUBLIC API
//------------------------------

// Allocates a new object, use the other json_object_* functions to add and remove attributes.
// If any of the allocations fails, ok will be set to false. On success, ok will be set to true.
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_object json_object_create(bool *ok);
struct json_value json_object_to_value(struct json_object obj);

// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_object json_object_copy(struct json_object obj, bool *ok);
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_value json_object_copyv(struct json_object obj, bool *ok);

// Modifying the object

// Sets or inserts on demand, a value for a key
// The key will be copied, the value will be taken as is, the value will be owned by the object afterwards.
// Returns false if there was an allocation error.
bool json_object_set(struct json_object *obj, struct json_string key, struct json_value value);

// Gets the value at the key.
struct json_value json_object_get(struct json_object *obj, struct json_string key, bool *found);

// Deletes key from the object
// Returns true if the key was found, false if not
bool json_object_del(struct json_object *obj, struct json_string key);

// WARN: Do not modify the object while iterating, some weird stuff can happen if you do.
struct json_object_iterator json_object_iterator_create(struct json_object *obj);
// WARN: Do not modify the object while iterating, some weird stuff can happen if you do.
struct json_object_entry json_object_iterator_next(struct json_object_iterator* iterator);

//------------------------------
// STRING PUBLIC API
//------------------------------

// Makes a copy of the str
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_string json_string_create(unsigned char *str, size_t len, bool *ok);
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_value json_string_createv(unsigned char *str, size_t len, bool *ok);

struct json_string json_string_create_empty(void);
struct json_value json_string_create_emptyv(void);

// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_string json_string_create_cstr(unsigned char *str, bool *ok);
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_value json_string_create_cstrv(char *str, bool *ok);

// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_string json_string_copy(struct json_string str, bool *ok);
// NOTE: ok has to be provided, if it null, an assertion will fail.
struct json_value json_string_copyv(struct json_string str, bool *ok);

// Do not use the str passed into this function, the returnec value has taken ownership of str
struct json_value json_string_to_value(struct json_string str);

#define JSON_STR(cstr) (struct json_string) { .data = (unsigned char *) cstr, .len = (sizeof(cstr) / sizeof(cstr[0])) - 1 }


// string
bool json_string_eq(struct json_string first, struct json_string second);

//------------------------------
// ARRAY PUBLIC API
//------------------------------

// Create a new, empty array. This is not allocated.
struct json_array json_array_create(void);

// Allocates a new array and copies the elements over, mostly for a utility macro
// If the arr.len > 0 and the return.items == NULL. Then the allocation failed. When a allocation fails, the returned array will have a length of 0.
struct json_array json_array_copy(struct json_array arr, bool *ok);
struct json_value json_array_copyv(struct json_array arr, bool *ok);

// Allocates a new array and copies both left and right into it
// If the left.len + right.len > 0 and the return.items == NULL. Then the allocation failed. When a allocation fails, the returned array will have a length of 0.
struct json_array json_array_concat(struct json_array left, struct json_array right, bool *ok);
struct json_value json_array_concatv(struct json_array left, struct json_array right, bool *ok);

#define JSON_CREATE_TEMP_ARRAY(...) (struct json_array) { .items = (struct json_value[]){ __VA_ARGS__ }, .len = sizeof((struct json_value[]){ __VA_ARGS__ }) / sizeof(struct json_value) }

// This macro allows for easy creation of a array's with values, because currently json_array's are immutable (100% not because I am lazy to implement the proper functions or something /s).
#define JSON_CREATE_ARRAY(ok, ...) json_array_copy(JSON_CREATE_TEMP_ARRAY(__VA_ARGS__), (ok))

struct json_value json_array_to_value(struct json_array arr);

//------------------------------
// OTHER CREATE PUBLIC API
//------------------------------

// This function cannot fail
struct json_value json_boolean(bool value);
// This function cannot fail
struct json_value json_number(double value);
// This function cannot fail
struct json_value json_null(void);

struct json_value json_value_copy(struct json_value value, bool *ok);

// Free
void json_value_delete(struct json_value value);
void json_object_delete(struct json_object object);
void json_array_delete(struct json_array array);
void json_string_delete(struct json_string string);


//------------------------------
// LEXER
//------------------------------

enum json_spec {
    // json.org
    JSON_SPEC_JSON,
    // json with comments
    JSON_SPEC_JSONC,
    // json5.org
    JSON_SPEC_JSON5,
};

struct json_loc {
    // Row/Line
    size_t row; // Starts at 1
    // Column
    size_t col; // Starts at 1

    // Index in the source string
    size_t pos; // Starts at 0
};

enum json_token_type {
    JSON_TOKEN_INVALID,
    JSON_TOKEN_EOF,

    JSON_TOKEN_NUMBER,
    JSON_TOKEN_STRING,

    JSON_TOKEN_LBRACE,
    JSON_TOKEN_RBRACE,
    JSON_TOKEN_LBRACKET,
    JSON_TOKEN_RBRACKET,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_COLON,

    JSON_TOKEN_IDENTIFIER,
    JSON_TOKEN_NULL,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
};

union json_token_data {
    double number;
    struct json_string string;
};

struct json_token {
    struct json_loc loc;
    size_t len;

    enum json_token_type type;
    union json_token_data data;
};

struct json_lexer {
    enum json_spec spec;

    struct json_string source_name;
    struct json_string source;

    size_t pos; // Starts at 0
    size_t read_pos; // Starts at 1
    int ch; // Why int you may ask, because EOF

    size_t row; // Starts at 1
    size_t col; // Starts at 1
};

// The string inputs are copied, you will have to delete them yourself.
bool json_lexer_init(struct json_lexer *lexer, struct json_string source, struct json_string source_name, enum json_spec spec);
void json_lexer_deinit(struct json_lexer *lexer);

struct json_token json_lexer_next_token(struct json_lexer *l, bool *ok);

#ifdef JSON_IMPLEMENTATION


//------------------------------
// INTERNAL API
//------------------------------

// hash map
static struct json__hash_map* json__hm_create(void);
static void json__hm_delete(struct json__hash_map* map);
static bool json__hash_map_insert(struct json__hash_map *hm, struct json_string key, struct json_value value);
static struct json__hash_map_entry* json__hash_map_get(struct json__hash_map *hm, struct json_string key);
static bool json__hash_map_grow(struct json__hash_map *hm);
static double json__hash_map_load_factor(struct json__hash_map *hm);
static void json__hash_map_entry_delete(struct json__hash_map_entry* entry);
static bool json__hash_map_set(struct json__hash_map *hm, struct json_string key, struct json_value value);
static bool json__hash_map_delete(struct json__hash_map *hm, struct json_string key);
static struct json__hash_map *json__hash_map_copy(struct json__hash_map *hm);

// hash map entry
static struct json__hash_map_entry json__hash_map_entry_copy(struct json__hash_map_entry entry, struct json__hash_map_entry *new_collisions, struct json__hash_map_entry *old_collisions, bool *ok);
static bool json__hash_map_entry_valid(struct json__hash_map_entry *entry);

// Internal Hash Map for strings to json_value
struct json__hash_map {
    struct json__hash_map_entry *bucket;
    size_t bucket_size;
    size_t bucket_cap;

    // This is a linked list
    struct json__hash_map_entry *collisions;
    size_t collisions_size;
    size_t collisions_cap;
};

struct json__hash_map_entry {
    struct json_string key;
    struct json_value value;
    struct json__hash_map_entry* next;
};



//------------------------------
// PUBLIC API IMPL
//------------------------------

struct json_object_iterator json_object_iterator_create(struct json_object *obj) {
    return (struct json_object_iterator) {
        ._bucket_index = 0,
        ._current_entry = NULL,
        ._hm = obj->_hm,
    };
}

struct json_object_entry json_object_iterator_next(struct json_object_iterator* iterator) {
    if (iterator->_current_entry && iterator->_current_entry->next) {
        iterator->_current_entry = iterator->_current_entry->next;
        return (struct json_object_entry) {
            .value = &iterator->_current_entry->value,
            .key = iterator->_current_entry->key,
            .found = true,
        };
    }

    for (; iterator->_bucket_index < iterator->_hm->bucket_cap; iterator->_bucket_index++) {
        iterator->_current_entry = &iterator->_hm->bucket[iterator->_bucket_index];
        if (json__hash_map_entry_valid(iterator->_current_entry)) {
            return (struct json_object_entry) {
                .value = &iterator->_current_entry->value,
                .key = iterator->_current_entry->key,
                .found = true,
            };
        }
    }
    return (struct json_object_entry) {
        .found = false,
    };
}

struct json_string json_string_create(unsigned char *str, size_t len, bool *ok) {
    JSON_ASSERT(ok != NULL, "ok has to be a valid pointer");

    if (len <= 0) {
        *ok = true;
        return json_string_create_empty();
    }

    struct json_string new_str = {
        .data = JSON_MALLOC(len),
        .len = len,
    };

    if (new_str.data == NULL) {
        *ok = false;
        new_str.len = 0;
        return new_str;
    }
    *ok = true;

    for (size_t i = 0; i < len; i++) {
        new_str.data[i] = str[i];
    }

    return new_str;
}

// NOTE: Copies the string
// The string has to be longer than 0
// Returns an empty string with NULL as data when the allocation fails
struct json_value json_string_createv(unsigned char *str, size_t len, bool *ok) {
    return json_string_to_value(json_string_create(str, len, ok));
}

inline struct json_string json_string_create_empty(void) {
    return (struct json_string) {0};
}

inline struct json_value json_string_create_emptyv(void) {
    return json_string_to_value(json_string_create_empty());
}

// NOTE: Copies the string
// The string has to be longer than 0
// Returns an empty string with NULL as data when the allocation fails
struct json_string json_string_create_cstr(unsigned char *str, bool *ok) {
    return json_string_create(str, strlen((char*) str), ok);
}
// NOTE: Copies the string
// The string has to be longer than 0
// Returns an empty string with NULL as data when the allocation fails
struct json_value json_string_create_cstrv(char *str, bool *ok) {
    return json_string_createv((unsigned char*)str, strlen(str), ok);
}

// NOTE: Copies the string
// The string has to be longer than 0
// Returns an empty string with NULL as data when the allocation fails
struct json_string json_string_copy(struct json_string str, bool *ok) {
    return json_string_create(str.data, str.len, ok);
}

// NOTE: Copies the string
// The string has to be longer than 0
// Returns an empty string with NULL as data when the allocation fails
struct json_value json_string_copyv(struct json_string str, bool *ok) {
    return json_string_to_value(json_string_create(str.data, str.len, ok));
}

struct json_value json_string_to_value(struct json_string str) {
    return (struct json_value) {
        .type = JSON_STRING,
        .data.string = str,
    };
}

// Create object

// Allocates a new object, use the other json_object_* functions to add and remove attributes.
//
struct json_object json_object_create(bool *ok) {
    JSON_ASSERT(ok != NULL, "ok has to be a valid pointer");
    struct json__hash_map *hm = json__hm_create();
    if (hm == NULL) {
        *ok = false;
    } else {
        *ok = true;
    }
    return (struct json_object) {
        ._hm = hm,
    };
}

struct json_value json_object_to_value(struct json_object obj) {
    return (struct json_value) {
        .type = JSON_OBJECT,
        .data.object = obj,
    };
}

struct json_object json_object_copy(struct json_object obj, bool *ok) {
    JSON_ASSERT(ok != NULL, "ok has to be a valid pointer");
    struct json_object new_obj = {
        ._hm = json__hash_map_copy(obj._hm),
    };

    if (new_obj._hm == NULL) {
        *ok = false;
    } else {
        *ok = true;
    }

    return new_obj;
}

struct json_value json_object_copyv(struct json_object obj, bool *ok) {
    return json_object_to_value(json_object_copy(obj, ok));
}

bool json_object_set(struct json_object *obj, struct json_string key, struct json_value value) {
    if (json__hash_map_set(obj->_hm, key, value)) {
        return true;
    }

    // Insert if it does not exist.
    return json__hash_map_insert(obj->_hm, key, value);
}

// Gets the value at the key.
struct json_value json_object_get(struct json_object *obj, struct json_string key, bool *found) {
    struct json__hash_map_entry *entry = json__hash_map_get(obj->_hm, key);
    if (entry == NULL) {
        *found = false;
        return (struct json_value) {0};
    }
    *found = true;
    return entry->value;
}

bool json_object_del(struct json_object *obj, struct json_string key) {
    return json__hash_map_delete(obj->_hm, key);
}

// Create array

// Allocates a new array, use the other json_array_* functions to add and remove elements.
struct json_array json_array_create(void) {
    return (struct json_array) {
        .len = 0,
        .items = NULL,
    };
}

// Allocates a new array and copies both left and right into it
// If the left.len + right.len > 0 and the return.items == NULL. Then the allocation failed. When a allocation fails, the returned array will have a length of 0.
struct json_array json_array_concat(struct json_array left, struct json_array right, bool *ok) {
    struct json_value *new_items = JSON_MALLOC(sizeof(*new_items) * (left.len + right.len));
    if (new_items == NULL) {
        *ok = false;
        return json_array_create();
    }

    *ok = true;

    for (size_t i = 0; i < left.len; i++) {
        new_items[i] = left.items[i];
    }

    for (size_t i = 0; i < right.len; i++) {
        new_items[i + left.len] = right.items[i];
    }

    return (struct json_array) {
        .items = new_items,
        .len = left.len + right.len,
    };
}

struct json_value json_array_concatv(struct json_array left, struct json_array right, bool *ok) {
    return json_array_to_value(json_array_concat(left, right, ok));
}

// Allocates a new array and copies the elements over, mostly for a utility macro
// If the arr.len > 0 and the return.items == NULL. Then the allocation failed. When a allocation fails, the returned array will have a length of 0.
struct json_array json_array_copy(struct json_array arr, bool *ok) {
    struct json_value *items = JSON_MALLOC(arr.len * sizeof(*items));


    if (items == NULL) {
        *ok = false;
        // Return a empty array
        return json_array_create();
    }

    *ok = true;

    for (size_t i = 0; i < arr.len; i++) {
        items[i] = arr.items[i];
    }

    return (struct json_array) {
        .len = arr.len,
        .items = items,
    };
}

struct json_value json_array_copyv(struct json_array arr, bool *ok) {
    return json_array_to_value(json_array_copy(arr, ok));
}

struct json_value json_array_to_value(struct json_array arr) {
    return (struct json_value) {
        .type = JSON_ARRAY,
        .data.array = arr,
    };
}


struct json_value json_boolean(bool value) {
    return (struct json_value) {
        .type = JSON_BOOLEAN,
        .data.boolean = value,
    };
}

struct json_value json_number(double value) {
    return (struct json_value) {
        .type = JSON_NUMBER,
        .data.number = value,
    };
}

struct json_value json_null(void) {
    return (struct json_value) {
        .type = JSON_NULL,
    };
}

struct json_value json_value_copy(struct json_value value, bool *ok) {
    JSON_ASSERT(ok != NULL, "ok has to be a valid pointer");
    switch (value.type) {
        case JSON_OBJECT:
            return json_object_copyv(value.data.object, ok);
        case JSON_ARRAY:
            return json_array_copyv(value.data.array, ok);
        case JSON_STRING:
            return json_string_copyv(value.data.string, ok);
        case JSON_NUMBER:
        case JSON_BOOLEAN:
        case JSON_NULL:
        case JSON_INVALID:
            return value;
    }
}

void json_value_delete(struct json_value value) {
    switch (value.type) {
        case JSON_OBJECT:
            json_object_delete(value.data.object);
            break;
        case JSON_ARRAY:
            json_array_delete(value.data.array);
            break;
        case JSON_STRING:
            json_string_delete(value.data.string);
            break;
        case JSON_NUMBER:
        case JSON_BOOLEAN:
        case JSON_NULL:
        case JSON_INVALID:
            break;
    }
}

void json_object_delete(struct json_object object) {}

void json_array_delete(struct json_array array) {
    for (size_t i = 0; i < array.len; i++) {
        json_value_delete(array.items[i]);
    }
    JSON_FREE(array.items);
}

void json_string_delete(struct json_string string) {
    if (0 < string.len) {
        JSON_FREE(string.data);
    }
}

//------------------------------
// INTERNAL HASH MAP FACILITIES
//------------------------------

// Generate a hash value
static size_t json__hash(unsigned char const * str, size_t len) {
    size_t hash = 5381;

    for (size_t i = 0; i < len; i++) {
        hash = hash * 33 ^ str[i];
    }

    return hash;
}

// Returns NULL if an allocation failed
static struct json__hash_map* json__hm_create(void) {
    struct json__hash_map* ptr = JSON_MALLOC(sizeof(*ptr));
    if (ptr == NULL) {
        return NULL;
    }

    struct json__hash_map_entry *bucket = JSON_CALLOC(JSON_INITIAL_BUCKET_SIZE, sizeof(*bucket));
    if (bucket == NULL) {
        JSON_FREE(ptr);
        return NULL;
    }

    struct json__hash_map_entry *collisions = JSON_CALLOC(JSON_INITIAL_BUCKET_SIZE, sizeof(*collisions));
    if (collisions == NULL) {
        JSON_FREE(ptr);
        JSON_FREE(bucket);
        return NULL;
    }

    *ptr = (struct json__hash_map) {
        .bucket_cap = JSON_INITIAL_BUCKET_SIZE,
        .bucket = bucket,

        .collisions = collisions,
        .collisions_cap = JSON_INITIAL_BUCKET_SIZE,
    };

    return ptr;
}

// Deletes all the values, keys and the internal memory of the hash map
static void json__hm_delete(struct json__hash_map* map) {
    for (size_t i = 0; i < map->collisions_size; i++) {
        if (json__hash_map_entry_valid(&map->collisions[i])) {
            json_string_delete(map->collisions[i].key);
            json_value_delete(map->collisions[i].value);
        }
    }

    for (size_t i = 0; i < map->bucket_cap; i++) {
        if (json__hash_map_entry_valid(&map->bucket[i])) {
            json_string_delete(map->bucket[i].key);
            json_value_delete(map->bucket[i].value);
        }
    }

    JSON_FREE(map->collisions);
    JSON_FREE(map->bucket);
    JSON_FREE(map);
}

bool json_string_eq(struct json_string first, struct json_string second) {
    if (first.len != second.len) {
        return false;
    }

    for (size_t i = 0; i < first.len; i++) {
        if (first.data[i] != second.data[i]) {
            return false;
        }
    }

    return true;
}

// NOTE: Returns NULL if not found
static struct json__hash_map_entry* json__hash_map_get(struct json__hash_map *hm, struct json_string key) {
    size_t hash = json__hash(key.data, key.len);
    size_t index = hash % hm->bucket_cap;
    struct json__hash_map_entry *entry = &hm->bucket[index];

    if (!json__hash_map_entry_valid(entry)) {
        return NULL;
    }

    if (json_string_eq(entry->key, key)) {
        return entry;
    }

    while (entry->next != NULL) {
        entry = entry->next;
        if (json_string_eq(entry->key, key)) {
            return entry;
        }
    }

    return NULL;
}

// This function creates a new hash map, inserts all elements of the old one into it and then replaces the old one.
// It returns false on a allocation failiure
static bool json__hash_map_grow(struct json__hash_map *hm) {
    size_t new_cap = hm->bucket_cap * JSON_GROWTH_FACTOR;
    struct json__hash_map_entry *bucket = JSON_CALLOC(new_cap, sizeof(*bucket));
    struct json__hash_map_entry *collisions = JSON_CALLOC(new_cap, sizeof(*collisions));
    struct json__hash_map new_hm = {
        .bucket = bucket,
        .bucket_cap = new_cap,

        .collisions = collisions,
        .collisions_cap = new_cap,
    };

    for (size_t i = 0; i < hm->bucket_cap; i++) {
        struct json__hash_map_entry *entry = &hm->bucket[i];
        if (!json__hash_map_entry_valid(entry)) {
            continue;
        }

        if(!json__hash_map_insert(&new_hm, entry->key, entry->value)) {
            JSON_FREE(bucket);
            JSON_FREE(collisions);
            return false;
        }
        while (entry->next != NULL) {
            entry = entry->next;
            if(!json__hash_map_insert(&new_hm, entry->key, entry->value)) {
                JSON_FREE(bucket);
                JSON_FREE(collisions);
                return false;
            }
        }
    }

    json__hm_delete(hm);
    *hm = new_hm;

    return true;
}

static double json__hash_map_load_factor(struct json__hash_map *hm) {
    return (double)hm->bucket_size / (double)hm->bucket_cap;
}

// NOTE: Returns false on allocation failiure
// The hash map stays valid even if the insert fails
// The key will be copied. You won't have to keep it alive.
static bool json__hash_map_insert(struct json__hash_map *hm, struct json_string key, struct json_value value) {
    bool ok = true;
    struct json__hash_map_entry new_entry = {
        .key = json_string_copy(key, &ok),
        .value = value,
    };

    if (!ok) {
        return false;
    }

    size_t hash = json__hash(new_entry.key.data, new_entry.key.len);
    size_t index = hash % hm->bucket_cap;
    struct json__hash_map_entry *entry = &hm->bucket[index];
    if (json__hash_map_entry_valid(entry)) {
        while (entry->next != NULL) {
            entry = entry->next;
        }

        if (hm->collisions_cap <= hm->collisions_size+1) {
            // NOTE: Why sizeof(*ptr)? this way, we can just change the type without having to change each sizeof
            size_t new_size = hm->collisions_cap * JSON_GROWTH_FACTOR * sizeof(*hm->collisions);
            struct json__hash_map_entry *new_collisions = JSON_REALLOC(hm->collisions, hm->collisions_cap * sizeof(*hm->collisions), new_size);
            if (new_collisions == NULL) {
                return false;
            }
            hm->collisions = new_collisions;
            hm->collisions_cap = new_size;
        }

        hm->collisions[hm->collisions_size] = new_entry;
        entry->next = &hm->collisions[hm->collisions_size];
        hm->collisions_size += 1;
    } else {
        *entry = new_entry;
    }
    hm->bucket_size += 1;

    if (json__hash_map_load_factor(hm) > JSON_MAX_LOAD_FACTOR) {
        json__hash_map_grow(hm);
    }
    return true;
}

static void json__hash_map_entry_delete(struct json__hash_map_entry* entry) {
    json_string_delete(entry->key);
    json_value_delete(entry->value);
}

// Updates the value at the key. If the key does not exist, false is returned.
static bool json__hash_map_set(struct json__hash_map *hm, struct json_string key, struct json_value value) {
    struct json__hash_map_entry *entry = json__hash_map_get(hm, key);
    if (entry == false) {
        return false;
    }

    json_value_delete(entry->value);
    entry->value = value;

    return true;
}


// Returns false if we could not find the key
static bool json__hash_map_delete(struct json__hash_map *hm, struct json_string key) {
    size_t hash = json__hash(key.data, key.len);
    size_t index = hash % hm->bucket_cap;
    struct json__hash_map_entry *entry = &hm->bucket[index];

    if (json_string_eq(entry->key, key)) {
        json__hash_map_entry_delete(entry);
        if (entry->next) {
            *entry = *entry->next;
        } else {
            // Reset the entry to 0
            *entry = (struct json__hash_map_entry) {0};
        }
        return true;
    }

    while (entry->next != NULL) {
        if (json_string_eq(entry->next->key, key)) {
            // NOTE: This leaves a unused slot over. It will be removed when we resize the hash map
            json__hash_map_entry_delete(entry->next);
            entry->next = entry->next->next;
            return true;
        }
        entry = entry->next;
    }

    return false;
}

static struct json__hash_map *json__hash_map_copy(struct json__hash_map *hm) {
    struct json__hash_map_entry *bucket = JSON_CALLOC(hm->bucket_cap, sizeof(*bucket));
    if (bucket == NULL) {
        return NULL;
    }
    struct json__hash_map_entry *collisions = JSON_CALLOC(hm->bucket_cap, sizeof(*collisions));
    if (collisions == NULL) {
        JSON_FREE(bucket);
        return NULL;
    }
    struct json__hash_map new_hm = {
        .bucket = bucket,
        .bucket_cap = hm->bucket_cap,
        .bucket_size = hm->bucket_size,

        .collisions = collisions,
        .collisions_cap = hm->bucket_cap,
        .collisions_size = hm->collisions_size,
    };

    bool ok = false;
    for (size_t i = 0; i < hm->bucket_cap; i++) {
        new_hm.bucket[i] = json__hash_map_entry_copy(hm->bucket[i], new_hm.collisions, hm->collisions, &ok);
        if (!ok) {
            json__hm_delete(&new_hm);
            return NULL;
        }
    }

    for (size_t i = 0; i < hm->collisions_size; i++) {
        new_hm.collisions[i] = json__hash_map_entry_copy(hm->collisions[i], new_hm.collisions, hm->collisions, &ok);
        if (!ok) {
            json__hm_delete(&new_hm);
            return NULL;
        }
    }

    struct json__hash_map *ptr = JSON_MALLOC(sizeof(*ptr));
    if (ptr == NULL) {
        json__hm_delete(&new_hm);
        return NULL;
    }
    *ptr = new_hm;
    return ptr;
}

static struct json__hash_map_entry json__hash_map_entry_copy(struct json__hash_map_entry entry, struct json__hash_map_entry *new_collisions, struct json__hash_map_entry *old_collisions, bool *ok) {
    if (!json__hash_map_entry_valid(&entry)) {
        *ok = true;
        return (struct json__hash_map_entry) {0};
    }

    *ok = true;

    struct json__hash_map_entry new_entry = {0};

    if (entry.next != NULL) {
        // Because next is a pointer, we need to get it's actual index in the collisions array and then add that back to the new collisons array
        new_entry.next = (entry.next - old_collisions) + new_collisions;
    }

    new_entry.key = json_string_copy(entry.key, ok);
    if (!*ok) {
        return new_entry;
    }

    new_entry.value = json_value_copy(entry.value, ok);
    if (!*ok) {
        json_string_delete(new_entry.key);
    }
    return new_entry;
}

static bool json__hash_map_entry_valid(struct json__hash_map_entry *entry) {
    return entry->value.type != JSON_INVALID;
}

//------------------------------
// STRING BUILDER
//------------------------------

// Generated using the javascript version of c_gen and the following config:
// {
//     "arrays": [
//         "unsigned char"
//     ],
//     "malloc": "JSON_MALLOC",
//     "realloc": "JSON_REALLOC",
//     "free": "JSON_FREE",
//     "prefix": "json__",
//     "assert": "JSON_ASSERT"
// }

enum json__array_err {
    ARRAY_OK,
    ARRAY_OOM
};

typedef enum json__array_err array_err;

struct json__slice_unsigned_char {
    unsigned char *items;
    size_t len;
};

typedef struct json__slice_unsigned_char json__slice_unsigned_char;

struct json__array_unsigned_char {
    unsigned char *items;
    size_t len;
    size_t cap;
};

typedef struct json__array_unsigned_char json__array_unsigned_char;

static void json__array_unsigned_char_delete(json__array_unsigned_char arr) {
    JSON_FREE(arr.items);
}

static array_err json__array_unsigned_char_grow(json__array_unsigned_char *arr) {
    if (arr->items == NULL) {
        arr->items = JSON_MALLOC(sizeof(arr->items[0]) * 4);
        if (arr->items == NULL) {
            return ARRAY_OOM;
        }
        arr->cap = 4;
    } else {
        size_t new_cap = sizeof(arr->items[0]) * arr->cap * 2;
        unsigned char* items = JSON_REALLOC(arr->items, arr->cap, new_cap);
        if (arr->items == NULL) {
            return ARRAY_OOM;
        }
        arr->items = items;
        arr->cap = new_cap;
    }
    return ARRAY_OK;
}

static array_err json__array_unsigned_char_grow_until(json__array_unsigned_char *arr, size_t until) {
    while(arr->cap < until) {
        array_err err = json__array_unsigned_char_grow(arr);
        if (err != ARRAY_OK) {
            return err;
        }
    }
    return ARRAY_OK;
}

static array_err json__array_unsigned_char_push(json__array_unsigned_char *arr, unsigned char item) {
    if (arr->cap <= arr->len) {
        array_err err = json__array_unsigned_char_grow_until(arr, arr->len + 1);
        if (err != ARRAY_OK) {
            return err;
        }
    }
    arr->items[arr->len] = item;
    arr->len += 1;

    return ARRAY_OK;
}

static array_err json__array_unsigned_char_append(json__array_unsigned_char *arr, json__slice_unsigned_char slice) {
    size_t new_size = slice.len + arr->len;
    if (arr->cap <= slice.len + arr->len) {
        array_err err = json__array_unsigned_char_grow_until(arr, new_size);
        if (err != ARRAY_OK) {
            return err;
        }
    }
    for (size_t i = 0; i < slice.len; i++) {
        arr->items[arr->len + i] = slice.items[i];
    }
    arr->len += slice.len;
    return ARRAY_OK;
}

#define JSON__ARRAY_UNSIGNED_CHAR_APPEND(arr, ...) json__array_unsigned_char_append((arr), (json__slice_unsigned_char){ .items = (unsigned char[]) { __VA_ARGS__ }, .len = sizeof((unsigned char[]){ __VA_ARGS__ }[0]) })


static void json__array_unsigned_char_unordered_remove(json__array_unsigned_char *arr, size_t at) {
    JSON_ASSERT_NO_MSG(0 <= at && at < arr->len);
    arr->items[at] = arr->items[arr->len - 1];
    arr->len -= 1;
}

static void json__array_unsigned_char_ordererd_remove(json__array_unsigned_char *arr, size_t at) {
    JSON_ASSERT_NO_MSG(0 <= at && at < arr->len);
    for (size_t i = at; i < arr->len - 1; i++) {
       	arr->items[i] = arr->items[i+1];
    }
    arr->len -= 1;
}

static void json__array_unsigned_char_pop(json__array_unsigned_char *arr) {
    JSON_ASSERT_NO_MSG(arr->len > 0);
    arr->len -= 1;
}

static void json__array_unsigned_char_pop_elements(json__array_unsigned_char *arr, size_t elements) {
    JSON_ASSERT_NO_MSG(arr->len > 0);
    arr->len -= elements;
}

static unsigned char json__array_unsigned_char_get(json__array_unsigned_char *arr, size_t at) {
    JSON_ASSERT_NO_MSG(at < arr->len);
    return arr->items[at];
}

static unsigned char json__array_unsigned_char_set(json__array_unsigned_char *arr, size_t at, unsigned char value) {
    JSON_ASSERT_NO_MSG(at < arr->len);
    unsigned char old_value = arr->items[at];
    arr->items[at] = value;
    return old_value;
}

// IMPORTANT: This slice is not owned, it has the same lifetime as the original array
static json__slice_unsigned_char json__array_unsigned_char_slice(json__array_unsigned_char *arr, size_t from, size_t to) {
    JSON_ASSERT_NO_MSG(0 <= from);
    JSON_ASSERT_NO_MSG(to <= arr->len);
    JSON_ASSERT_NO_MSG(to < from);
    return (json__slice_unsigned_char){
        .items = arr->items + from,
        .len = to,
    };
}

// IMPORTANT: This slice is not owned, it has the same lifetime as the original slice
static json__slice_unsigned_char json__slice_unsigned_char_slice(json__slice_unsigned_char *slice, size_t from, size_t to) {
    JSON_ASSERT_NO_MSG(0 <= from);
    JSON_ASSERT_NO_MSG(to <= slice->len);
    JSON_ASSERT_NO_MSG(to < from);
    return (json__slice_unsigned_char){
        .items = slice->items + from,
        .len = to,
    };
}

static array_err json__array_unsigned_char_to_owned_slice(json__array_unsigned_char *arr, json__slice_unsigned_char *dst) {
    unsigned char *new_slice = JSON_MALLOC(sizeof(unsigned char) * arr->len);

    if (new_slice == NULL) {
        return ARRAY_OOM;
    }

    for (size_t i = 0; i < arr->len; i++) {
        new_slice[i] = arr->items[i];
    }

    *dst = (json__slice_unsigned_char){
        .items = new_slice,
        .len = arr->len,
    };

    return ARRAY_OK;
}

static void json__slice_unsigned_char_delete_owned(json__slice_unsigned_char slice) {
    JSON_FREE(slice.items);
}

static unsigned char json__slice_unsigned_char_get(json__slice_unsigned_char *slice, size_t at) {
    JSON_ASSERT_NO_MSG(at < slice->len);
    return slice->items[at];
}

static unsigned char json__slice_unsigned_char_set(json__slice_unsigned_char *slice, size_t at, unsigned char value) {
    JSON_ASSERT_NO_MSG(at < slice->len);
    unsigned char old_value = slice->items[at];
    slice->items[at] = value;
    return old_value;
}


//------------------------------
// LEXER
//------------------------------

// TODO: Add UTF-8 Support using something like this https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
static void json__lexer_read_ch(struct json_lexer *l) {
    if (l->ch == '\n') {
        l->row += 1;
        l->col = 1;
    } else {
        l->col += 1;
    }

    if (l->read_pos >= l->source.len) {
        l->ch = -1;
    } else {
        l->ch = l->source.data[l->read_pos];
    }

    l->pos = l->read_pos;
    l->read_pos += 1;
}

// The string inputs are copied, you will have to delete them yourself.
bool json_lexer_init(struct json_lexer *l, struct json_string source, struct json_string source_name, enum json_spec spec) {
    bool ok = true;
    l->source_name = json_string_copy(source_name, &ok);
    if (!ok) {
        return false;
    }

    l->source = json_string_copy(source, &ok);
    if (!ok) {
        json_string_delete(l->source_name);
        l->source_name = (struct json_string){0};
        return false;
    }

    l->spec = spec;

    // We start on the first row
    l->row = 1;

    json__lexer_read_ch(l);

    return true;
}

void json_lexer_deinit(struct json_lexer *l) {
    json_string_delete(l->source);
    json_string_delete(l->source_name);
    *l = (struct json_lexer) {0};
}

// Hashes for the keywords, as long as all of them are different, we can use them to compare each other.
static size_t false_hash = 210667871779;
static size_t true_hash = 6383874908;
static size_t null_hash = 229417312366851;

static bool json__lexer_is_valid_identifier_ch(unsigned char ch, bool first_ch) {
    return ('a' <= ch && ch <= 'z')
            || ('A' <= ch && ch <= 'Z')
            || ch == '_'
            || (!first_ch && ('0' <= ch && ch <= '9'));
}

static bool json__is_whitespace(int ch) {
    return ch == ' '
            || ch == '\n'
            || ch == '\r'
            || ch == '\t';
}

static void json__lexer_skip_whitespace(struct json_lexer *l) {
    while (json__is_whitespace(l->ch)) {
        json__lexer_read_ch(l);
    }
}

static struct json_token json__lexer_read_token(struct json_lexer *l, bool *ok) {
    struct json_loc start_loc = (struct json_loc) {.pos = l->pos, .col = l->col, .row = l->row};
    size_t start_pos = l->pos;

    while (json__lexer_is_valid_identifier_ch(l->ch, false)) {
        json__lexer_read_ch(l);
    }

    size_t len = l->pos - start_pos;
    size_t hash = json__hash(l->source.data + start_pos, len);
    enum json_token_type type = JSON_TOKEN_IDENTIFIER;
    union json_token_data data = {0};

    // This is a lot more efficent then comparing each with the string
    // This only works if all the hashes are in fact, different, if not
    // we probably have to add an actual hash table
    if (hash == false_hash) {
        type = JSON_TOKEN_FALSE;
    } else if (hash == true_hash) {
        type = JSON_TOKEN_TRUE;
    } else if (hash == null_hash) {
        type = JSON_TOKEN_NULL;
    } else {
        data.string = json_string_create(l->source.data + start_pos, len, ok);
    }
    return (struct json_token){
        .data = data,
        .loc = start_loc,
        .len = len,
        .type = type,
    };
}

struct json_token json_lexer_next_token(struct json_lexer *l, bool *ok) {
    json__lexer_skip_whitespace(l);

    struct json_token t = {
        .len = 1,
        .loc = (struct json_loc) {.pos = l->pos, .col = l->col, .row = l->row},
    };

    switch (l->ch) {
    case -1:    t.type = JSON_TOKEN_EOF; break;
    case '{':   t.type = JSON_TOKEN_LBRACE; break;
    case '}':   t.type = JSON_TOKEN_RBRACE; break;
    case '[':   t.type = JSON_TOKEN_LBRACKET; break;
    case ']':   t.type = JSON_TOKEN_RBRACKET; break;
    case ',':   t.type = JSON_TOKEN_COMMA; break;
    case ':':   t.type = JSON_TOKEN_COLON; break;
    default:
        if (json__lexer_is_valid_identifier_ch(l->ch, true)) {
            return json__lexer_read_token(l, ok);
        }
    }
    json__lexer_read_ch(l);

    return t;
}

#endif // JSON_IMPLEMENTATION

#endif // JSON_H_INC
