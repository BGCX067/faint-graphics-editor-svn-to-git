#ifndef FAINT_PY_GRID_HH
#define FAINT_PY_GRID_HH
#include "util/idtypes.hh"
class CanvasInterface;

extern PyTypeObject GridType;
typedef struct {
  PyObject_HEAD
  CanvasInterface* canvas;
  CanvasId canvasId;
} gridObject;

PyObject* py_grid_canvas(CanvasInterface*);
PyObject* py_grid_app();
#endif
