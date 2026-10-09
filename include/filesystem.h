#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "options.h"

/*
 * List entries inside a directory.
 */
int list_directory(const char *path,
                   const LsOptions *options);

/*
 * Handle a file or directory path.
 */
int list_path(const char *path,
              const LsOptions *options);

#endif
