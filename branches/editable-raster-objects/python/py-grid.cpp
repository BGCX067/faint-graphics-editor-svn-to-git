#include "app/getappcontext.hh"
#include "geo/grid.hh"
#include "python/pyinterface.hh"
#include "python/pythoninclude.hh"
#include "python/py-grid.hh"
#include "python/py-util.hh"
#include "util/canvasinterface.hh"

// Returns true if the canvas for the grid exists.
// Otherwise, sets a python error and returns false.
bool grid_ok( gridObject* self ){
  if ( self->canvas == nullptr ){
    // A null-canvas means the active canvas is the target, There
    // should always be atleast one canvas.
    return true;
  }
  else if ( GetAppContext().Exists(self->canvasId) ){
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
  return false;
}

static CanvasInterface& get_canvas( gridObject* self ){
  return self->canvas == nullptr ?
    GetAppContext().GetActiveCanvas() :
    *self->canvas;
}

Grid get_grid( gridObject* self ){
  return get_canvas(self).GetGrid();
}

static PyObject* grid_new( PyTypeObject* type, PyObject*, PyObject* ){
  gridObject* self = (gridObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static int grid_init(gridObject* self, PyObject* /*args*/, PyObject*){
  self->canvas = nullptr;
  self->canvasId = CanvasId::DefaultID();
  return faint::init_ok;
}

PyObject* py_grid_canvas( CanvasInterface* canvas ){
  gridObject* py_grid = (gridObject*) GridType.tp_alloc(&GridType,0);
  py_grid->canvas = canvas;
  if ( canvas != nullptr ){
    py_grid->canvasId = canvas->GetId();
  }
  return (PyObject*) py_grid;
}

PyObject* grid_get_color( gridObject* self, void* ){
  if ( !grid_ok(self) ){
    return nullptr;
  }
  CanvasInterface& canvas = get_canvas(self);
  return build_color_tuple(canvas.GetGrid().Color());
}

int grid_set_color( gridObject* self, PyObject* args, void* ){
  if ( !grid_ok(self) ){
    return faint::setter_fail;
  }

  faint::Color color;
  if ( !parse_color(args, color) ){
    return faint::setter_fail;
  }

  CanvasInterface& canvas = get_canvas(self);
  Grid g = canvas.GetGrid();
  if ( g.Color() != color ){
    g.SetColor(color);
    canvas.SetGrid(g);
    faint::python_queue_refresh(&canvas);
  }
  return faint::setter_ok;
}

PyObject* grid_get_dashed( gridObject* self, void* ){
  if ( !grid_ok(self) ){
    return nullptr;
  }
  CanvasInterface& canvas = get_canvas(self);
  bool dashed = canvas.GetGrid().Dashed();
  return dashed ? Py_True : Py_False;
}

int grid_set_dashed( gridObject* self, PyObject* args, void* ){
  if ( !grid_ok(self) ){
    return faint::setter_fail;
  }
  CanvasInterface& canvas = get_canvas(self);
  bool dashed = args == Py_True;
  Grid g = canvas.GetGrid();
  if ( g.Dashed() != dashed ){
    g.SetDashed(dashed);
    canvas.SetGrid(g);
    faint::python_queue_refresh(&canvas);
  }
  return faint::setter_ok;
}

PyObject* grid_get_spacing( gridObject* self, void* ){
  if ( !grid_ok(self) ){
    return nullptr;
  }
  CanvasInterface& canvas = get_canvas(self);
  return Py_BuildValue( "i", canvas.GetGrid().Spacing() );
}

int grid_set_spacing( gridObject* self, PyObject* args, void* ){
  if ( !grid_ok(self) ){
    return faint::setter_fail;
  }

  if ( !PyNumber_Check(args) ){
    PyErr_SetString(PyExc_TypeError, "Grid spacing must be a number");
    return faint::setter_fail;
  }
  int spacing = PyInt_AsLong(args);
  if ( spacing == -1 && faint::py_error_occurred() ){
    return faint::setter_fail;
  }
  CanvasInterface& canvas = get_canvas(self);
  Grid g = canvas.GetGrid();
  g.SetSpacing(spacing);
  canvas.SetGrid(g);
  faint::python_queue_refresh(&canvas);
  return faint::setter_ok;
}

PyObject* grid_get_visible( gridObject* self, void* ){
  if ( !grid_ok(self) ){
    return nullptr;
  }
  CanvasInterface& canvas = get_canvas(self);
  bool dashed = canvas.GetGrid().Visible();
  return dashed ? Py_True : Py_False;
}

int grid_set_visible( gridObject* self, PyObject* arg, void* ){
  if ( !grid_ok(self) ){
    return faint::setter_fail;
  }
  bool visible = 1 == PyObject_IsTrue(arg);
  CanvasInterface& canvas = get_canvas(self);
  Grid g = canvas.GetGrid();
  if ( g.Visible() != visible ){
    g.SetVisible(visible);
    canvas.SetGrid(g);
    faint::python_queue_refresh(&canvas);
  }
  return faint::setter_ok;
}

static PyObject* grid_repr(gridObject* /*self*/ ){
  return Py_BuildValue("s", "Grid");
}

static PyGetSetDef grid_getseters[] = {
  {(char*)"color",
   (getter)grid_get_color,
   (setter)grid_set_color,
   (char*)"Grid color (rgba-tuple)", nullptr},

  {(char*)"dashed",
   (getter)grid_get_dashed,
   (setter)grid_set_dashed,
   (char*)"Grid dashes (boolean)", nullptr},

  {(char*)"spacing",
   (getter)grid_get_spacing,
   (setter)grid_set_spacing,
   (char*)"Spacing (px)", nullptr},

  {(char*)"visible",
   (getter)grid_get_visible,
   (setter)grid_set_visible,
   (char*)"Visibility (True/False)", nullptr},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject GridType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Grid", // tp_name
  sizeof(gridObject), //tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)grid_repr, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "Interface for adjusting the guiding/snapping grids in an image.", // tp_doc
  0, // tp_traverse */
  0, // tp_clear */
  0, // tp_richcompare */
  0, // tp_weaklistoffset */
  0, // tp_iter */
  0, // tp_iternext */
  0, // tp_methods */
  0, // tp_members
  grid_getseters, // tp_getset */
  0, // tp_base */
  0, // tp_dict */
  0, // tp_descr_get */
  0, // tp_descr_set */
  0, // tp_dictoffset */
  (initproc)grid_init, // tp_init
  0, // tp_alloc
  grid_new, // tp_new */
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0 // tp_version_tag
};
