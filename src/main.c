#include <stdlib.h>
#include <util.h>
#include <stdint.h>
#include <inttypes.h>
#include <err.h>
#include <stdio.h>
#include <sys/stat.h>

#include "filesystem.h"
#include "options.h"
#include "entries.h"
#include "sorting.h"
#include "display.h"

/*
 * Determine whether an operand should be listed
 * as a directory rather than printed as a file.
 */
static int is_directory_operand(const LsEntry *entry,
                                const LsOptions *options)
{
    struct stat target;

    if (options->directory_mode == DIR_ONLY)
        return 0;

    if (S_ISDIR(entry->info.st_mode))
        return 1;

    /* Preserve the existing symbolic-link behavior. */
    if (S_ISLNK(entry->info.st_mode) &&
        options->output_format == FORMAT_NORMAL &&
        stat(entry->path, &target) == 0 &&
        S_ISDIR(target.st_mode)) {
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    LsOptions options;
    LsEntryList operands = {0};
    LsEntryList files = {0};
    DisplayWidths widths;
    struct stat info;
    int first_operand;
    int result = 0;
    int printed_any = 0;
    int show_headers;
    int i;
    size_t j;

    if (parse_options(argc, argv, &options,
                      &first_operand) != 0) {
        return 1;
    }

    /* No operands: list the current directory. */
    if (first_operand >= argc)
        return list_path(".", &options);

    /* Collect valid operands. */
    for (i = first_operand; i < argc; i++) {
        if (lstat(argv[i], &info) == -1) {
            warn("%s", argv[i]);
            result = 1;
            continue;
        }

        if (entry_list_add(&operands, argv[i],
                           argv[i], &info) != 0) {
            perror("myls: entry_list_add");
            result = 1;
            break;
        }
    }

    /* Sort all operands using the existing sorter. */
    sort_entries(&operands, &options);

    /* Collect file operands for shared column widths. */
    for (j = 0; j < operands.count; j++) {
        if (is_directory_operand(&operands.items[j], &options))
            continue;

        if (entry_list_add(&files,
                           operands.items[j].name,
                           operands.items[j].path,
                           &operands.items[j].info) != 0) {
            perror("myls: entry_list_add");
            entry_list_free(&files);
            entry_list_free(&operands);
            return 1;
        }
    }

    calculate_display_widths(&files, &options, &widths);

    /* Print files using shared column widths. */
    for (j = 0; j < files.count; j++) {
        const LsEntry *entry = &files.items[j];

        if (options.output_format != FORMAT_NORMAL) {
            if (print_entry_aligned(entry, &options, &widths) != 0)
                result = 1;
        } else {
            int failed = 0;

            if (options.show_inode &&
                printf("%*ju ", (int)widths.inode_width,
                       (uintmax_t)entry->info.st_ino) < 0)
                failed = 1;

            if (!failed && options.show_blocks) {
                if (options.size_mode == SIZE_HUMAN) {
                    char human_size[5];

                    if (humanize_number(human_size, sizeof(human_size),
                                        (int64_t)entry->info.st_size, "",
                                        HN_AUTOSCALE,
                                        HN_DECIMAL | HN_B | HN_NOSPACE) < 0 ||
                        printf("%*s ", (int)widths.blocks_width,
                               human_size) < 0)
                        failed = 1;
                } else if (printf("%*ju ",
                                  (int)widths.blocks_width,
                                  display_block_count(&entry->info,
                                                      &options)) < 0) {
                    failed = 1;
                }
            }

            if (!failed &&
                (print_display_name(entry->name, &options) != 0 ||
                 printf("%s\n",
                        classify_marker(&entry->info, &options)) < 0))
                failed = 1;

            if (failed) {
                perror("myls: print operand");
                result = 1;
            }
        }

        printed_any = 1;
    }

    /* Directory headers are needed for multiple operands. */
    show_headers = operands.count > 1;

    /* Second pass: list directory contents. */
    for (j = 0; j < operands.count; j++) {
        if (!is_directory_operand(&operands.items[j],
                                  &options)) {
            continue;
        }

        /* Separate files and directory sections. */
        if (printed_any && putchar('\n') == EOF) {
            perror("myls: newline");
            result = 1;
        }

        if (show_headers) {
            if (print_display_name(operands.items[j].path,
                                   &options) != 0 ||
                puts(":") == EOF) {
                perror("myls: directory header");
                result = 1;
            }
        }

        if (list_path(operands.items[j].path,
                      &options) != 0) {
            result = 1;
        }

        printed_any = 1;
    }

    entry_list_free(&files);
    entry_list_free(&operands);

    return result;
}
