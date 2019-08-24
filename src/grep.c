/**
 * @file
 * @brief grep utility
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef TEST
/**
 * Declare some functions with extern linkage, allowing the test suite to call
 * those functions.
 */
# define LINKAGE extern
# include "../test/seams.h"
#else /* !(TEST) */
/**
 * Define all functions as static when not testing.
 */
# define LINKAGE static
#endif /* TEST */

/**
 * @defgroup grep_exit_status possible exit status codes
 *
 * Exit status codes returned by grep.
 */

/**
 * At least one matched line.
 *
 * @ingroup grep_exit_status
 */
#define GREP_EXIT_MATCH   (0)

/**
 * No matching lines.
 *
 * @ingroup grep_exit_status
 */
#define GREP_EXIT_NOMATCH (1)

/**
 * Error occurred.
 *
 * @ingroup grep_exit_status
 */
#define GREP_EXIT_FAILURE (2)

/**
 * Regex or fixed string pattern.
 */
struct grep_pattern{
  /**
   * Copy of pattern string.
   */
  char *pattern;

  /**
   * Compiled form of @ref pattern if using regex search patterns.
   */
  regex_t regex;

  /**
   * Set to true if @ref regex has been compiled.
   */
  bool compiled;

  /**
   * Alignment padding.
   */
  char pad[7];
};

/**
 * grep utility context.
 */
struct grep_ctx{
  /**
   * See @ref grep_exit_status.
   */
  int status_code;

  /**
   * Number of files to search.
   */
  int num_files;

  /**
   * Print the number of matching lines.
   *
   * Corresponds to (-c) argument.
   */
  bool line_count;

  /**
   * Compile the pattern with extended regular expressions.
   *
   * Corresponds to (-E) argument.
   */
  bool extended_reg_expr;

  /**
   * Match strings exactly without the use of a regular expression.
   *
   * Corresponds to (-F) argument.
   */
  bool fixed_string;

  /**
   * Match using case-insensitive searching.
   *
   * Corresponds to (-i) argument.
   */
  bool case_insensitive;

  /**
   * Write file names with matching lines.
   *
   * Corresponds to (-l) argument.
   */
  bool write_file_names;

  /**
   * Write the line number of each match.
   *
   * Corresponds to (-n) argument.
   */
  bool line_number;

  /**
   * Do not write to STDOUT.
   *
   * Corresponds to (-q) argument.
   */
  bool quiet;

  /**
   * Ignore files that do not exist or cannot read from.
   *
   * Corresponds to (-s) argument.
   */
  bool ignore_file_error;

  /**
   * Consider only lines that do not match.
   *
   * Corresponds to (-v) argument.
   */
  bool invert_match;

  /**
   * Only check if entire line matches.
   *
   * Corresponds to (-x) argument.
   */
  bool full_string_match;

  /**
   * Alignment padding.
   */
  char pad[6];

  /**
   * See @ref grep_pattern.
   */
  struct grep_pattern *pattern_list;

  /**
   * Number of patterns in @ref pattern_list.
   */
  size_t num_patterns;

  /**
   * Comparison function used to match a pattern with a string.
   *
   * See @ref grep_set_fn_match.
   */
  bool
  (*fn_match)(const struct grep_pattern *const pattern,
              const char *const line);
};

/**
 * Add two size_t values and check for wrap.
 *
 * @param[in]  a      Add this value with @p b.
 * @param[in]  b      Add this value with @p a.
 * @param[out] result Save the result of the addition here.
 * @retval     true   Value wrapped.
 * @retval     false  Value did not wrap.
 */
LINKAGE bool
si_add_size_t(const size_t a,
              const size_t b,
              size_t *const result){
  bool wraps;

  *result = a + b;
  if(*result < a){
    wraps = true;
  }
  else{
    wraps = false;
  }
#ifdef TEST
  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_si_add_size_t)){
    return true;
  }
#endif /* TEST */
  return wraps;
}

