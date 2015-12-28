#include "bitmap/bitmap.hh"
#include "python/pythoninclude.hh"
#include "python/py_bitmap.hh"
#include "python/py_util.hh"
#include "rendering/cairocontext.hh"

static int Bitmap_init(bitmapObject* self, PyObject* args, PyObject*){
  int w, h;
  if ( !PyArg_ParseTuple(args, "ii", &w, &h) ){
    return -1;
  }
  if ( w <= 0 || h <= 0 ){
    PyErr_SetString(PyExc_ValueError, "Bitmap width and height must be greater than 0");
    return -1;
  }
  try {
    self->bmp = new faint::Bitmap(faint::cairo_compatible_bitmap(IntSize(w,h)));
    faint::clear(*self->bmp, faint::Color(255,255,255));
  }
  catch ( const std::bad_alloc& ){
    PyErr_SetString(PyExc_MemoryError, "Failed allocating memory for Bitmap");
    return -1;
  }
  return 0;
}

static PyObject* Bitmap_new(PyTypeObject* type, PyObject*, PyObject* ){
  bitmapObject* self;
  self = (bitmapObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static PyObject* Bitmap_repr( bitmapObject* /*self*/ ){
  return Py_BuildValue("s", "Bitmap");
}

static PyObject* Bitmap_clear(bitmapObject* self, PyObject* args){
  faint::Color color;
  if ( !parse_color(args, color) ){
    return 0;
  }
  clear(*(self->bmp), color);
  return Py_BuildValue("");
}

static PyObject* Bitmap_set_pixel(bitmapObject* self, PyObject* args){
  int x, y, r, g, b, a;
  if ( !PyArg_ParseTuple( args, "ii(iiii)", &x, &y, &r, &g, &b, &a ) ){
    PyErr_Clear();
    a = 255;
    if ( !PyArg_ParseTuple( args, "ii(iii)", &x, &y, &r, &g, &b ) ){
      return 0;
    }
  }
  if ( invalid_color(r,g,b,a) ){
    return 0;
  }

  faint::Bitmap& bmp(*self->bmp);
  if ( invalid_pixel_pos(x, y, bmp) ){
    return 0;
  }

  // Fixme: Need a command. Not one per pixel though...
  faint::put_pixel_raw( bmp, x,y, faint::color_from_ints( r, g, b, a ) );
  return Py_BuildValue("");
}

static void Bitmap_dealloc( bitmapObject* self ){
  delete self->bmp;
  self->bmp = 0;
}

static PyMethodDef bitmap_methods[] = {
  {"set_pixel", (PyCFunction)Bitmap_set_pixel, METH_VARARGS,
   "set_pixel(x,y,(r,g,b[,a]))\nSet the pixel at (x,y) to the specified color."},
  {"clear", (PyCFunction)Bitmap_clear, METH_VARARGS,
   "clear(r,g,b[,a])\nClear the bitmap with the specified color."},
  {0,0,0,0} // Sentinel
};

PyTypeObject BitmapType = {
  PyObject_HEAD_INIT(NULL)
  0, // ob_size
  "Bitmap", // tp_name
  sizeof(bitmapObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)Bitmap_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)Bitmap_repr, // tp_repr
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
  // tp_doc

  "Allows in-memory bitmap editing.\n"
  "To show a Bitmap, blit it onto an opened image (see Canvas.blit).\n\n"
  "A Bitmap can be used for the loading step of custom raster file formats (see\n"
  "add_format and ImageProps).",

  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  bitmap_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)Bitmap_init, // tp_init
  0, // tp_alloc
  Bitmap_new, // tp_new
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
