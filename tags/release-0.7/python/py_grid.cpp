#include "app/getappcontext.hh"
#include "geo/grid.hh"
#include "python/pythoninclude.hh"
#include "py_grid.hh"
#include "util/canvasinterface.hh"

static PyObject* grid_new( PyTypeObject* type, PyObject*, PyObject* ){
  gridObject* self = (gridObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static int grid_init(gridObject* self, PyObject* /*args*/, PyObject*){
  self->canvas = 0;
  self->canvasId = CanvasId();
  return 0;
}

PyObject* py_grid_canvas( CanvasInterface* canvas ){
  gridObject* py_grid = (gridObject*) GridType.tp_alloc(&GridType,0);
  py_grid->canvas = canvas;
  if ( canvas != 0 ){
    py_grid->canvasId = canvas->GetId();
  }
  return (PyObject*) py_grid;
}

PyObject* grid_get_spacing( gridObject* self, void* ){
  if ( self->canvas != 0 && !GetAppContext().Exists( self->canvasId ) ){
    PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
    return 0;
  }
  CanvasInterface& canvas ( self->canvas == 0 ? GetAppContext().GetActiveCanvas() : *self->canvas );
  return Py_BuildValue( "i", canvas.GetGrid().Spacing() );
}

int grid_set_spacing( gridObject* self, PyObject* args, void* ){
  if ( self->canvas != 0 && !GetAppContext().Exists( self->canvasId ) ){
    PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
    return -1;
  }
  int spacing = PyLong_AsLong(args); // Fixme: Error checking
  CanvasInterface& canvas ( self->canvas == 0 ? GetAppContext().GetActiveCanvas() : *self->canvas );
  Grid g = canvas.GetGrid();
  g.SetSpacing(spacing);
  canvas.SetGrid(g);
  return 0;
}

PyObject* grid_get_dashed( gridObject* self, void* ){
  if ( self->canvas != 0 && !GetAppContext().Exists( self->canvasId ) ){
    PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
    return 0;
  }
  CanvasInterface& canvas ( self->canvas == 0 ? GetAppContext().GetActiveCanvas() : *self->canvas );
  bool dashed = canvas.GetGrid().Dashed();
  return dashed ? Py_True : Py_False;
}

int grid_set_dashed( gridObject* self, PyObject* args, void* ){
  if ( self->canvas != 0 && !GetAppContext().Exists( self->canvasId ) ){
    PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
    return -1;
  }
  CanvasInterface& canvas ( self->canvas == 0 ? GetAppContext().GetActiveCanvas() : *self->canvas );
  bool dashed = args == Py_True;
  Grid g = canvas.GetGrid();
  g.SetDashed(dashed);
  canvas.SetGrid(g);
  return 0;
}

static PyObject* grid_repr(gridObject* /*self*/ ){
  return Py_BuildValue("s", "Grid");
}

static PyGetSetDef grid_getseters[] = {
  {(char*)"spacing",
   (getter)grid_get_spacing,
   (setter)grid_set_spacing,
   (char*)"Spacing (px)", NULL},
  {(char*)"dashed",
   (getter)grid_get_dashed,
   (setter)grid_set_dashed,
   (char*)"Grid dashes (boolean)", NULL},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject GridType = {
  PyObject_HEAD_INIT(NULL)
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