/**
 * Multiply two size_t values and check for wrap.
 *
 * @param[in]  a      Multiply this value with @p b.
 * @param[in]  b      Multiply this value with @p a.
 * @param[out] result Save the result of the multiplication here.
 * @retval     true   Value wrapped.
 * @retval     false  Value did not wrap.
 */
LINKAGE bool
si_mul_size_t(const size_t a,
              const size_t b,
              size_t *const result){
  bool wraps;

  *result = a * b;
  if(b != 0 && a > SIZE_MAX / b){
    wraps = true;
  }
  else{
    wraps = false;
  }
#ifdef TEST
  if(test_seam_dec_err_ctr(&g_test_seam_err_ctr_si_mul_size_t)){
    return true;
  }
#endif /* TEST */
  return wraps;
}

/**
 * Look for a substring using a case-insensitive search.
 *
 * @param[in] s1    String to search.
 * @param[in] s2    Substring to search for in @p s1.
 * @retval    char* Pointer to beginning of substring in @p s1.
 * @retval    NULL  Substring not found.
 */
LINKAGE const char *
grep_strcasestr(const char *s1,
                const char *s2){
  if(*s2 == '\0'){
    return s1;
  }

  while(*s1){
    while(1){
      if(*s2 == '\0'){
        return s1;
      }
      else if(tolower(*s1) != tolower(*s2)){
        break;
      }
      s2 += 1;
    }
    s1 += 1;
  }
  return NULL;
}

/**
 * Print an error message to STDERR and set an error status code.
 *
 * @param[in,out] grep_ctx  See @ref grep_ctx.
 * @param[in]     errno_msg Include a standard message describing errno.
 * @param[in]     fmt       Format string used by vwarn.
 */
static void
grep_warn(struct grep_ctx *const grep_ctx,
          const bool errno_msg,
          const char *const fmt, ...){
  va_list ap;

  grep_ctx->status_code = GREP_EXIT_FAILURE;
  va_start(ap, fmt);
  if(errno_msg){
    vwarn(fmt, ap);
  }
  else{
    vwarnx(fmt, ap);
  }
  va_end(ap);
}

/**
 * Surround a regular expression pattern with BOL/EOL and parenthesis.
 *
 * This ensures the entire line will get matched by the regular expression.
 *
 * Example:
 * abc -> ^(abc)$
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 * @param[in,out] pattern  Updates the pattern string in this data structure.
 *                         See @ref grep_pattern.
 */
static void
grep_pattern_add_bol_eol(struct grep_ctx *const grep_ctx,
                         struct grep_pattern *const pattern){
  size_t alloc_sz;
  char *new_pattern;
  char *pattern_copy;

  /*
   * alloc_sz = 1 + 1 + strlen(pattern) + 1 + 1 + 1
   *            ^   (       pattern       )   $   \0
   */
  if(si_add_size_t(strlen(pattern->pattern), 5, &alloc_sz)){
    grep_warn(grep_ctx, false, "malloc: %zu", alloc_sz);
  }
  else{
    new_pattern = malloc(alloc_sz);
    if(new_pattern == NULL){
      grep_warn(grep_ctx, true, "malloc: %zu", alloc_sz);
    }
    else{
      pattern_copy = stpcpy(new_pattern, "^(");
      pattern_copy = stpcpy(pattern_copy, pattern->pattern);
      stpcpy(pattern_copy, ")$");
      free(pattern->pattern);
      pattern->pattern = new_pattern;
    }
  }
}

/**
 * Compile all regex patterns in @ref grep_ctx::pattern_list.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 */
