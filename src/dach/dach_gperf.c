#include "dach/dach_gperf.h"
/* ANSI-C code produced by gperf version 3.0.3 */
/* Command-line: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/gperf --output-file=gperf.c gperf_in.gperf  */
/* Computed positions: -k'6' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#include <string.h>
#include <stddef.h>
enum
  {
    TOTAL_KEYWORDS = 6,
    MIN_WORD_LENGTH = 8,
    MAX_WORD_LENGTH = 11,
    MIN_HASH_VALUE = 8,
    MAX_HASH_VALUE = 15
  };

/* maximum key range = 8, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
dach_gperf_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16,  5,  0, 16,
      16, 16, 16, 16,  5, 16, 16, 16,  0, 16,
      16, 16, 16, 16, 16,  0, 16, 16, 16, 16,
      16, 16, 16, 16, 16, 16, 16, 16
    };
  return len + asso_values[(unsigned char)str[5]];
}

const char *
in_word_set (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      unsigned int key = dach_gperf_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register const char *resword;

          if (key < 10)
            {
              switch (key - 8)
                {
                  case 0:
                    resword = "dach_set";
                    goto compare;
                  case 1:
                    resword = "dach_list";
                    goto compare;
                }
            }
          else
            {
              if (key < 14)
                {
                  switch (key - 10)
                    {
                      case 0:
                        resword = "dach_block";
                        goto compare;
                      case 1:
                        resword = "dach_stream";
                        goto compare;
                    }
                }
              else
                {
                  switch (key - 14)
                    {
                      case 0:
                        resword = "dach_hash";
                        goto compare;
                      case 1:
                        resword = "dach_arena";
                        goto compare;
                    }
                }
            }
          return 0;
        compare:
          if (*str == *resword && !strcmp (str + 1, resword + 1))
            return resword;
        }
    }
  return 0;
}
#line 18 "gperf_in.gperf"

