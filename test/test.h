/**
 * @file
 * @brief test suite
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */
#ifndef GREP_TEST_H
#define GREP_TEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool
si_add_size_t(const size_t a,
              const size_t b,
              size_t *const result);

bool
si_mul_size_t(const size_t a,
              const size_t b,
              size_t *const result);

const char *
grep_strcasestr(const char *s1,
                const char *s2);

int
grep_main(int argc,
          char *argv[]);

bool
test_seam_dec_err_ctr(int *const err_ctr);

int
test_seam_fclose(FILE *stream);

int
test_seam_ferror(FILE *stream);

FILE *
test_seam_fopen(const char *pathname,
                const char *mode);

void *
test_seam_malloc(size_t size);

int
test_seam_printf(const char *format, ...);

int
test_seam_puts(const char *s);

void *
test_seam_realloc(void *ptr,
                  size_t size);

char *
test_seam_strdup(const char *s);

extern int g_test_seam_err_ctr_fclose;
extern int g_test_seam_err_ctr_ferror;
extern int g_test_seam_err_ctr_fopen;
extern int g_test_seam_err_ctr_malloc;
extern int g_test_seam_err_ctr_printf;
extern int g_test_seam_err_ctr_puts;
extern int g_test_seam_err_ctr_realloc;
extern int g_test_seam_err_ctr_strdup;
extern int g_test_seam_err_ctr_si_add_size_t;
extern int g_test_seam_err_ctr_si_mul_size_t;

#endif /* GREP_TEST_H */