static void
grep_compile_pattern_list(struct grep_ctx *const grep_ctx){
  size_t i;
  struct grep_pattern *pattern;
  int cflags;
  int errcode;
  char errbuf[1000];

  cflags = REG_NOSUB;
  if(grep_ctx->extended_reg_expr){
    cflags |= REG_EXTENDED;
  }
  if(grep_ctx->case_insensitive){
    cflags |= REG_ICASE;
  }
  for(i = 0; i < grep_ctx->num_patterns; i++){
    pattern = &grep_ctx->pattern_list[i];
    if(grep_ctx->full_string_match){
      grep_pattern_add_bol_eol(grep_ctx, pattern);
    }
    errcode = regcomp(&pattern->regex, pattern->pattern, cflags);
    if(errcode == 0){
      pattern->compiled = true;
    }
    else{
      regerror(errcode, NULL, errbuf, sizeof(errbuf));
      grep_warn(grep_ctx, false, "regcomp(%s): %s", pattern->pattern, errbuf);
    }
  }
}

/**
 * Remove all patterns in @ref grep_ctx::pattern_list.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 */
static void
grep_free_pattern_list(struct grep_ctx *const grep_ctx){
  size_t i;
  struct grep_pattern *pattern;

  for(i = 0; i < grep_ctx->num_patterns; i++){
    pattern = &grep_ctx->pattern_list[i];
    free(pattern->pattern);
    if(pattern->compiled){
      regfree(&pattern->regex);
      pattern->compiled = false;
    }
  }
  free(grep_ctx->pattern_list);
  grep_ctx->num_patterns = 0;
}

/**
 * Add another search pattern to @ref grep_ctx::pattern_list.
 *
 * @param[in,out] grep_ctx    See @ref grep_ctx.
 * @param[in]     pattern_str Copies this pointer to the new pattern structure.
 *                            The caller must have allocated this memory with
 *                            malloc or strdup.
 */
static void
grep_pattern_append(struct grep_ctx *const grep_ctx,
                    char *const pattern_str){
  struct grep_pattern *new_pattern_list;
  struct grep_pattern *new_pattern;
  size_t alloc_sz;

  /* (grep_ctx->num_patterns + 1) * sizeof(*grep_ctx->pattern_list) */
  if(si_add_size_t(grep_ctx->num_patterns, 1, &alloc_sz) ||
     si_mul_size_t(alloc_sz, sizeof(*grep_ctx->pattern_list), &alloc_sz)){
    grep_warn(grep_ctx, false, "realloc: %zu", alloc_sz);
    free(pattern_str);
  }
  else{
    new_pattern_list = realloc(grep_ctx->pattern_list, alloc_sz);
    if(new_pattern_list == NULL){
      grep_warn(grep_ctx, true, "realloc: %zu", alloc_sz);
      free(pattern_str);
    }
    else{
      grep_ctx->pattern_list = new_pattern_list;
      new_pattern = &grep_ctx->pattern_list[grep_ctx->num_patterns];
      grep_ctx->num_patterns += 1;
      new_pattern->pattern = pattern_str;
      new_pattern->compiled = false;
    }
  }
}

/**
 * Read patterns from a string.
 *
 * @param[in,out] grep_ctx    See @ref grep_ctx.
 * @param[in,out] pattern_str List of patterns separated by newline.
 */
static void
grep_pattern_string(struct grep_ctx *const grep_ctx,
                    char *const pattern_str){
  char *pattern_token;
  char *pattern_strdup;

  if(*pattern_str == '\0'){
    pattern_strdup = strdup("");
    if(pattern_strdup == NULL){
      grep_warn(grep_ctx, true, "strdup: \"\"");
    }
    else{
      grep_pattern_append(grep_ctx, pattern_strdup);
    }
  }
  else{
    for(pattern_token = strtok(pattern_str, "\n");
        pattern_token;
        pattern_token = strtok(NULL, "\n")){
      pattern_strdup = strdup(pattern_token);
      if(pattern_strdup == NULL){
        grep_warn(grep_ctx, true, "strdup: %s", pattern_str);
      }
      else{
        grep_pattern_append(grep_ctx, pattern_strdup);
      }
    }
  }
}

/**
 * Read patterns from a file.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 * @param[in]     path     Pattern file path.
 */
