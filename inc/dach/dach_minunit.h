#ifndef DACH_MINUNIT_H
#define DACH_MINUNIT_H
/* 
 * This is the simplest unit testing framework I could come up with. Inspiration 
 * taken from http://www.jera.com/techinfo/jtns/jtn002.html
 */
// Not thread safe
static int dach_tests_run;
static int dach_tests_fail;

#define dach_assert(message, dach_true) do {\
  dach_tests_run++;\
  if (dach_true) {\
    fprintf(stderr, "ok   %i %s\n", dach_tests_run, message);\
  }\
  else {\
    dach_tests_fail++;\
    fprintf(stderr, "nok  %i %s\n", dach_tests_run, message);\
}} while (0)

#define dach_run_test(fn_name) do {\
  int dach_pass = fn_name();\
  if (dach_pass) {\
    fprintf(stderr, "ok   ..finished %s()\n", #fn_name);\
  }\
  else {\
    dach_tests_fail++;\
    fprintf(stderr, "nok  ..finished %s()\n", #fn_name);\
  }} while (0)
#endif /* DACH_MINUNIT_H */
