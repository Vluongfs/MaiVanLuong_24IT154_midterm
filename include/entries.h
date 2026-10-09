#ifndef ENTRIES_H
#define ENTRIES_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

/* Information about a filesystem entry. */
typedef struct {
    char *name;
    char *path;
    struct stat info;
} LsEntry;

/* Dynamic list of filesystem entries. */
typedef struct {
    LsEntry *items;
    size_t count;
    size_t capacity;
} LsEntryList;

/* Add an entry to the list. */
int entry_list_add(LsEntryList *list,
                   const char *name,
                   const char *path,
                   const struct stat *info);

/* Release all memory owned by the list. */
void entry_list_free(LsEntryList *list);

#endif
