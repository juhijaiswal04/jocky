/* JOCKY Runtime — Filesystem / Part of libjocky. / FS Implementation */
#include "jky_fs.h"
#include "jky_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

JkyList *jky_fs_list(JkyString *path, JkyError **err) {
    // Stub
    if (err) *err = jky_error_from("Not implemented");
    return jky_list_new();
}

JkyFSEntry *jky_fs_metadata(JkyString *path, JkyError **err) {
    if (!path) return NULL;
    struct stat st;
    if (stat(path->data, &st) == 0) {
        JkyFSEntry *e = malloc(sizeof(JkyFSEntry));
        e->name = jky_string_from_cstr("unknown");
        e->path = path;
        e->is_dir = S_ISDIR(st.st_mode);
        e->size = st.st_size;
        e->mtime = st.st_mtime;
        e->ctime = st.st_ctime;
        e->atime = st.st_atime;
        e->permissions = jky_string_from_cstr("");
        e->owner = jky_string_from_cstr("");
        e->sha256 = NULL;
        return e;
    }
    if (err) *err = jky_error_from("stat failed");
    return NULL;
}

JkyString *jky_fs_hash(JkyString *path, JkyError **err) {
    return jky_sha256_file(path->data, err);
}

JkyBytes *jky_fs_read(JkyString *path, JkyError **err) {
    if (!path) return NULL;
    FILE *f = fopen(path->data, "rb");
    if (!f) {
        if (err) *err = jky_error_from("fopen failed");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    JkyBytes *b = jky_bytes_new(size);
    if (b) {
        b->len = size;
        fread(b->data, 1, size, f);
    }
    fclose(f);
    return b;
}

int jky_fs_exists(JkyString *path) {
    if (!path) return 0;
    struct stat st;
    return stat(path->data, &st) == 0;
}

JkyString *jky_fs_entry_json(JkyFSEntry *e) {
    return jky_string_from_cstr("{}");
}
