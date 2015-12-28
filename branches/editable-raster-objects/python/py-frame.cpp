#include <sstream>
#include "app/getappcontext.hh"
#include "commands/draw-object-cmd.hh"
#include "objects/objline.hh"
#include "python/pyinterface.hh"
#include "python/pyinterface.hh"
#include "python/pythoninclude.hh"
#include "python/py-canvas.hh"
#include "python/py-frame.hh"
#include "python/py-util.hh"
#include "util/canvasinterface.hh"
#include "util/image.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

bool frame_ok( frameObject* self ){
  if ( canvas_ok(self->canvasId) && self->canvas->Has(self->frameId) ){
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "That frame is removed.");
  return false;
}

static const faint::Image& get_image(frameObject* self){
  return self->canvas->GetFrameById(self->frameId);
}

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
  // Fixme: Frames must always be tied to a canvas,
  // Retrieve them from one instead!
  canvasObject* canvas;
  int num;
  if ( !PyArg_ParseTuple(args, "O!i", &CanvasType, &canvas, &num ) ){
    return faint::init_fail;
  }
  if ( !canvas_ok(canvas->id) ){
    PyErr_SetString(PyExc_ValueError, "Canvas is removed.");
    return faint::init_fail;
  }

  index_t frameCount = canvas->canvas->GetNumFrames();
  index_t index = index_t(size_t(num));
  if ( num < 0 || frameCount <= index ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified.");
    return faint::init_fail;
  }

  self->canvas = canvas->canvas;
  self->canvasId = canvas->canvas->GetId();
  self->frameId = self->canvas->GetFrame(index).GetId();
  return faint::init_ok;
}

static PyObject* frame_get_bitmap(frameObject* self, PyObject* ){
  if ( !frame_ok(self) ){
    return nullptr;
  }
  const faint::Image& image(get_image(self));
  return build_bitmap(image.GetBitmap()); // Fixme: Handle memory error
}

static PyObject* frame_line(frameObject* self, PyObject* args ){
  if ( !frame_ok(self) ){
    return nullptr;
  }

  Point p0, p1;
  if ( !parse_point2( args, &p0, &p1) ){
    PyErr_SetString( PyExc_TypeError, "Invalid parameters to line( x0, y0, x1, y1 )" );
    return nullptr;
  }

  Settings s( default_line_settings() );
  s.Update( GetAppContext().GetToolSettings() );
  s.Set( ts_AntiAlias, false );

  Command* cmd = new DrawObjectCommand(its_yours(new ObjLine(Points(vector_of(p0, p1)), s)));

  faint::python_run_command(self->canvas, cmd, self->frameId);
  return Py_BuildValue("");
}

static PyObject* frame_get_objects( frameObject* self ){
  if ( !frame_ok(self) ){
    return nullptr;
  }
  const faint::Image& image = get_image(self);
  const objects_t& objects = image.GetObjects();
  return build_object_list(objects, self->canvas, image.GetId());
}

static PyObject* frame_get_size( frameObject* self ){
  if (!frame_ok(self)){
    return nullptr;
  }
  return build_intsize(get_image(self).GetSize());
}

// Method table for Python-Frame interface (excluding standard
// functions like del, repr and str) Mind the alphabetical
// ordering!
static PyMethodDef frame_methods[] = {
  { "get_bitmap", (PyCFunction)frame_get_bitmap, METH_NOARGS, "get_bitmap()->bmp\nReturns a copy of the pixel data as an ifaint.Bitmap"},
  {"get_objects", (PyCFunction)frame_get_objects, METH_NOARGS, "get_objects() -> [ object1, object2, ... ]\nReturns the objects in frame"},
  {"get_size", (PyCFunction)frame_get_size, METH_NOARGS, "get_size() -> (w,h)\nReturns the size of the frame."},
  {"line", (PyCFunction)frame_line, METH_VARARGS, "line(x0,y0,x1,y1)\nDraw a line from x0,y0 to x1,y1"},
  {0,0,0,0}
};

PyTypeObject FrameType = {
  PyObject_HEAD_INIT(nullptr)
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
