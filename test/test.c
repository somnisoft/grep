/**
 * @file
 * @brief test suite
 * @author James Humphrey (humphreyj@somnisoft.com)
 *
 * This software has been placed into the public domain using CC0.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

/**
 * Generate the output of grep to this file.
 */
#define PATH_TMP_FILE "build/test-grep.txt"

/**
 * Test file used to search for patterns.
 */
#define PATH_SEARCH "test/search.txt"

/**
 * Number of arguments in @ref g_argv.
 */
static int
g_argc;

/**
 * Argument list to pass to utility.
 */
static char **
g_argv;

/**
 * Call @ref grep_main with the given arguments.
 *
 * @param[in] c                  See @ref grep_main.
 * @param[in] e                  See @ref grep_main.
 * @param[in] E                  See @ref grep_main.
 * @param[in] f                  See @ref grep_main.
 * @param[in] F                  See @ref grep_main.
 * @param[in] i                  See @ref grep_main.
 * @param[in] l                  See @ref grep_main.
 * @param[in] n                  See @ref grep_main.
 * @param[in] q                  See @ref grep_main.
 * @param[in] s                  See @ref grep_main.
 * @param[in] v                  See @ref grep_main.
 * @param[in] x                  See @ref grep_main.
 * @param[in] invalid_arg        Invalid argument.
 * @param[in] pattern_list       See @ref grep_main.
 * @param[in] pattern_file       See @ref grep_main.
 * @param[in] expect_ref_file    Reference file containing expected grep
 *                               output.
 * @param[in] stdin_data         Pipe this string to stdin.
 * @param[in] expect_exit_status Expected exit status code.
 * @param[in] file_list          See @ref grep_main.
 */
static void
test_grep_main(const bool c,
               const bool e,
               const bool E,
               const bool f,
               const bool F,
               const bool i,
               const bool l,
               const bool n,
               const bool q,
               const bool s,
               const bool v,
               const bool x,
               const bool invalid_arg,
               const char *const pattern_list,
               const char *const pattern_file,
               const char *const stdin_data,
               const char *const expect_ref_file,
               const int expect_exit_status,
               const char *const file_list, ...){
  pid_t pid;
  const char *file;
  int status;
  int exit_status;
  int cmp_exit_status;
  va_list ap;
  FILE *new_stdout;
  size_t stdin_bytes_len;
  ssize_t bytes_written;
  int pipe_stdin[2];
  char cmp_cmd[1000];

  g_argc = 1;
  if(c){
    strcpy(g_argv[g_argc++], "-c");
  }
  if(e){
    strcpy(g_argv[g_argc++], "-e");
  }
  if(E){
    strcpy(g_argv[g_argc++], "-E");
  }
  if(f){
    strcpy(g_argv[g_argc++], "-f");
  }
  if(F){
    strcpy(g_argv[g_argc++], "-F");
  }
  if(i){
    strcpy(g_argv[g_argc++], "-i");
  }
  if(l){
    strcpy(g_argv[g_argc++], "-l");
  }
  if(n){
    strcpy(g_argv[g_argc++], "-n");
  }
  if(q){
    strcpy(g_argv[g_argc++], "-q");
  }
  if(s){
    strcpy(g_argv[g_argc++], "-s");
  }
  if(v){
    strcpy(g_argv[g_argc++], "-v");
  }
  if(x){
    strcpy(g_argv[g_argc++], "-x");
  }
  if(invalid_arg){
    strcpy(g_argv[g_argc++], "-z");
  }
  if(pattern_list){
    strcpy(g_argv[g_argc++], "-e");
    strcpy(g_argv[g_argc++], pattern_list);
  }
  if(pattern_file){
    strcpy(g_argv[g_argc++], "-f");
    strcpy(g_argv[g_argc++], pattern_file);
  }
  va_start(ap, file_list);
  for(file = file_list; file; file = va_arg(ap, const char *const)){
    strcpy(g_argv[g_argc++], file);
  }
  va_end(ap);
  if(stdin_data){
    stdin_bytes_len = strlen(stdin_data);
    assert(pipe(pipe_stdin) == 0);
  }
  else{
    stdin_bytes_len = 0;
  }
  pid = fork();
  assert(pid >= 0);
  if(pid == 0){
    if(stdin_data){
      assert(close(pipe_stdin[1]) == 0);
      assert(dup2(pipe_stdin[0], STDIN_FILENO) >= 0);
      assert(close(pipe_stdin[0]) == 0);
    }
    new_stdout = freopen(PATH_TMP_FILE, "w", stdout);
    assert(new_stdout);
    exit_status = grep_main(g_argc, g_argv);
    exit(exit_status);
  }
  if(stdin_data){
    assert(close(pipe_stdin[0]) == 0);
    bytes_written = write(pipe_stdin[1], stdin_data, stdin_bytes_len);
    assert(bytes_written >= 0 && (size_t)bytes_written == stdin_bytes_len);
    assert(close(pipe_stdin[1]) == 0);
  }
  assert(waitpid(pid, &status, 0) == pid);
  assert(WIFEXITED(status));
  assert(WEXITSTATUS(status) == expect_exit_status);
  if(expect_ref_file){
    sprintf(cmp_cmd, "cmp '%s' '%s'", PATH_TMP_FILE, expect_ref_file);
    cmp_exit_status = system(cmp_cmd);
    assert(cmp_exit_status == 0);
  }
}

