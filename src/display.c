#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <time.h>
#include <pwd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "display.h"

/*
 * Convert the file mode into a 10-character
 * string describing type and permissions.
 */
/* Convert 512-byte blocks to the selected unit. */
uintmax_t display_block_count(const struct stat *info,
                                 const LsOptions *options)
{
    uintmax_t blocks = (uintmax_t)info->st_blocks;

    if (options->size_mode == SIZE_KILOBYTES)
        return blocks / 2 + blocks % 2;

    return blocks;
}

static void format_file_mode(mode_t mode, char result[11])
{
    /* Determine the file type. */
    result[0] = '-';

    if (S_ISDIR(mode))
        result[0] = 'd';
    else if (S_ISLNK(mode))
        result[0] = 'l';
    else if (S_ISCHR(mode))
        result[0] = 'c';
    else if (S_ISBLK(mode))
        result[0] = 'b';
    else if (S_ISFIFO(mode))
        result[0] = 'p';
    else if (S_ISSOCK(mode))
        result[0] = 's';
#ifdef S_ISWHT
    else if (S_ISWHT(mode))
        result[0] = 'w';
#endif

    /* Owner permissions. */
    result[1] = (mode & S_IRUSR) ? 'r' : '-';
    result[2] = (mode & S_IWUSR) ? 'w' : '-';
    result[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Group permissions. */
    result[4] = (mode & S_IRGRP) ? 'r' : '-';
    result[5] = (mode & S_IWGRP) ? 'w' : '-';
    result[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Other permissions. */
    result[7] = (mode & S_IROTH) ? 'r' : '-';
    result[8] = (mode & S_IWOTH) ? 'w' : '-';
    result[9] = (mode & S_IXOTH) ? 'x' : '-';

    /* Set-user-ID permission. */
    if (mode & S_ISUID) {
        result[3] = (mode & S_IXUSR) ? 's' : 'S';
    }

    /* Set-group-ID permission. */
    if (mode & S_ISGID) {
        result[6] = (mode & S_IXGRP) ? 's' : 'S';
    }

    /* Sticky bit permission. */
    if (mode & S_ISVTX) {
        result[9] = (mode & S_IXOTH) ? 't' : 'T';
    }

    result[10] = '\0';
}

/*
 * Display file permissions and name.
 * This is an intermediate implementation.
 */

/*
 * Print the target of a symbolic link.
 * The buffer grows when the target is too long.
 */
static int print_link_target(const char *path)
{
    size_t capacity = 128;

    for (;;) {
        char *target = malloc(capacity);
        ssize_t length;
        int printed;

        if (target == NULL) {
            perror("myls: malloc");
            return 1;
        }

        length = readlink(path, target, capacity - 1);

        if (length < 0) {
            perror(path);
            free(target);
            return 1;
        }

        if ((size_t)length < capacity - 1) {
            target[length] = '\0';

            printed = printf(" -> %s", target);
            free(target);

            if (printed < 0) {
                perror("myls: printf");
                return 1;
            }

            return 0;
        }

        free(target);

        if (capacity > SIZE_MAX / 2) {
            errno = ENOMEM;
            perror("myls: readlink");
            return 1;
        }

        capacity *= 2;
    }
}
int print_entry_aligned(const LsEntry *entry,
                const LsOptions *options,
                const DisplayWidths *widths)
{
    char mode_string[11];
    char uid_string[32];
    char gid_string[32];
    char time_string[32];
    time_t selected_time;
    char size_string[64];
    char human_size[5];
    const char *size_output;
    struct tm *local_time;
    const char *time_format;
    double age_seconds;
    const char *owner;
    const char *group_name;
    struct passwd *pw = NULL;
    struct group *gr = NULL;

    if (entry == NULL || options == NULL ||
        entry->name == NULL) {
        fprintf(stderr, "myls: invalid display arguments\n");
        return 1;
    }

    format_file_mode(entry->info.st_mode, mode_string);
    size_output = size_string;
    if (S_ISCHR(entry->info.st_mode) ||
        S_ISBLK(entry->info.st_mode)) {
        int length = snprintf(size_string, sizeof(size_string),
                              "%*ju, %*ju",
                              widths ? (int)widths->major_width : 0,
                              (uintmax_t)major(entry->info.st_rdev),
                              widths ? (int)widths->minor_width : 0,
                              (uintmax_t)minor(entry->info.st_rdev));
        if (length < 0 || (size_t)length >= sizeof(size_string)) {
            fprintf(stderr, "myls: cannot format device number\n");
            return 1;
        }
    } else if (options->size_mode == SIZE_HUMAN) {
        if (humanize_number(human_size, sizeof(human_size),
                            (int64_t)entry->info.st_size, "",
                            HN_AUTOSCALE,
                            HN_DECIMAL | HN_B | HN_NOSPACE) < 0) {
            perror("myls: humanize_number");
            return 1;
        }
        size_output = human_size;
    } else {
        int length = snprintf(size_string, sizeof(size_string),
                              "%jd", (intmax_t)entry->info.st_size);
        if (length < 0 || (size_t)length >= sizeof(size_string)) {
            perror("myls: format size");
            return 1;
        }
    }
    selected_time = entry->info.st_mtime;
    if (options->time_mode == TIME_CHANGED)
        selected_time = entry->info.st_ctime;
    else if (options->time_mode == TIME_ACCESSED)
        selected_time = entry->info.st_atime;

    local_time = localtime(&selected_time);
    age_seconds = difftime(time(NULL), selected_time);
    time_format = (age_seconds > -15724800.0 &&
                   age_seconds < 15724800.0)
        ? "%b %e %H:%M" : "%b %e  %Y";
    if (local_time == NULL ||
        strftime(time_string, sizeof(time_string),
                 time_format, local_time) == 0) {
        fprintf(stderr, "myls: timestamp error\n");
        return 1;
    }

    /* Resolve owner and group names unless -n is used. */
    if (options->output_format != FORMAT_NUMERIC) {
        pw = getpwuid(entry->info.st_uid);
        gr = getgrgid(entry->info.st_gid);
    }

    /* Owner name or numeric UID. */
    if (pw != NULL && pw->pw_name != NULL) {
        owner = pw->pw_name;
    } else {
        if (snprintf(uid_string, sizeof(uid_string),
                     "%ju",
                     (uintmax_t)entry->info.st_uid) < 0) {
            return 1;
        }
        owner = uid_string;
    }

    /* Group name or numeric GID. */
    if (gr != NULL && gr->gr_name != NULL) {
        group_name = gr->gr_name;
    } else {
        if (snprintf(gid_string, sizeof(gid_string),
                     "%ju",
                     (uintmax_t)entry->info.st_gid) < 0) {
            return 1;
        }
        group_name = gid_string;
    }

    /* Print inode number when -i is enabled. */
    if (options->show_inode) {
        if (printf("%*ju ",
                   widths ? (int)widths->inode_width : 0,
                   (uintmax_t)entry->info.st_ino) < 0) {
            perror("myls: printf inode");
            return 1;
        }
    }

    /* Print allocated blocks when -s is enabled. */
    if (options->show_blocks && options->size_mode == SIZE_HUMAN) {
        char human_blocks[5];
        if (humanize_number(human_blocks, sizeof(human_blocks),
                            (int64_t)entry->info.st_blocks * 512, "",
                            HN_AUTOSCALE,
                            HN_DECIMAL | HN_B | HN_NOSPACE) < 0) {
            perror("myls: humanize_number blocks");
            return 1;
        }
        if (printf("%*s ",
                   widths ? (int)widths->blocks_width : 0,
                   human_blocks) < 0) {
            perror("myls: printf blocks");
            return 1;
        }
    }
    if (options->show_blocks && options->size_mode != SIZE_HUMAN) {
        if (printf("%*ju ",
                   widths ? (int)widths->blocks_width : 0,
                   display_block_count(&entry->info, options)) < 0) {
            perror("myls: printf blocks");
            return 1;
        }
    }

    if (printf("%s  %*llu %-*s  %-*s  %*s %s ",
               mode_string,
               widths ? (int)widths->nlink_width : 0,
               (unsigned long long)entry->info.st_nlink,
               widths ? (int)widths->owner_width : 0,
               owner,
               widths ? (int)widths->group_width : 0,
               group_name,
               widths ? (int)widths->size_width : 0,
               size_output,
               time_string) < 0) {
        perror("myls: printf");
        return 1;
    }

    if (print_display_name(entry->name, options) != 0 ||
        fputs(classify_marker(&entry->info, options), stdout) == EOF) {
        perror("myls: print name");
        return 1;
    }

    if (S_ISLNK(entry->info.st_mode)) {
        if (print_link_target(entry->path) != 0)
            return 1;
    }

    if (puts("") == EOF) {
        perror("myls: puts");
        return 1;
    }
    return 0;
}

/* Keep the largest column width. */
static void keep_max_width(size_t *largest, size_t current)
{
    if (current > *largest) {
        *largest = current;
    }
}

/* Count the decimal digits of an unsigned integer. */
static size_t unsigned_width(uintmax_t value)
{
    size_t digits = 1;

    while (value >= 10) {
        value /= 10;
        digits++;
    }

    return digits;
}

/* Count characters needed to print a signed integer. */
static size_t signed_width(intmax_t value)
{
    int length = snprintf(NULL, 0, "%jd", value);

    return (length > 0) ? (size_t)length : 0;
}

/* Find maximum widths of long-format columns. */
void calculate_display_widths(const LsEntryList *list,
                              const LsOptions *options,
                              DisplayWidths *widths)
{
    size_t i;
    const LsEntry *entry;
    const char *owner;
    const char *group_name;
    struct passwd *pw;
    struct group *gr;

    if (widths == NULL) {
        return;
    }

    *widths = (DisplayWidths){0};

    if (list == NULL || options == NULL) {
        return;
    }

    for (i = 0; i < list->count; i++) {
        entry = &list->items[i];

        keep_max_width(&widths->inode_width,
                       unsigned_width((uintmax_t)entry->info.st_ino));

        if (S_ISCHR(entry->info.st_mode) ||
            S_ISBLK(entry->info.st_mode)) {
            keep_max_width(&widths->major_width,
                unsigned_width((uintmax_t)major(entry->info.st_rdev)));
            keep_max_width(&widths->minor_width,
                unsigned_width((uintmax_t)minor(entry->info.st_rdev)));
        }

        keep_max_width(&widths->blocks_width,
                       options->size_mode == SIZE_HUMAN ? 4 : unsigned_width(display_block_count(&entry->info, options)));
        keep_max_width(&widths->nlink_width,
                       unsigned_width((uintmax_t)entry->info.st_nlink));

        keep_max_width(&widths->size_width,
                       options->size_mode == SIZE_HUMAN ? 4 : signed_width((intmax_t)entry->info.st_size));

        owner = NULL;
        group_name = NULL;

        if (options->output_format != FORMAT_NUMERIC) {
            pw = getpwuid(entry->info.st_uid);
            gr = getgrgid(entry->info.st_gid);

            if (pw != NULL) {
                owner = pw->pw_name;
            }

            if (gr != NULL) {
                group_name = gr->gr_name;
            }
        }

        keep_max_width(
            &widths->owner_width,
            owner != NULL
                ? strlen(owner)
                : unsigned_width((uintmax_t)entry->info.st_uid)
        );

        keep_max_width(
            &widths->group_width,
            group_name != NULL
                ? strlen(group_name)
                : unsigned_width((uintmax_t)entry->info.st_gid)
        );
    }
    if (widths->major_width > 0) {
        size_t device_width = widths->major_width +
                              widths->minor_width + 2;

        if (widths->size_width < device_width)
            widths->size_width = device_width;
        else
            widths->major_width =
                widths->size_width - widths->minor_width - 2;
    }
}

/* Display a single entry without directory-wide widths. */
int print_entry(const LsEntry *entry,
                const LsOptions *options)
{
    return print_entry_aligned(entry, options, NULL);
}

/*
 * Return the file classification suffix.
 * An empty string means no suffix is needed.
 */
const char *classify_marker(const struct stat *info,
                            const LsOptions *options)
{
    mode_t mode;

    if (info == NULL || options == NULL ||
        !options->classify) {
        return "";
    }

    mode = info->st_mode;

    if (S_ISDIR(mode))
        return "/";

    if (S_ISLNK(mode))
        return "@";

    if (S_ISFIFO(mode))
        return "|";

    if (S_ISSOCK(mode))
        return "=";

    if (S_ISREG(mode) &&
        (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return "*";

    return "";
}

/*
 * Print a filename according to -q and -w.
 * -q replaces non-printable characters with '?'.
 * -w preserves the original filename.
 * Default behavior depends on whether stdout is a terminal.
 */
int print_display_name(const char *name, const LsOptions *options)
{
    const char *cursor;
    mbstate_t state = {0};
    int sanitize;

    if (name == NULL || options == NULL)
        return 1;

    sanitize = options->name_mode == NAME_QUESTION ||
               (options->name_mode == NAME_AUTO &&
                isatty(STDOUT_FILENO));

    if (!sanitize)
        return fputs(name, stdout) == EOF ? 1 : 0;

    cursor = name;

    while (*cursor != '\0') {
        wchar_t wc;
        size_t length = mbrtowc(&wc, cursor, MB_CUR_MAX, &state);

        if (length == (size_t)-1 ||
            length == (size_t)-2) {
            memset(&state, 0, sizeof(state));

            if (fputc('?', stdout) == EOF)
                return 1;

            cursor++;
            continue;
        }

        if (length == 0)
            break;

        if (iswprint(wc)) {
            if (fwrite(cursor, 1, length, stdout) != length)
                return 1;
        } else {
            if (fputc('?', stdout) == EOF)
                return 1;
        }

        cursor += length;
    }

    return 0;
}
