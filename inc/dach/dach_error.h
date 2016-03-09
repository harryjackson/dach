#ifndef DACH_ERROR_H
#define DACH_ERROR_H
#include <stdio.h>
#include <stdlib.h> // abort() in macro
//#define DACH_M_ERROR_PRINT_ERRORS 1;
typedef int dach_status;

#define DACH_PRIV_START_ERROR    0xDEC0DE

#ifdef EACCES
#define DACH_DEF_EACCES EACCES
#else
#define DACH_DEF_EACCES         (DACH_PRIV_START_ERROR + 1)
#endif

#ifdef EEXIST
#define DACH_DEF_EEXIST EEXIST
#else
#define DACH_DEF_EEXIST         (DACH_PRIV_START_ERROR + 2)
#endif

#ifdef ENAMETOOLONG
#define DACH_DEF_ENAMETOOLONG ENAMETOOLONG
#else
#define DACH_DEF_ENAMETOOLONG   (DACH_PRIV_START_ERROR + 3)
#endif

#ifdef ENOENT
#define DACH_DEF_ENOENT ENOENT
#else
#define DACH_DEF_ENOENT         (DACH_PRIV_START_ERROR + 4)
#endif

#ifdef ENOTDIR
#define DACH_DEF_ENOTDIR ENOTDIR
#else
#define DACH_DEF_ENOTDIR        (DACH_PRIV_START_ERROR + 5)
#endif

#ifdef ENOSPC
#define DACH_DEF_ENOSPC ENOSPC
#else
#define DACH_DEF_ENOSPC         (DACH_PRIV_START_ERROR + 6)
#endif

#ifdef ENOMEM
#define DACH_DEF_ENOMEM ENOMEM
#else
#define DACH_DEF_ENOMEM         (DACH_PRIV_START_ERROR + 7)
#endif

#ifdef EMFILE
#define DACH_DEF_EMFILE EMFILE
#else
#define DACH_DEF_EMFILE         (DACH_PRIV_START_ERROR + 8)
#endif

#ifdef ENFILE
#define DACH_DEF_ENFILE ENFILE
#else
#define DACH_DEF_ENFILE         (DACH_PRIV_START_ERROR + 9)
#endif

#ifdef EBADF
#define DACH_DEF_EBADF EBADF
#else
#define DACH_DEF_EBADF          (DACH_PRIV_START_ERROR + 10)
#endif

#ifdef EINVAL
#define DACH_DEF_EINVAL EINVAL
#else
#define DACH_DEF_EINVAL         (DACH_PRIV_START_ERROR + 11)
#endif

#ifdef ESPIPE
#define DACH_DEF_ESPIPE ESPIPE
#else
#define DACH_DEF_ESPIPE         (DACH_PRIV_START_ERROR + 12)
#endif

#ifdef EAGAIN
#define DACH_DEF_EAGAIN EAGAIN
#else
#define DACH_DEF_EAGAIN         (DACH_PRIV_START_ERROR + 13)
#endif

#ifdef EWOULDBLOCK
#define DACH_DEF_EWOULDBLOCK EWOULDBLOCK
#else
#define DACH_DEF_EWOULDBLOCK    (DACH_PRIV_START_ERROR + 14)
#endif

#ifdef EINTR
#define DACH_DEF_EINTR EINTR
#else
#define DACH_DEF_EINTR          (DACH_PRIV_START_ERROR + 14)
#endif

#ifdef ENOTSOCK
#define DACH_DEF_ENOTSOCK ENOTSOCK
#else
#define DACH_DEF_ENOTSOCK       (DACH_PRIV_START_ERROR + 15)
#endif

#ifdef ECONNREFUSED
#define DACH_DEF_ECONNREFUSED ECONNREFUSED
#else
#define DACH_DEF_ECONNREFUSED   (DACH_PRIV_START_ERROR + 16)
#endif

#ifdef EINPROGRESS
#define DACH_DEF_EINPROGRESS EINPROGRESS
#else
#define DACH_DEF_EINPROGRESS    (DACH_PRIV_START_ERROR + 17)
#endif

