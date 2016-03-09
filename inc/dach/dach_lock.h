#ifndef DACH_LOCK_H
#define DACH_LOCK_H

typedef struct dach_lock     dach_lock;

typedef struct dach_lock_ops {
  int (*lock   )(dach_lock *lock);
  int (*unlock )(dach_lock *lock);
} dach_lock_ops;

struct dach_lock {
  void           *obj;
  dach_lock_ops *ops;
};

dach_lock  *dach_lock_new (void);
void        dach_lock_free(dach_lock *lock);

#endif /* DACH_LOCK_H */
