#include "app/getappcontext.hh"
#include "geo/grid.hh"
#include "python/pythoninclude.hh"
#include "python/py_app.hh"
#include "python/py_util.hh"

// Method table for the Python interface class for the Faint app
static PyMethodDef faintapp_methods[] = {
  {0, 0, 0, 0}  // Sentinel
};

static PyObject* faintapp_get_grid_color( faintAppObject* /*self*/, PyObject*, void* ){
  return build_color_tuple(GetAppContext().GetDefaultGrid().Color());
}

static PyObject* faintapp_get_grid_dashed( faintAppObject* /*self*/, PyObject*, void* ){
  if ( GetAppContext().GetDefaultGrid().Dashed() ){
    return Py_True;
  }
  return Py_False;
}

static int faintapp_init(faintAppObject*, PyObject*, PyObject*) {
  return 0;
}

static PyObject* faintapp_new(PyTypeObject* type, PyObject*, PyObject*){
  faintAppObject* self = ( faintAppObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintapp_repr(faintAppObject*){
  return Py_BuildValue("s", "FaintApp");
}

int faintapp_set_grid_color( faintAppObject* /*self*/, PyObject* args, void* ){
  faint::Color gridColor;
  if ( !parse_color(args, gridColor) ){
    return -1;
  }
  AppContext& app = GetAppContext();
  Grid grid(app.GetDefaultGrid());
  grid.SetColor(gridColor);
  app.SetDefaultGrid(grid);
  return 0;
}

int faintapp_set_grid_dashed( faintAppObject* /*self*/, PyObject* args, void* ){
  AppContext& app = GetAppContext();
  Grid grid(app.GetDefaultGrid());
  if ( PyObject_IsTrue(args) ){
    grid.SetDashed(true);
  }
  else {
    grid.SetDashed(false);
  }
  app.SetDefaultGrid(grid);
  return 0;
}

static PyGetSetDef faintapp_getseters[] = {
  {(char*)"gridcolor",
   GETTER(faintapp_get_grid_color),
   (setter)faintapp_set_grid_color,
   (char*)"Color of grids", NULL},
  {(char*)"griddashed",
   GETTER(faintapp_get_grid_dashed),
   (setter)faintapp_set_grid_dashed,
   (char*)"Whether grids are dashed", NULL},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject FaintAppType = {
    PyObject_HEAD_INIT(NULL)
    0, // ob_size
    "FaintApp", //tp_name
    sizeof(faintAppObject), // tp_basicsize
    0, // tp_itemsize
    0, // tp_dealloc
    0, // tp_print
    0, // tp_getattr
    0, // tp_setattr
    0, // tp_compare
    (reprfunc)faintapp_repr, // tp_repr
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
    "The Faint Application Window", // tp_doc
    0, // tp_traverse
    0, // tp_clear
    0, // tp_richcompare
    0, // tp_weaklistoffset
    0, // tp_iter
    0, // tp_iternext
    faintapp_methods, // tp_methods
    0, // tp_members
    faintapp_getseters, // tp_getset
    0, // tp_base
    0, // tp_dict
    0, // tp_descr_get
    0, // tp_descr_set
    0, // tp_dictoffset
    (initproc)faintapp_init, // tp_init
    0, // tp_alloc
    faintapp_new, // tp_new
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
