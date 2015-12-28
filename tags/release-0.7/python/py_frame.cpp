#include <sstream>
#include "python/pythoninclude.hh"
#include "python/py_canvas.hh"
#include "python/py_frame.hh"
#include "python/py_util.hh"
#include "util/canvasinterface.hh"
#include "util/settingutil.hh"
#include "util/image.hh"
#include "app/getappcontext.hh"

static PyObject* frame_new(PyTypeObject* type, PyObject*, PyObject*){
  frameObject* self;
  self = (frameObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static PyObject* frame_repr(frameObject* self){
  std::stringstream ss;
  if ( canvas_ok( self->canvasId ) ){
    ss << "Frame of canvas #" << self->canvasId.Raw();
  }
  else {
    ss << "Frame of retired Canvas #" << self->canvasId.Raw();
  }
  return Py_BuildValue("s", ss.str().c_str());
}

static int frame_init(frameObject* self, PyObject* args, PyObject* ){
  canvasObject* canvas;
  int num;
  if ( !PyArg_ParseTuple(args, "O!i", &CanvasType, &canvas, &num ) ){
    return -1;
  }

  if ( !canvas_ok(canvas->id) ){
    PyErr_SetString(PyExc_ValueError, "Canvas is removed.");
    return -1;
  }

  const faint::ImageList& images( canvas->canvas->GetFrames() );
  if ( num < 0 || images.GetNumImages().Get() <= static_cast<size_t>(num) ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified.");
    return -1;
  }

  self->canvas = canvas->canvas;
  self->canvasId = canvas->canvas->GetId();
  self->frame = &(images.GetImage(index_t(size_t(num))));
  return 0;
}

static PyObject* frame_line(frameObject* self, PyObject* args ){
  if ( !canvas_ok( self->canvasId ) ){
    PyErr_SetString(PyExc_ValueError, "Canvas is removed.");
    return 0;
  }

  Point p0, p1;
  if ( !parse_point2( args, &p0, &p1) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to line( x0, y0, x1, y1 )" );
    return 0;
  }

  Settings s( default_line_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );

  //Command* cmd = new DrawObjectCommand(its_yours(new ObjLine(Points(vector_of(p0, p1)), s)));
  //faint::python_run_command(self->canvas, cmd);
  return Py_BuildValue("");
}

// Method table for Python-Frame interface (excluding standard
// functions like del, repr and str) Mind the alphabetical
// ordering!
static PyMethodDef frame_methods[] = {
  {"line", (PyCFunction)frame_line, METH_VARARGS, "line(x0,y0,x1,y1)\nDraw a line from x0,y0 to x1,y1"},
  {0,0,0,0}
};

PyTypeObject FrameType = {
  PyObject_HEAD_INIT(NULL)
  0, // ob_size
  "Frame", //tp_name
  sizeof(frameObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0,  // tp_compare
  (reprfunc)frame_repr, // tp_repr
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
  "An frame in a Faint image.", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  frame_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)frame_init, // tp_init
  0, // tp_alloc
  frame_new, // tp_new
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
