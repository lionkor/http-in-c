#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* TODO: This is the same as string_view :) */
typedef struct {
    const char* data;
    size_t len;
} string;

bool string_equal(string* l, string* r) {
    return l->len == r->len && memcmp(l->data, r->data, l->len) == 0;
}

string string_from_cstr(const char* str) {
    string s;
    s.len = strlen(str);
    s.data = str;
    return s;
}

void string_trim_spaces(string* s) {
    /* left trim */
    while (*s->data == ' ') {
        s->data += 1;
    }
    /* right trim */
    while (s->len > 0 && s->data[s->len - 1] == ' ') {
        s->len -= 1;
    }
}

typedef struct {
    const char* start;
    size_t len;
} string_view;

typedef struct {
    string_view* splits;
    size_t count;
    size_t capacity;
} string_splits;

static string_splits split_string(const char* str, size_t len, const char* split_by) {
    string_splits result;
    const char* start = str;
    size_t result_i = 0;
    size_t split_by_len = strlen(split_by);

    result.capacity = 8;
    result.splits = (string_view*)calloc(sizeof(string_view), result.capacity);
    result.count = 0;

    for (size_t i = 0; i < len; ++i) {
        if (i + split_by_len < len && memcmp(&str[i], split_by, split_by_len) == 0) {
            result.splits[result_i].start = start;
            result.splits[result_i].len = &str[i] - start;
            result.count += 1;
            result_i += 1;
            start = &str[i + split_by_len];
            i += split_by_len;

            if (result.count == result.capacity) {
                result.capacity *= 2;
                string_view* temp = (string_view*)realloc(result.splits, sizeof(string_view) * result.capacity);
                if (temp) {
                    result.splits = temp;
                } else {
                    perror("realloc()");
                    abort();
                }
            }
        }
    }
    /* we might miss the last one if the input string doesn't end with the delimiter */
    size_t last_len = &str[len] - start;
    if (last_len > 0) {
        result.splits[result_i].start = start;
        result.splits[result_i].len = last_len;
        result.count += 1;
    }
    return result;
}

static void free_splits(string_splits* splits) {
    if (splits) {
        free(splits->splits);
        splits->splits = NULL;
    }
}
