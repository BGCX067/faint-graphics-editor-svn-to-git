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

// Returns a grid object targetting the specified canvas.
// If the specified canvas is 0, the grid object will
// always target the active canvas.
PyObject* py_grid_canvas(CanvasInterface*);

// Returns the Grid-class for this gridObject.
// Note: the gridObject must return true for grid_ok
Grid get_grid( gridObject* );

// Returns true if the canvas for the grid exists.
// Otherwise, sets a python error and returns false.
bool grid_ok( gridObject* );
#endif
