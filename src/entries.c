#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "entries.h"

/*
 * Add a filesystem entry to a dynamic list.
 * Return 0 on success and 1 on failure.
 */
int entry_list_add(LsEntryList *list,
                   const char *name,
                   const char *path,
                   const struct stat *info)
{
    LsEntry *new_items;
    char *name_copy;
    char *path_copy;
    size_t new_capacity;

    if (list == NULL || name == NULL ||
        path == NULL || info == NULL) {
        errno = EINVAL;
        return 1;
    }

    name_copy = malloc(strlen(name) + 1);
    if (name_copy == NULL) {
        return 1;
    }
    strcpy(name_copy, name);

    path_copy = malloc(strlen(path) + 1);
    if (path_copy == NULL) {
        free(name_copy);
        return 1;
    }
    strcpy(path_copy, path);

    /* Grow the array when it is full. */
    if (list->count == list->capacity) {
        if (list->capacity == 0) {
            new_capacity = 8;
        } else {
            if (list->capacity >
                SIZE_MAX / (2 * sizeof(LsEntry))) {
                free(name_copy);
                free(path_copy);
                errno = ENOMEM;
                return 1;
            }
            new_capacity = list->capacity * 2;
        }

        new_items = realloc(
            list->items,
            new_capacity * sizeof(LsEntry)
        );

        if (new_items == NULL) {
            free(name_copy);
            free(path_copy);
            return 1;
        }

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count].name = name_copy;
    list->items[list->count].path = path_copy;
    list->items[list->count].info = *info;

    list->count++;

    return 0;
}

/* Free all entries and reset the list. */
void entry_list_free(LsEntryList *list)
{
    size_t i;

    if (list == NULL) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        free(list->items[i].name);
        free(list->items[i].path);
    }

    free(list->items);

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
