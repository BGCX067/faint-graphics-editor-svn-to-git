// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include <iomanip>
#include <sstream>
#include "pythoninclude.hh"
#include "python/py-tri.hh"
#include "python/py-util.hh"
#include "geo/tri.hh"

static PyObject* tri_p0( triObject* self ){
  return build_point(self->tri.P0());
}

static PyObject* tri_p1( triObject* self ){
  return build_point(self->tri.P1());
}

static PyObject* tri_p2( triObject* self ){
  return build_point(self->tri.P2());
}

static PyObject* tri_p3( triObject* self ){
  return build_point(self->tri.P3());
}

static PyObject* tri_angle( triObject* self ){
  return build_radian(self->tri.Angle());
}

static PyObject* tri_width( triObject* self ){
  return build_coord(self->tri.Width());
}

static PyObject* tri_height( triObject* self ){
  return build_coord(self->tri.Height());
}

static PyObject* tri_get_skew( triObject* self ){
  faint::coord skew = self->tri.Skew();
  return build_coord(skew);
}

static PyObject* tri_set_skew( triObject* self, PyObject* args ){
  faint::coord skew = 0;
  if ( !parse_coord( args, &skew ) ){
    return nullptr;
  }
  self->tri = skewed( self->tri, skew );
  return Py_BuildValue("");
}

static PyObject* tri_rotate( triObject* self, PyObject* args ){
  faint::coord x = 0;
  faint::coord y = 0;
  faint::radian angle = 0;

  if ( PyArg_ParseTuple( args, "ddd", &angle, &x, &y ) ){
    self->tri = rotated( self->tri, angle, Point(x,y) );
    return Py_BuildValue("");
  }

  PyErr_Clear();
  if ( PyArg_ParseTuple( args, "d", &angle ) ){
    self->tri = rotated( self->tri, angle, self->tri.P3() );
    return Py_BuildValue("");
  }

  PyErr_SetString( PyExc_ValueError, "Invalid angle" );
  return nullptr;
}

static PyObject* tri_translate( triObject* self, PyObject* args ){
  Point delta;
  if ( !parse_point( args, &delta ) ){
    return nullptr;
  }
  self->tri = translated( self->tri, delta.x, delta.y );
  return Py_BuildValue("");
}

static PyObject* tri_center( triObject* self, PyObject* ){
  return build_point(center_point(self->tri));
}

static PyObject* tri_offset_aligned( triObject* self, PyObject* args ){
  Point delta;
  if ( !parse_point( args, &delta ) ){
    return nullptr;
  }
  self->tri = offset_aligned( self->tri, delta.x, delta.y );
  return Py_BuildValue("");
}

// Method table for Python-Tri interface
static PyMethodDef tri_methods[] = {
  {"p0", (PyCFunction)tri_p0, METH_NOARGS, ""},
  {"p1", (PyCFunction)tri_p1, METH_NOARGS, ""},
  {"p2", (PyCFunction)tri_p2, METH_NOARGS, ""},
  {"p3", (PyCFunction)tri_p3, METH_NOARGS, ""},
  {"angle", (PyCFunction)tri_angle, METH_NOARGS, ""},
  {"get_skew", (PyCFunction)tri_get_skew, METH_VARARGS, ""},
  {"set_skew", (PyCFunction)tri_set_skew, METH_VARARGS, ""},
  {"translate", (PyCFunction)tri_translate, METH_VARARGS, ""},
  {"offset_aligned", (PyCFunction)tri_offset_aligned, METH_VARARGS, "offset_aligned( dx, dy )\nOffset the tri by dx, dy aligned with its axes."},
  {"rotate", (PyCFunction)tri_rotate, METH_VARARGS, "rotate( radians, x, y )\nRotate the tri radians around (x,y)"},
  {"width", (PyCFunction)tri_width, METH_NOARGS, ""},
  {"height", (PyCFunction)tri_height, METH_NOARGS, ""},
  {"center", (PyCFunction)tri_center, METH_NOARGS, "Returns the center point of the tri, adjusted for rotation" },
  {0,0,0,0} // Sentinel
};

static PyObject* tri_new(PyTypeObject *type, PyObject*, PyObject* )
{
  triObject* self;
  self = (triObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static int tri_init(triObject* self, PyObject* args, PyObject*) {
  faint::coord x0 = 0;
  faint::coord y0 = 0;
  faint::coord x1 = 0;
  faint::coord y1 = 0;
  faint::coord x2 = 0;
  faint::coord y2 = 0;
  if ( !PyArg_ParseTuple(args, "(dd)(dd)(dd)", &x0, &y0, &x1, &y1, &x2, &y2 ) ){
    return faint::init_fail;
  }
  self->tri = Tri(Point(x0,y0),Point(x1,y1),Point(x2,y2));
  return faint::init_ok;
}

static PyObject* tri_repr( triObject* self ){
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  const Tri& tri( self->tri );
  Point p0 = tri.P0();

  ss << "Tri(" << p0.x << "," << p0.y << "," << tri.Width() << ", " << tri.Height() << ")";
  return Py_BuildValue("s", ss.str().c_str());
}

PyTypeObject TriType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Tri", // tp_name
  sizeof(triObject), //tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)tri_repr, // tp_repr
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
  "A Tri defines the geometry of an object", // tp_doc
  0, // tp_traverse */
  0, // tp_clear */
  0, // tp_richcompare */
  0, // tp_weaklistoffset */
  0, // tp_iter */
  0, // tp_iternext */
  tri_methods, // tp_methods */
  0, // tp_members
  0, // tp_getset */
  0, // tp_base */
  0, // tp_dict */
  0, // tp_descr_get */
  0, // tp_descr_set */
  0, // tp_dictoffset */
  (initproc)tri_init, // tp_init
  0, // tp_alloc
  tri_new, // tp_new */
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

PyObject* pythoned( const Tri& tri ){
  triObject* py_tri = ( triObject* ) TriType.tp_alloc( &TriType, 0 );
  py_tri->tri = tri;
  return (PyObject*)py_tri;
}
