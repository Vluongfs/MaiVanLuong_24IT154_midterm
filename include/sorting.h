#ifndef SORTING_H
#define SORTING_H

#include "entries.h"
#include "options.h"

/*
 * Sort filesystem entries according to
 * the selected command-line options.
 */
void sort_entries(LsEntryList *list,
                  const LsOptions *options);

#endif