/**
 * Test harness for @ref si_add_size_t and @ref si_mul_size_t.
 *
 * @param[in] si_op_size_t Set to @ref si_add_size_t or @ref si_mul_size_t.
 * @param[in] a            First operand value.
 * @param[in] b            Second operand value.
 * @param[in] expect_calc  Expected result of calculation.
 * @param[in] expect_wraps Expected wrap result.
 */
static void
test_si_op_size(bool (*si_op_size_t)(const size_t a,
                                     const size_t b,
                                     size_t *const result),
                const size_t a,
                const size_t b,
                const size_t expect_calc,
                const bool expect_wraps){
  bool wraps;
  size_t result;

  wraps = si_op_size_t(a, b, &result);
  assert(expect_wraps == wraps);
  if(wraps == false){
    assert(result == expect_calc);
  }
}

/**
 * Run all test cases for the si_* functions.
 */
static void
test_all_si(void){
  test_si_op_size(si_add_size_t, 0, 1, 1, false);
  test_si_op_size(si_add_size_t, SIZE_MAX, 1, 0, true);

  test_si_op_size(si_mul_size_t, 2, 2, 4, false);
  test_si_op_size(si_mul_size_t, 2, 0, 0, false);
  test_si_op_size(si_mul_size_t, SIZE_MAX / 2, 2, SIZE_MAX - 1, false);
  test_si_op_size(si_mul_size_t, SIZE_MAX, 2, SIZE_MAX / 2, true);
}

/**
 * Test harness for @ref grep_strcasestr.
 *
 * @param[in] s1     String to search.
 * @param[in] s2     Substring to search for in @p s1.
 * @param[in] expect Expected result from @ref grep_strcasestr.
 */
static void
test_strcasestr(const char *const s1,
                const char *const s2,
                const char *const expect){
  const char *result;

  result = grep_strcasestr(s1, s2);
  if(expect == NULL){
    assert(result == expect);
  }
  else{
    assert(strcmp(result, expect) == 0);
  }
}

/**
 * Run all test cases for the @ref grep_strcasestr function.
 */
