#ifndef FAINT_PY_BITMAP_HH
#define FAINT_PY_BITMAP_HH
#include "util/commonfwd.hh"

extern PyTypeObject BitmapType;
typedef struct {
  PyObject_HEAD
  faint::Bitmap* bmp;
} bitmapObject;

#endif
