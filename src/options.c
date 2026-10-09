#include <stdio.h>
#include <unistd.h>
#include "options.h"

/*
 * Parse command-line options for myls.
 * Return 0 on success and 1 on error.
 */
int parse_options(int argc, char *argv[],
                  LsOptions *options, int *first_operand)
{
    int ch;

    if (argc < 1 || argv == NULL ||
        options == NULL || first_operand == NULL) {
        return 1;
    }

    /* Initialize all options to default values. */
    *options = (LsOptions){0};

    options->sort_mode = SORT_NAME;
    options->output_format = FORMAT_NORMAL;
    options->time_mode = TIME_MODIFIED;
    options->size_mode = SIZE_DEFAULT;
    options->directory_mode = DIR_NORMAL;
    options->name_mode = NAME_AUTO;

    /* NetBSD ls enables -A for the superuser. */
    options->almost_all = (geteuid() == 0);

    opterr = 0;
    optind = 1;

    while ((ch = getopt(argc, argv,
                        "AacdFfhiklnqRrSstuw")) != -1) {

        switch (ch) {
        case 'A':
            options->almost_all = 1;
            break;

        case 'a':
            options->show_all = 1;
            break;

        case 'c':
            options->time_mode = TIME_CHANGED;
            break;

        case 'd':
            options->directory_mode = DIR_ONLY;
            break;

        case 'F':
            options->classify = 1;
            break;

        case 'f':
            options->sort_mode = SORT_NONE;
            options->show_all = 1;
            break;

        case 'h':
            options->size_mode = SIZE_HUMAN;
            break;

        case 'i':
            options->show_inode = 1;
            break;

        case 'k':
            options->size_mode = SIZE_KILOBYTES;
            break;

        case 'l':
            options->output_format = FORMAT_LONG;
            break;

        case 'n':
            options->output_format = FORMAT_NUMERIC;
            break;

        case 'q':
            options->name_mode = NAME_QUESTION;
            break;

        case 'R':
            if (options->directory_mode != DIR_ONLY)
                options->directory_mode = DIR_RECURSIVE;
            break;

        case 'r':
            options->reverse = 1;
            break;

        case 'S':
            if (options->sort_mode != SORT_NONE) {
            options->sort_mode = SORT_SIZE;
            }
            break;

        case 's':
            options->show_blocks = 1;
            break;

        case 't':
            if (options->sort_mode != SORT_NONE) {
            options->sort_mode = SORT_TIME;
            }
            break;

        case 'u':
            options->time_mode = TIME_ACCESSED;
            break;

        case 'w':
            options->name_mode = NAME_RAW;
            break;

        default:
            fprintf(stderr,
                    "myls: invalid option -- '%c'\n",
                    optopt ? optopt : '?');
            return 1;
        }
    }

    /* Index of the first file or directory argument. */
    *first_operand = optind;

    return 0;
}
