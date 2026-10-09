#ifndef OPTIONS_H
#define OPTIONS_H

/* File sorting methods. */
typedef enum {
    SORT_NAME,
    SORT_SIZE,
    SORT_TIME,
    SORT_NONE
} SortMode;

/* Output detail formats. */
typedef enum {
    FORMAT_NORMAL,
    FORMAT_LONG,
    FORMAT_NUMERIC
} OutputFormat;

/* File timestamp selection. */
typedef enum {
    TIME_MODIFIED,
    TIME_CHANGED,
    TIME_ACCESSED
} TimeMode;

/* File size display formats. */
typedef enum {
    SIZE_DEFAULT,
    SIZE_HUMAN,
    SIZE_KILOBYTES
} SizeMode;

/* Directory handling modes. */
typedef enum {
    DIR_NORMAL,
    DIR_ONLY,
    DIR_RECURSIVE
} DirectoryMode;

/* Non-printable character handling. */
typedef enum {
    NAME_AUTO,
    NAME_QUESTION,
    NAME_RAW
} NameMode;

/* Configuration of the ls command. */
typedef struct {
    int show_all;
    int almost_all;
    int classify;
    int show_inode;
    int show_blocks;
    int reverse;

    SortMode sort_mode;
    OutputFormat output_format;
    TimeMode time_mode;
    SizeMode size_mode;
    DirectoryMode directory_mode;
    NameMode name_mode;
} LsOptions;

/* Parse command-line options and locate file operands. */
int parse_options(int argc, char *argv[],
                  LsOptions *options, int *first_operand);

#endif
