#ifndef DACH_OBJECT_H
#define DACH_OBJECT_H
#undef T
#define DACH_SUPER_METHODS(T)                  \
  size_t size;                              \
  T (*ctor )(T self, va_list *arg);     \
  T (*dtor )(T self);                   \
  T (*clone)(const T self);             \
  T (*cmp  )(const T self, const T b);

#endif /* H_OBJECT_H */