#ifdef ECONNABORTED
#define DACH_DEF_ECONNABORTED ECONNABORTED
#else
#define DACH_DEF_ECONNABORTED   (DACH_PRIV_START_ERROR + 18)
#endif

#ifdef ECONNRESET
#define DACH_DEF_ECONNRESET ECONNRESET
#else
#define DACH_DEF_ECONNRESET     (DACH_PRIV_START_ERROR + 19)
#endif

#ifdef ETIMEDOUT
#define DACH_DEF_ETIMEDOUT ETIMEDOUT
#else
#define DACH_DEF_ETIMEDOUT      (DACH_PRIV_START_ERROR + 20)
#endif

#ifdef EHOSTUNREACH
#define DACH_DEF_EHOSTUNREACH EHOSTUNREACH
#else
#define DACH_DEF_EHOSTUNREACH   (DACH_PRIV_START_ERROR + 21)
#endif

#ifdef ENETUNREACH
#define DACH_DEF_ENETUNREACH ENETUNREACH
#else
#define DACH_DEF_ENETUNREACH    (DACH_PRIV_START_ERROR + 22)
#endif

#ifdef EFTYPE
#define DACH_DEF_EFTYPE EFTYPE
#else
#define DACH_DEF_EFTYPE        (DACH_PRIV_START_ERROR + 23)
#endif

#ifdef EPIPE
#define DACH_DEF_EPIPE EPIPE
#else
#define DACH_DEF_EPIPE         (DACH_PRIV_START_ERROR + 24)
#endif

#ifdef EXDEV
#define DACH_DEF_EXDEV EXDEV
#else
#define DACH_DEF_EXDEV         (DACH_PRIV_START_ERROR + 25)
#endif

#ifdef ENOTEMPTY
#define DACH_DEF_ENOTEMPTY ENOTEMPTY
#else
#define DACH_DEF_ENOTEMPTY     (DACH_PRIV_START_ERROR + 26)
#endif

#ifdef EAFNOSUPPORT
#define DACH_DEF_EAFNOSUPPORT EAFNOSUPPORT
#else
#define DACH_DEF_EAFNOSUPPORT  (DACH_PRIV_START_ERROR + 27)
#endif

typedef enum {
DACH_SUCCESS = 0,
DACH_EBADF = DACH_DEF_EBADF,
DACH_EFTYPE = DACH_DEF_EFTYPE,
DACH_EINVAL = DACH_DEF_EINVAL,
DACH_ENAMETOOLONG = DACH_DEF_ENAMETOOLONG,
DACH_ENOTEMPTY = DACH_DEF_ENOTEMPTY,
DACH_EACCES = DACH_DEF_EACCES,
DACH_EAGAIN = DACH_DEF_EAGAIN,
DACH_EEXIST = DACH_DEF_EEXIST,
DACH_EINPROGRESS = DACH_DEF_EINPROGRESS,
DACH_ENOMEM = DACH_DEF_ENOMEM,
DACH_ESPIPE = DACH_DEF_ESPIPE,
} dach_errno;

const char *dach_strerror(dach_errno eno);

void dach_runtime_error(const char* file, int line, int errnum);


#define DACH_CHECK_ERRNO(e)                                         \
    do {                                                            \
      dach_error_t *dach_err_t = (dach_error_t *) e;                \
      if (dach_err_t->errno > 0) {                                  \
        dach_runtime_error( __FILE__, __LINE__, dach_err_t->errno); \
      }                                                             \
    } while (0);


#define DACH_OBJ_VALID(e)  \
    ((void) ((!(e)) ? (void)0 : dach_error_object_valid(#e, e, __FILE__, __LINE__)))
#define dach_error_object_valid(code_str, code_int, file, line) \
    ((void)printf ("%s:%u: Object in invalid state: %s == %i\n", file, line, code_str, code_int), abort())

#endif				/* DACH_ERROR_H */
