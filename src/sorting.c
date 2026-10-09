#include <stddef.h>
#include <string.h>
#include <time.h>

#include "sorting.h"

/* Select the timestamp requested by the user. */
static const struct timespec *get_entry_time(const LsEntry *entry,
                             const LsOptions *options)
{
    switch (options->time_mode) {
    case TIME_CHANGED:
        return &entry->info.st_ctim;
    case TIME_ACCESSED:
        return &entry->info.st_atim;
    default:
        return &entry->info.st_mtim;
    }
}

/* Compare two entries according to the sort options. */
static int compare_entries(const LsEntry *a,
                           const LsEntry *b,
                           const LsOptions *options)
{
    int result = 0;

    if (options->sort_mode == SORT_SIZE) {
        if (a->info.st_size > b->info.st_size)
            result = -1;
        else if (a->info.st_size < b->info.st_size)
            result = 1;
    } else if (options->sort_mode == SORT_TIME) {
        const struct timespec *ta = get_entry_time(a, options);
        const struct timespec *tb = get_entry_time(b, options);

        if (ta->tv_sec > tb->tv_sec || (ta->tv_sec == tb->tv_sec && ta->tv_nsec > tb->tv_nsec))
            result = -1;
        else if (ta->tv_sec < tb->tv_sec || (ta->tv_sec == tb->tv_sec && ta->tv_nsec < tb->tv_nsec))
            result = 1;
    }

    /* Use the file name when primary values are equal. */
    if (result == 0)
        result = strcmp(a->name, b->name);

    if (options->reverse) {
        if (result > 0)
            return -1;
        if (result < 0)
            return 1;
        return 0;
    }

    return result;
}

/* Exchange two entries without copying their strings. */
static void swap_entries(LsEntry *a, LsEntry *b)
{
    LsEntry temp = *a;
    *a = *b;
    *b = temp;
}

/* Restore the heap property within the given range. */
static void sift_down(LsEntry *items, size_t root,
                      size_t end, const LsOptions *options)
{
    while (end > 0 && root <= (end - 1) / 2) {
        size_t child = root * 2 + 1;

        if (child < end &&
            compare_entries(&items[child],
                            &items[child + 1],
                            options) < 0) {
            child++;
        }

        if (compare_entries(&items[root],
                            &items[child],
                            options) >= 0) {
            break;
        }

        swap_entries(&items[root], &items[child]);
        root = child;
    }
}

/* Sort entries in-place using heap sort. */
void sort_entries(LsEntryList *list,
                  const LsOptions *options)
{
    size_t start;
    size_t end;

    if (list == NULL || options == NULL ||
        list->count < 2 ||
        options->sort_mode == SORT_NONE) {
        return;
    }

    /* Build a max heap. */
    start = list->count / 2;

    while (start > 0) {
        start--;
        sift_down(list->items, start,
                  list->count - 1, options);
    }

    /* Move the largest entry to the end repeatedly. */
    end = list->count - 1;

    while (end > 0) {
        swap_entries(&list->items[0], &list->items[end]);

        if (end > 1) {
            sift_down(list->items, 0, end - 1, options);
        }

        end--;
    }
}
