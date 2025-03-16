#pragma once

#include "string_ops.h"
#include <linux/limits.h>
#include <sys/stat.h>
#include <stdbool.h>

typedef struct {
    bool exists;
} fs_metadata;

fs_metadata fs_get_metadata(string_view filename) {
    char buf[PATH_MAX];
    fs_metadata metadata;
    struct stat st;
    metadata.exists = false;
    if (filename.len + 1 > PATH_MAX) {
        return metadata;
    }
    memset(buf, 0, sizeof(buf));
    memcpy(buf, filename.data, filename.len);
    if (stat(buf, &st) < 0) {
        return metadata;
    }
    metadata.exists = true;
    return metadata;
}