static void
test_all_strcasestr(void){
  test_strcasestr(""   , ""  , ""   );
  test_strcasestr("a"  , ""  , "a"  );
  test_strcasestr("a"  , "a" , "a"  );
  test_strcasestr("a"  , "b" , NULL );
  test_strcasestr("aa" , "a" , "aa" );
  test_strcasestr("aa" , "aa", "aa" );
  test_strcasestr("aba", "a" , "aba");
  test_strcasestr("abc", "b" , "bc" );
  test_strcasestr("abc", "B" , "bc" );
  test_strcasestr("aBc", "b" , "Bc" );
  test_strcasestr("abc", "c" , "c"  );
  test_strcasestr("abc", "d" , NULL );
  test_strcasestr("abc", ""  , "abc");
}

/**
 * Run through different error scenarios.
 */
static void
test_all_errors(void){
  int i;

  /* Invalid argument. */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 2,
                 NULL);

  /* -E and -F */
  test_grep_main(false,
                 false,
                 true,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 2,
                 NULL);

  /* -c and -l */
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 2,
                 NULL);

  /* -c and -q */
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 2,
                 NULL);

  /* -l and -q */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 2,
                 NULL);

  /* Search file does not exist. */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "test",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 "build/noexist",
                 NULL);

  /* Pattern file does not exist. */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "test",
                 "build/noexist",
                 NULL,
                 NULL,
                 2,
                 "README.md",
                 NULL);

  /* Fail during file reads. */
  for(i = 0; i < 2; i++){
    g_test_seam_err_ctr_ferror = i;
    test_grep_main(false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   "test",
                   "README.md",
                   NULL,
                   NULL,
                   2,
                   "README.md",
                   NULL);
    g_test_seam_err_ctr_ferror = -1;
  }

  /* Fail to close pattern and search files. */
  for(i = 0; i < 2; i++){
    g_test_seam_err_ctr_fclose = i;
    test_grep_main(false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   "test",
                   "README.md",
                   NULL,
                   NULL,
                   2,
                   "README.md",
                   NULL);
    g_test_seam_err_ctr_fclose = -1;
  }

  g_test_seam_err_ctr_realloc = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "test",
                 "README.md",
                 NULL,
                 NULL,
                 2,
                 "README.md",
                 NULL);
  g_test_seam_err_ctr_realloc = -1;

  g_test_seam_err_ctr_strdup = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_strdup = -1;

  for(i = 0; i < 2; i++){
    g_test_seam_err_ctr_strdup = 0;
    test_grep_main(false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   "test",
                   "README.md",
                   NULL,
                   NULL,
                   2,
                   "README.md",
                   NULL);
    g_test_seam_err_ctr_strdup = -1;
  }

  for(i = 0; i < 2; i++){
    g_test_seam_err_ctr_si_add_size_t = i;
    test_grep_main(false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   false,
                   true,
                   false,
                   "test",
                   NULL,
                   NULL,
                   NULL,
                   2,
                   "README.md",
                   NULL);
    g_test_seam_err_ctr_si_add_size_t = -1;
  }

  g_test_seam_err_ctr_si_mul_size_t = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "test",
                 "README.md",
                 NULL,
                 NULL,
                 2,
                 "README.md",
                 NULL);
  g_test_seam_err_ctr_si_mul_size_t = -1;

  /* malloc fails in bol/eol. */
  g_test_seam_err_ctr_malloc = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 "test",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 "README.md",
                 NULL);
  g_test_seam_err_ctr_malloc = -1;

  /* Invalid regular expression. */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "[abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 "README.md",
                 NULL);

  /* puts line failed. */
  g_test_seam_err_ctr_puts = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_puts = -1;

  /*
   * regex
   * multiple files
   * printf fails
   */
  g_test_seam_err_ctr_printf = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_printf = -1;

  /*
   * regex
   * file names
   * puts fails
   */
  g_test_seam_err_ctr_puts = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_puts = -1;

  /*
   * regex
   * line count
   * multiple files
   * printf fails
   */
  g_test_seam_err_ctr_printf = 0;
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_printf = -1;

  /*
   * regex
   * line numbers
   * printf fails
   */
  g_test_seam_err_ctr_printf = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_printf = -1;

  /*
   * suppress errors
   * fopen error unrelated to nonexistent or unreadable file
   */
  g_test_seam_err_ctr_fopen = 0;
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_fopen = -1;
}

