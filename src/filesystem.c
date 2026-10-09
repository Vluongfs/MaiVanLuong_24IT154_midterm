#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesystem.h"
#include "entries.h"
#include "sorting.h"
#include "display.h"

/*
 * Determine whether an entry should be displayed.
 * -a includes all entries.
 * -A includes hidden entries except "." and "..".
 */
static int should_display_entry(const char *name,
                                const LsOptions *options)
{
    if (options->show_all) {
        return 1;
    }

    if (strcmp(name, ".") == 0 ||
        strcmp(name, "..") == 0) {
        return 0;
    }

    if (name[0] == '.' && !options->almost_all) {
        return 0;
    }

    return 1;
}

/*
 * Construct a full path from directory and file name.
 * The caller must free the returned string.
 */
static char *make_full_path(const char *dir,
                            const char *name)
{
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    char *full_path;
    int need_slash;

    if (dir_len > SIZE_MAX - name_len - 2) {
        errno = ENOMEM;
        return NULL;
    }

    full_path = malloc(dir_len + name_len + 2);

    if (full_path == NULL) {
        return NULL;
    }

    need_slash = (dir_len > 0 &&
                  dir[dir_len - 1] != '/');

    memcpy(full_path, dir, dir_len);

    if (need_slash) {
        full_path[dir_len++] = '/';
    }

    memcpy(full_path + dir_len,
           name, name_len + 1);

    return full_path;
}

/*
 * Read directory entries into a dynamic list,
 * then display them and release allocated memory.
 */
static int list_directory_internal(const char *path,
                                   const LsOptions *options,
                                   int show_header)
{
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    LsEntryList list = {0};
    DisplayWidths widths = {0};

    char *full_path;
    size_t i;
    int result = 0;

    if (path == NULL || options == NULL) {
        fprintf(stderr, "myls: invalid arguments\n");
        return 1;
    }

    dir = opendir(path);

    if (dir == NULL) {
        warn("%s", path);
        return 1;
    }

    for (;;) {
        errno = 0;
        entry = readdir(dir);

        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir");
                result = 1;
            }
            break;
        }

        if (!should_display_entry(entry->d_name,
                                  options)) {
            continue;
        }

        full_path = make_full_path(
            path, entry->d_name
        );

        if (full_path == NULL) {
            perror("myls: make_full_path");
            result = 1;
            break;
        }

        if (lstat(full_path, &info) == -1) {
            perror(full_path);
            free(full_path);
            result = 1;
            continue;
        }

        if (entry_list_add(&list,
                           entry->d_name,
                           full_path,
                           &info) != 0) {
            perror("myls: entry_list_add");
            free(full_path);
            result = 1;
            break;
        }

        free(full_path);
    }

    if (closedir(dir) == -1) {
        perror("closedir");
        result = 1;
    }

    /* Sort entries before displaying them. */
    sort_entries(&list, options);
    calculate_display_widths(&list, options, &widths);

    /* Print directory header in recursive mode. */
    if (show_header && options->directory_mode == DIR_RECURSIVE) {
        if (print_display_name(path, options) != 0 ||
            puts(":") == EOF) {
            perror("myls: directory header");
            result = 1;
        }
    }

    if (options->output_format != FORMAT_NORMAL && list.count > 0) {
        uintmax_t total_blocks = 0;
        uintmax_t total_bytes = 0;
        char human_total[5];

        for (i = 0; i < list.count; i++) {
            total_blocks += (uintmax_t)list.items[i].info.st_blocks;
            if (list.items[i].info.st_size > 0)
                total_bytes += (uintmax_t)list.items[i].info.st_size;
        }

        if (options->size_mode == SIZE_KILOBYTES)
            total_blocks = total_blocks / 2 + total_blocks % 2;
        if (options->size_mode == SIZE_HUMAN) {
            if (humanize_number(human_total, sizeof(human_total),
                                (int64_t)total_bytes, "",
                                HN_AUTOSCALE,
                                HN_DECIMAL | HN_B | HN_NOSPACE) < 0) {
                perror("myls: humanize_number");
                result = 1;
            } else if (printf("total %s\n", human_total) < 0) {
                perror("myls: printf");
                result = 1;
            }
        } else if (printf("total %ju\n", total_blocks) < 0) {
            perror("myls: printf");
            result = 1;
        }
    }

    /* Display collected entries. */
    for (i = 0; i < list.count; i++) {
        /* Print inode before block count. */
        if (options->output_format == FORMAT_NORMAL &&
            options->show_inode) {
            if (printf("%*ju ", (int)widths.inode_width,
                       (uintmax_t)list.items[i].info.st_ino) < 0) {
                perror("myls: printf inode");
                result = 1;
                break;
            }
        }
        if (options->show_blocks &&
            options->output_format == FORMAT_NORMAL) {
            if (options->size_mode == SIZE_HUMAN) {
                char human_size[5];
                if (humanize_number(human_size, sizeof(human_size),
                                    (int64_t)list.items[i].info.st_size, "",
                                    HN_AUTOSCALE,
                                    HN_DECIMAL | HN_B | HN_NOSPACE) < 0) {
                    perror("myls: humanize_number");
                    result = 1;
                    break;
                }
                if (printf("%*s ", (int)widths.blocks_width,
                           human_size) < 0) {
                    perror("myls: printf size");
                    result = 1;
                    break;
                }
            } else if (printf("%*ju ",
                       (int)widths.blocks_width,
                       display_block_count(&list.items[i].info, options)) < 0) {
                perror("myls: printf blocks");
                result = 1;
                break;
            }
        }
        if (options->output_format != FORMAT_NORMAL) {
            if (print_entry_aligned(&list.items[i], options, &widths) != 0) {
                result = 1;
                break;
            }
        } else {

            if (print_display_name(list.items[i].name, options) != 0 ||
                printf("%s\n",
                       classify_marker(&list.items[i].info, options)) < 0) {
                perror("myls: print name");
                result = 1;
                break;
            }
        }
    }

    /* Recursively visit real subdirectories. */
    if (options->directory_mode == DIR_RECURSIVE) {
        for (i = 0; i < list.count; i++) {
            if (!S_ISDIR(list.items[i].info.st_mode))
                continue;

            if (strcmp(list.items[i].name, ".") == 0 ||
                strcmp(list.items[i].name, "..") == 0)
                continue;

            if (puts("") == EOF) {
                perror("myls: newline");
                result = 1;
                break;
            }

            if (list_directory_internal(list.items[i].path, options, 1) != 0)
                result = 1;
        }
    }

    /* Release allocated memory. */
    entry_list_free(&list);

    return result;
}

int list_directory(const char *path, const LsOptions *options)
{
    return list_directory_internal(path, options, 0);
}