static void
grep_pattern_file(struct grep_ctx *const grep_ctx,
                  const char *const path){
  FILE *fp;
  char *line;
  size_t len;
  ssize_t read;

  fp = fopen(path, "r");
  if(fp == NULL){
    grep_warn(grep_ctx, true, "fopen: %s", path);
  }
  else{
    line = NULL;
    len = 0;
    while((read = getline(&line, &len, fp)) != -1){
      line[read - 1] = '\0';
      grep_pattern_append(grep_ctx, line);
      line = NULL;
    }
    free(line);
    if(ferror(fp)){
      grep_warn(grep_ctx, true, "ferror: %s", path);
    }
    if(fclose(fp) != 0){
      grep_warn(grep_ctx, true, "fclose: %s", path);
    }
  }
}

/**
 * Perform a case-sensitive search on the entire string.
 *
 * @param[in] pattern See @ref grep_pattern.
 * @param[in] line    String to search.
 * @retval    true    @p line matches pattern.
 * @retval    false   @p line does not match pattern.
 */
static bool
grep_match_fixed_strcmp(const struct grep_pattern *const pattern,
                        const char *const line){
  return strcmp(line, pattern->pattern) == 0;
}

/**
 * Perform a case-insensitive search on the entire string.
 *
 * @param[in] pattern See @ref grep_pattern.
 * @param[in] line    String to search.
 * @retval    true    @p line matches pattern.
 * @retval    false   @p line does not match pattern.
 */
static bool
grep_match_fixed_strcasecmp(const struct grep_pattern *const pattern,
                            const char *const line){
  return strcasecmp(line, pattern->pattern) == 0;
}

/**
 * Perform a case-sensitive search for a partial match of a string.
 *
 * @param[in] pattern See @ref grep_pattern.
 * @param[in] line    String to search.
 * @retval    true    @p line matches pattern.
 * @retval    false   @p line does not match pattern.
 */
static bool
grep_match_fixed_strstr(const struct grep_pattern *const pattern,
                        const char *const line){
  return strstr(line, pattern->pattern) != NULL;
}

/**
 * Perform a case-insensitive search for a partial match of a string.
 *
 * @param[in] pattern See @ref grep_pattern.
 * @param[in] line    String to search.
 * @retval    true    @p line matches pattern.
 * @retval    false   @p line does not match pattern.
 */
static bool
grep_match_fixed_strcasestr(const struct grep_pattern *const pattern,
                            const char *const line){
  return grep_strcasestr(line, pattern->pattern) != NULL;
}

/**
 * Perform a regular expression search on a line.
 *
 * @param[in] pattern See @ref grep_pattern.
 * @param[in] line    String to search.
 * @retval    true    @p line matches pattern.
 * @retval    false   @p line does not match pattern.
 */
static bool
grep_match_regex(const struct grep_pattern *const pattern,
                 const char *const line){
  return regexec(&pattern->regex, line, 0, NULL, 0) == 0;
}

/**
 * Set the matching function (@ref grep_ctx::fn_match) based on
 * the following criteria.
 *
 * Parameters |         Function
 * -----------|----------------------------
 *    -Fix    | @ref grep_match_fixed_strcasecmp
 *    -Fi     | @ref grep_match_fixed_strcasestr
 *    -Fx     | @ref grep_match_fixed_strcmp
 *    -F      | @ref grep_match_fixed_strstr
 *   OTHERS   | @ref grep_match_regex
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 */
static void
grep_set_fn_match(struct grep_ctx *const grep_ctx){
  if(grep_ctx->fixed_string){
    if(grep_ctx->case_insensitive){
      if(grep_ctx->full_string_match){
        grep_ctx->fn_match = grep_match_fixed_strcasecmp;
      }
      else{
        grep_ctx->fn_match = grep_match_fixed_strcasestr;
      }
    }
    else{
      if(grep_ctx->full_string_match){
        grep_ctx->fn_match = grep_match_fixed_strcmp;
      }
      else{
        grep_ctx->fn_match = grep_match_fixed_strstr;
      }
    }
  }
  else{
    grep_ctx->fn_match = grep_match_regex;
  }
}

