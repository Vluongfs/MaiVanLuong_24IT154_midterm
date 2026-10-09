#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#include "entries.h"
#include "options.h"

/*
 * Maximum field widths for long-format output.
 * These values are calculated from the entries
 * before the directory is displayed.
 */
typedef struct {
    size_t nlink_width;
    size_t inode_width;
    size_t blocks_width;
    size_t owner_width;
    size_t group_width;
    size_t size_width;
    size_t major_width;
    size_t minor_width;
} DisplayWidths;

/*
 * Display one filesystem entry.
 * Return 0 on success, 1 on failure.
 */
int print_entry_aligned(const LsEntry *entry,
                        const LsOptions *options,
                        const DisplayWidths *widths);

int print_entry(const LsEntry *entry,
                const LsOptions *options);


/* Calculate column widths from a list of entries. */
void calculate_display_widths(const LsEntryList *list,
                              const LsOptions *options,
                              DisplayWidths *widths);


/* Return the classification suffix for the -F option. */
const char *classify_marker(const struct stat *info,
                            const LsOptions *options);

uintmax_t display_block_count(const struct stat *info,
                              const LsOptions *options);

int print_display_name(const char *name, const LsOptions *options);
#endif
