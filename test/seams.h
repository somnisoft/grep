/**
 * @file
 * @brief test seams
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */
#ifndef GREP_TEST_SEAMS_H
#define GREP_TEST_SEAMS_H

#include "test.h"

/*
 * Redefine these functions to internal test seams.
 */
#undef fclose
#undef ferror
#undef fopen
#undef malloc
#undef printf
#undef puts
#undef realloc
#undef strdup

/**
 * Inject a test seam to replace fclose().
 */
#define fclose test_seam_fclose

/**
 * Inject a test seam to replace ferror().
 */
#define ferror test_seam_ferror

/**
 * Inject a test seam to replace fopen().
 */
#define fopen  test_seam_fopen

/**
 * Inject a test seam to replace malloc().
 */
#define malloc test_seam_malloc

/**
 * Inject a test seam to replace printf().
 */
#define printf  test_seam_printf

/**
 * Inject a test seam to replace puts().
 */
#define puts    test_seam_puts

/**
 * Inject a test seam to replace realloc().
 */
#define realloc test_seam_realloc

/**
 * Inject a test seam to replace strdup().
 */
#define strdup  test_seam_strdup

#endif /* GREP_TEST_SEAMS_H */