/**
 * Check if any of the search patterns match this line.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 * @param[in]     line     Search this line.
 * @retval        true     At least one pattern matched @p line.
 * @retval        false    None of the patterns matched @p line.
 */
static bool
grep_match_output_line(struct grep_ctx *const grep_ctx,
                       const char *const line){
  size_t i;
  bool matched;
  struct grep_pattern *pattern;

  matched = false;
  for(i = 0; i < grep_ctx->num_patterns; i++){
    pattern = &grep_ctx->pattern_list[i];
    if(grep_ctx->fn_match(pattern, line)){
      matched = true;
    }
  }
  if(grep_ctx->invert_match){
    matched = !matched;
  }
  return matched;
}

/**
 * grep a file handle.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 * @param[in]     path     Name of file pointed to by @p fp.
 * @param[in,out] fp       File to search.
 */
static void
grep_match_output_fp(struct grep_ctx *const grep_ctx,
                     const char *const path,
                     FILE *const fp){
  char *line;
  size_t len;
  ssize_t read;
  int line_i;
  int match_i;

  line = NULL;
  len = 0;
  line_i = 1;
  match_i = 0;
  while((read = getline(&line, &len, fp)) != -1){
    line[read - 1] = '\0';
    if(grep_match_output_line(grep_ctx, line)){
      match_i += 1;
      if(grep_ctx->status_code != GREP_EXIT_FAILURE){
        grep_ctx->status_code = GREP_EXIT_MATCH;
      }
      if(grep_ctx->write_file_names){
        if(puts(path) == EOF){
          grep_warn(grep_ctx, true, "puts: %s", path);
        }
        break;
      }
      else{
        if(grep_ctx->quiet == false && grep_ctx->line_count == false){
          if(grep_ctx->num_files > 1){
            if(printf("%s:", path) < 0){
              grep_warn(grep_ctx, true, "fputs: %s", path);
            }
          }
          if(grep_ctx->line_number){
            if(printf("%d:", line_i) < 0){
              grep_warn(grep_ctx, true, "printf: %d", line_i);
            }
          }
          if(puts(line) == EOF){
            grep_warn(grep_ctx, true, "puts: stdout: %s", line);
          }
        }
      }
    }
    line_i += 1;
  }
  if(ferror(fp)){
    grep_warn(grep_ctx, true, path);
  }
  free(line);
  if(grep_ctx->line_count){
    if(grep_ctx->num_files > 1){
      if(printf("%s:", path) < 0){
        grep_warn(grep_ctx, true, "fputs: %s", path);
      }
    }
    if(printf("%d\n", match_i) < 0){
      grep_warn(grep_ctx, true, "printf: %d", match_i);
    }
  }
}

/**
 * Open a file and grep it.
 *
 * See @ref grep_match_output_fp.
 *
 * @param[in,out] grep_ctx See @ref grep_ctx.
 * @param[in]     path     File to search.
 */
static void
grep_match_output_path(struct grep_ctx *const grep_ctx,
                       const char *const path){
  FILE *fp;
  bool fopen_error_display;

  fp = fopen(path, "r");
  if(fp == NULL){
    if(grep_ctx->ignore_file_error){
      /* Supress error messages related to nonexistent and unreadable files. */
      switch(errno){
        case EACCES:
        case EISDIR:
        case ELOOP:
        case ENAMETOOLONG:
        case ENOENT:
        case ENOTDIR:
        case ENXIO:
          fopen_error_display = false;
          break;
        default:
          fopen_error_display = true;
          break;
      }
    }
    else{
      fopen_error_display = true;
    }
    if(fopen_error_display){
      grep_warn(grep_ctx, true, "fopen: %s", path);
    }
  }
  else{
    grep_match_output_fp(grep_ctx, path, fp);
    if(fclose(fp) != 0){
      grep_warn(grep_ctx, true, "fclose: %s", path);
    }
  }
}

