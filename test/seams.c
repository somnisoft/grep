/**
 * @file
 * @brief test seams
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

/**
 * Error counter for @ref test_seam_fclose.
 */
int g_test_seam_err_ctr_fclose = -1;

/**
 * Error counter for @ref test_seam_ferror.
 */
int g_test_seam_err_ctr_ferror = -1;

/**
 * Error counter for @ref test_seam_fopen.
 */
int g_test_seam_err_ctr_fopen  = -1;

/**
 * Error counter for @ref test_seam_malloc.
 */
int g_test_seam_err_ctr_malloc = -1;

/**
 * Error counter for @ref test_seam_printf.
 */
int g_test_seam_err_ctr_printf = -1;

/**
 * Error counter for @ref test_seam_puts.
 */
int g_test_seam_err_ctr_puts = -1;

/**
 * Error counter for @ref test_seam_realloc.
 */
int g_test_seam_err_ctr_realloc = -1;

/**
 * Error counter for @ref test_seam_strdup.
 */
int g_test_seam_err_ctr_strdup = -1;

/**
 * Error counter for @ref si_add_size_t.
 */
int g_test_seam_err_ctr_si_add_size_t = -1;

/**
 * Error counter for @ref si_mul_size_t.
 */
int g_test_seam_err_ctr_si_mul_size_t = -1;

/**
 * Decrement an error counter until it reaches -1.
 *
 * After a counter reaches -1, it will return a true response. This gets
 * used by the test suite to denote when to cause a function to fail. For
 * example, the unit test might need to cause the malloc() function to fail
 * after calling it a third time. In that case, the counter should initially
 * get set to 2 and will get decremented every time this function gets called.
 *
 * @param[in,out] err_ctr Error counter to decrement.
 * @retval        true    The counter has reached -1.
 * @retval        false   The counter has been decremented, but did not reach
 *                        -1 yet.
 */
bool
test_seam_dec_err_ctr(int *const err_ctr){
  bool reached_end;

  reached_end = false;
  if(*err_ctr >= 0){
    *err_ctr -= 1;
    if(*err_ctr < 0){
      reached_end = true;
    }
  }
  return reached_end;
}

/**
 * Control when fclose() fails.
 *
 * @param[in,out] stream File to close.
 * @retval        0      Closed the file.
 * @retval        EOF    Failed to close the file.
 */
int
test_seam_fclose(FILE *stream){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_fclose)){
    errno = EBADF;
    rc = EOF;
  }
  else{
    rc = fclose(stream);
  }
  return rc;
}

/**
 * Control when ferror() fails.
 *
 * @param[in] stream File stream.
 * @retval    0      Error status not set.
 * @retval    !0     Error status set.
 */
int
test_seam_ferror(FILE *stream){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_ferror)){
    rc = 1;
  }
  else{
    rc = ferror(stream);
  }
  return rc;
}

/**
 * Control when fopen() fails.
 *
 * @param[in] pathname Path pointing to file to open.
 * @param[in] mode     Mode string.
 * @retval    FILE*    File handle.
 * @retval    NULL     Failed to open file.
 */
FILE *
test_seam_fopen(const char *pathname,
                const char *mode){
  FILE *fp;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_fopen)){
    fp = NULL;
    errno = ENOMEM;
  }
  else{
    fp = fopen(pathname, mode);
  }
  return fp;
}

/**
 * Control when malloc() fails.
 *
 * @param[in] size  Number of bytes to allocate.
 * @retval    void* Pointer to allocated memory.
 * @retval    NULL  Memory allocation failed.
 */
void *
test_seam_malloc(size_t size){
  void *mem;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_malloc)){
    errno = ENOMEM;
    mem = NULL;
  }
  else{
    mem = malloc(size);
  }
  return mem;
}

/**
 * Control when printf() fails.
 *
 * @param[in] format Format string to print.
 * @retval    >=0    Number of bytes output.
 * @retval    <0     Failed to output string.
 */
int
test_seam_printf(const char *format, ...){
  int nbytes;
  va_list ap;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_printf)){
    nbytes = -1;
    errno = ENOMEM;
  }
  else{
    va_start(ap, format);
    nbytes = vprintf(format, ap);
    va_end(ap);
  }
  return nbytes;
}

/**
 * Control when puts() fails.
 *
 * @param[in] s   String to print.
 * @retval    >=0 Successfully printed string.
 * @retval    EOF Failed to print string.
 */
int
test_seam_puts(const char *s){
  int rc;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_puts)){
    rc = EOF;
    errno = ENOMEM;
  }
  else{
    rc = puts(s);
  }
  return rc;
}

/**
 * Control when realloc() fails.
 *
 * @param[in,out] ptr   Pointer to an existing allocated memory region, or
 *                      NULL to allocate a new memory region.
 * @param[in]     size  Number of bytes to allocate.
 * @retval        void* Pointer to allocated memory.
 * @retval        NULL  Memory allocation failed.
 */
void *
test_seam_realloc(void *ptr,
                  size_t size){
  void *mem;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_realloc)){
    errno = ENOMEM;
    mem = NULL;
  }
  else{
    mem = realloc(ptr, size);
  }
  return mem;
}

/**
 * Control when strdup() fails.
 *
 * @param[in] s     String to duplicate.
 * @retval    char* Duplicated string.
 * @retval    NULL  Failed to duplicate string.
 */
char *
test_seam_strdup(const char *s){
  char *str;

  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_strdup)){
    errno = ENOMEM;
    str = NULL;
  }
  else{
    str = strdup(s);
  }
  return str;
}

