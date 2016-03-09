#include "dach/dach_error.h"

#define CASE(num,s) case num: r_err = s; break;

void dach_runtime_error(const char* file, int line, int errnum) {
  const char *err_str = dach_strerror(errnum);
  fprintf(stderr, "%s:%i: Object in invalid state: %s == %i\n", file, line, err_str, errnum);
  return;
}

const char *dach_strerror(dach_errno eno) {
  const char *r_err = NULL;
  switch(eno) {
    CASE(DACH_SUCCESS,"no error")
    CASE(DACH_EBADF,"file descriptor not open")
    CASE(DACH_EFTYPE,"bad file type")
    CASE(DACH_EINVAL,"invalid argument")
    CASE(DACH_ENAMETOOLONG,"file name too long")
    CASE(DACH_ENOTEMPTY,"directory not empty")
    CASE(DACH_EACCES,"access denied")
    CASE(DACH_EAGAIN,"temporary failure")
    CASE(DACH_EEXIST,"file already exists")
    CASE(DACH_EINPROGRESS,"operation in progress")
    CASE(DACH_ENOMEM,"out of memory")
    CASE(DACH_ESPIPE,"illegal seek")
  }
  if(r_err == NULL) {
    r_err = "unknown error";
  }
  return r_err;
}

#undef CASE