/**
 * Main entry point for grep utility.
 *
 * Usage:
 *
 * grep [-E|-F] [-c|-l|-q] [-insvx] -e pattern_list [-e pattern_list]...
 *      [-f pattern_file]... [file...]
 *
 * grep [-E|-F] [-c|-l|-q] [-insvx] [-e pattern_list]...
 *      -f pattern_file [-f pattern_file]... [file...]
 *
 * grep [-E|-F] [-c|-l|-q] [-insvx] pattern_list [file...]
 *
 * @param[in]     argc Number of arguments in @p argv.
 * @param[in,out] argv Argument list.
 * @return             See @ref grep_ctx::status_code.
 */
LINKAGE int
grep_main(int argc,
          char *argv[]){
  int c;
  int i;
  struct grep_ctx grep_ctx;

  memset(&grep_ctx, 0, sizeof(grep_ctx));
  grep_ctx.status_code = GREP_EXIT_NOMATCH;
  while((c = getopt(argc, argv, "ce:Ef:Filnqsvx")) != -1){
    switch(c){
      case 'c':
        grep_ctx.line_count = true;
        break;
      case 'e':
        grep_pattern_string(&grep_ctx, optarg);
        break;
      case 'E':
        grep_ctx.extended_reg_expr = true;
        break;
      case 'f':
        grep_pattern_file(&grep_ctx, optarg);
        break;
      case 'F':
        grep_ctx.fixed_string = true;
        break;
      case 'i':
        grep_ctx.case_insensitive = true;
        break;
      case 'l':
        grep_ctx.write_file_names = true;
        break;
      case 'n':
        grep_ctx.line_number = true;
        break;
      case 'q':
        grep_ctx.quiet = true;
        break;
      case 's':
        grep_ctx.ignore_file_error = true;
        break;
      case 'v':
        grep_ctx.invert_match = true;
        break;
      case 'x':
        grep_ctx.full_string_match = true;
        break;
      default:
        grep_ctx.status_code = GREP_EXIT_FAILURE;
        break;
    }
  }
  argc -= optind;
  argv += optind;
  if(grep_ctx.extended_reg_expr && grep_ctx.fixed_string){
    grep_warn(&grep_ctx, false, "[-E|-F]");
  }
  if((grep_ctx.line_count && grep_ctx.write_file_names) ||
     (grep_ctx.line_count && grep_ctx.quiet) ||
     (grep_ctx.write_file_names && grep_ctx.quiet)){
    grep_warn(&grep_ctx, false, "[-c|-l|-q]");
  }
  if(grep_ctx.num_patterns == 0){
    if(argc < 1){
      grep_warn(&grep_ctx, false, "missing pattern_list");
    }
    else{
      grep_pattern_string(&grep_ctx, argv[0]);
      argc -= 1;
      argv += 1;
    }
  }
  grep_set_fn_match(&grep_ctx);
  if(!grep_ctx.fixed_string){
    grep_compile_pattern_list(&grep_ctx);
  }
  if(grep_ctx.status_code != GREP_EXIT_FAILURE){
    if(argc < 1){
      grep_ctx.num_files = 1;
      grep_match_output_fp(&grep_ctx, "(standard input)", stdin);
    }
    else{
      grep_ctx.num_files = argc;
      for(i = 0; i < argc; i++){
        grep_match_output_path(&grep_ctx, argv[i]);
      }
    }
  }
  grep_free_pattern_list(&grep_ctx);
  return grep_ctx.status_code;
}

#ifndef TEST
/**
 * Main program entry point.
 *
 * @param[in]     argc See @ref grep_main.
 * @param[in,out] argv See @ref grep_main.
 * @return             See @ref grep_main.
 */
int
main(int argc,
     char *argv[]){
  return grep_main(argc, argv);
}
#endif /* TEST */