/**
 * Test grep and compare the expected output with reference files.
 */
static void
test_all_grep_ref(void){
  /*
   * regex
   * pattern file
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 NULL,
                 "test/pattern_file.txt",
                 NULL,
                 "build/r.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * NULL BRE shall match every line.
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "",
                 NULL,
                 NULL,
                 PATH_SEARCH,
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * NULL ERE shall match every line.
   */
  test_grep_main(false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "",
                 NULL,
                 NULL,
                 PATH_SEARCH,
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * NULL fixed string shall match every line.
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "",
                 NULL,
                 NULL,
                 PATH_SEARCH,
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * NULL fixed string case insensitive shall match every line.
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "",
                 NULL,
                 NULL,
                 PATH_SEARCH,
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * case sensitive
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/r.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * case insensitive
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/ri.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * fixed string
   * case sensitive
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/F.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * fixed string
   * case sensitive
   * match entire string
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/Fx.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * fixed string
   * case insensitive
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/Fi.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * fixed string
   * case insensitive
   * match entire string
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 true,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/Fix.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * count of selected lines
   * multiple files
   */
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/c_multi_files.txt",
                 0,
                 PATH_SEARCH,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * line numbers
   * multiple files
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/n_multi_files.txt",
                 0,
                 PATH_SEARCH,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * multiple files
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/multi_files.txt",
                 0,
                 PATH_SEARCH,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * line count
   */
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/c.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * line count
   * printf fails
   */
  g_test_seam_err_ctr_printf = 0;
  test_grep_main(true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 2,
                 PATH_SEARCH,
                 NULL);
  g_test_seam_err_ctr_printf = -1;

  /*
   * regex
   * file names
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/l.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * line numbers
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/n.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * quiet
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/q.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regex
   * inverted match
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/v.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regular expression (basic)
   * curly braces (unescaped)
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "127{1}",
                 NULL,
                 NULL,
                 "build/basic_curly.txt",
                 1,
                 PATH_SEARCH,
                 NULL);

  /*
   * regular expression (basic)
   * curly braces (escaped)
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "127\\{1\\}",
                 NULL,
                 NULL,
                 "build/basic_curly_escaped.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * regular expression (extended)
   * curly braces
   */
  test_grep_main(false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "127{1}",
                 NULL,
                 NULL,
                 "build/E_curly.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * suppress errors
   * file does exist
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 "build/r.txt",
                 0,
                 PATH_SEARCH,
                 NULL);

  /*
   * suppress errors
   * file does not exist
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 true,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 NULL,
                 NULL,
                 1,
                 "build/noexist.txt",
                 NULL);

  /*
   * regex
   * stdin
   */
  test_grep_main(false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 false,
                 "abc",
                 NULL,
                 "123\nabcdefg\nabc\n456",
                 NULL,
                 0,
                 NULL);
}

/**
 * Run all test cases for grep.
 */
static void
test_all(void){
  test_all_si();
  test_all_strcasestr();
  test_all_errors();
  test_all_grep_ref();
}

/**
 * Test grep utility.
 *
 * Usage: test
 *
 * @retval 0 All tests passed.
 */
int
main(void){
  const size_t MAX_ARGS = 20;
  const size_t MAX_ARG_LEN = 255;
  size_t i;

  g_argv = malloc(MAX_ARGS * sizeof(g_argv));
  assert(g_argv);
  for(i = 0; i < MAX_ARGS; i++){
    g_argv[i] = malloc(MAX_ARG_LEN * sizeof(*g_argv));
    assert(g_argv[i]);
  }
  g_argc = 0;
  strcpy(g_argv[g_argc++], "grep");
  test_all();
  for(i = 0; i < MAX_ARGS; i++){
    free(g_argv[i]);
  }
  free(g_argv);
  return 0;
}

