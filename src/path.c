#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>

#include "filesystem.h"
#include "display.h"

/*
 * Display a file operand.
 * In long or numeric format, use print_entry().
 * Otherwise, display only the original path.
 */
static int print_path_entry(const char *path,
                            const struct stat *info,
                            const LsOptions *options)
{
    LsEntry entry = {0};

    if (options->output_format != FORMAT_NORMAL) {
        /*
         * Borrow the path pointer temporarily.
         * No allocation or ownership transfer occurs.
         */
        entry.name = (char *)path;
        entry.path = (char *)path;
        entry.info = *info;

        return print_entry(&entry, options);
    }

    /* Print inode before allocated blocks. */
    if (options->show_inode) {
        if (printf("%ju ", (uintmax_t)info->st_ino) < 0) {
            perror("myls: printf inode");
            return 1;
        }
    }

    if (options->show_blocks && options->size_mode == SIZE_HUMAN) {
        char human_size[5];
        if (humanize_number(human_size, sizeof(human_size),
                            (int64_t)info->st_size, "",
                            HN_AUTOSCALE,
                            HN_DECIMAL | HN_B | HN_NOSPACE) < 0) {
            perror("myls: humanize_number");
            return 1;
        }
        if (printf("%s ", human_size) < 0) {
            perror("myls: printf size");
            return 1;
        }
    }

    if (options->show_blocks && options->size_mode != SIZE_HUMAN) {
        if (printf("%ju ",
                   display_block_count(info, options)) < 0) {
            perror("myls: printf blocks");
            return 1;
        }
    }


    if (print_display_name(path, options) != 0 ||
        printf("%s\n", classify_marker(info, options)) < 0) {
        perror("myls: print name");
        return 1;
    }

    return 0;
}

/*
 * Handle a file or directory operand.
 * Directories are opened unless -d is specified.
 */
int list_path(const char *path,
              const LsOptions *options)
{
    struct stat info;

    if (path == NULL || options == NULL) {
        fprintf(stderr, "myls: invalid arguments\n");
        return 1;
    }

    /* Read metadata without following symbolic links. */
    if (lstat(path, &info) == -1) {
        perror(path);
        return 1;
    }

    /*
     * With -d, display the directory operand itself,
     * including its metadata when -l is selected.
     */
    if (options->directory_mode == DIR_ONLY) {
        return print_path_entry(path, &info, options);
    }

    /* Display the contents of a normal directory. */
    if (S_ISDIR(info.st_mode)) {
        return list_directory(path, options);
    }

    /*
     * Preserve existing behavior for symbolic links
     * pointing to directories.
     */
    if (S_ISLNK(info.st_mode)) {
        struct stat target;

        if (options->output_format == FORMAT_NORMAL && stat(path, &target) == 0 &&
            S_ISDIR(target.st_mode)) {
            return list_directory(path, options);
        }
    }

    /* Display regular files and other operands. */
    return print_path_entry(path, &info, options);
}
