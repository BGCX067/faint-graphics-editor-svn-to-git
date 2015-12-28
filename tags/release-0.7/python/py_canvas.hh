#ifndef FAINT_PY_CANVAS_HH
#define FAINT_PY_CANVAS_HH
#include "util/commonfwd.hh"
#include "util/idtypes.hh"

class CanvasInterface;

extern PyTypeObject CanvasType;
typedef struct {
  PyObject_HEAD
  CanvasInterface* canvas;
  CanvasId id;
} canvasObject;

bool canvas_ok( const CanvasId& );
PyObject* pythoned( CanvasInterface& );

#endif
