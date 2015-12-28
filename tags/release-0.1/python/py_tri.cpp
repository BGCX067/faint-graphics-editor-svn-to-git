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

#include "pythoninclude.hh"
#include "py_tri.hh"
#include "py_util.hh"
#include "geo/tri.hh"
#include <sstream>
#include <iomanip>

// Fixme: Use py_util functions to build return values
static PyObject* tri_p0( triObject* self ){
  Point pt = self->tri.P0();
  return Py_BuildValue( "dd", pt.x, pt.y );
}

static PyObject* tri_p1( triObject* self ){
  Point pt = self->tri.P1();
  return Py_BuildValue( "dd", pt.x, pt.y );
}

static PyObject* tri_p2( triObject* self ){
  Point pt = self->tri.P2();
  return Py_BuildValue( "dd", pt.x, pt.y );
}

static PyObject* tri_p3( triObject* self ){
  Point pt = self->tri.P3();
  return Py_BuildValue( "dd", pt.x, pt.y );
}

static PyObject* tri_angle( triObject* self ){
  faint::radian angle = self->tri.Angle();
  return Py_BuildValue( "d", angle );
}

static PyObject* tri_width( triObject* self ){
  faint::coord w = self->tri.Width();
  return Py_BuildValue( "d", w );
}

static PyObject* tri_height( triObject* self ){
  faint::coord h = self->tri.Height();
  return Py_BuildValue( "d", h );
}

static PyObject* tri_get_skew( triObject* self ){
  faint::coord skew = self->tri.Skew();
  // Fixme: Check if close to zero, and if so round to 0?
  // (e.g.: -2.1316282072803006e-14)
  return Py_BuildValue( "d", skew );
}

static PyObject* tri_set_skew( triObject* self, PyObject* args ){
  faint::coord skew = 0;
  if ( !ParseCoord( args, &skew ) ){
    return 0;
  }
  self->tri = Skewed( self->tri, skew );
  return Py_BuildValue("");
}

static PyObject* tri_rotate( triObject* self, PyObject* args ){
  faint::coord x = 0;
  faint::coord y = 0;
  faint::radian angle = 0;

  if ( PyArg_ParseTuple( args, "ddd", &angle, &x, &y ) ){
    self->tri = Rotated( self->tri, angle, Point(x,y) );
    return Py_BuildValue("");
  }

  PyErr_Clear();
  if ( PyArg_ParseTuple( args, "d", &angle ) ){
    self->tri = Rotated( self->tri, angle, self->tri.P3() );
    return Py_BuildValue("");
  }

  PyErr_SetString( PyExc_ValueError, "Invalid angle" );
  return 0;
}

static PyObject* tri_translate( triObject* self, PyObject* args ){
  Point delta;
  if ( !ParsePoint( args, &delta ) ){
    return 0;
  }
  self->tri = Translated( self->tri, delta.x, delta.y );
  return Py_BuildValue("");
}

static PyObject* tri_center( triObject* self, PyObject* ){
  Point center( CenterPoint(self->tri) );
  return Py_BuildValue("dd", center.x, center.y); // Fixme: Use helper (Point-two-2 float)
}

static PyObject* tri_offset_aligned( triObject* self, PyObject* args ){
  Point delta;  
  if ( !ParsePoint( args, &delta ) ){
    return 0;
  }
  self->tri = OffsetAligned( self->tri, delta.x, delta.y );
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
    return -1;
  }
  self->tri = Tri(Point(x0,y0),Point(x1,y1),Point(x2,y2));
  return 0;
}

static PyObject* tri_repr( triObject* self ){
  std::stringstream ss;
  // Fixme: This will print unnecessary decimals, but is better than ridiculous precision
  ss << std::setprecision(2) << std::fixed;
  const Tri& tri( self->tri );
  Point p0 = tri.P0();

  ss << "Tri(" << p0.x << "," << p0.y << "," << tri.Width() << ", " << tri.Height() << ")";
  return Py_BuildValue("s", ss.str().c_str());
}

PyTypeObject TriType = {
  PyObject_HEAD_INIT(NULL)
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

PyObject* Pythoned( const Tri& tri ){
  triObject* py_tri = ( triObject* ) TriType.tp_alloc( &TriType, 0 );
  py_tri->tri = tri;
  return (PyObject*)py_tri;
}
