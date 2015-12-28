// -*- coding: us-ascii-unix -*-
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
#include "geo/tri.hh"
#include "python/py-include.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"

namespace faint{

Tri& get_cpp_object(triObject* self){
  return self->tri;
}

bool faint_side_ok(triObject*){
  // The triObject contains the Tri - it is not a reference
  // that might be broken.
  return true;
}

void show_error(triObject*){
  // See faint_side_ok.
}

static Point tri_p0(Tri& self){
  return self.P0();
}

static Point tri_p1(Tri& self){
  return self.P1();
}

static Point tri_p2(Tri& self){
  return self.P2();
}

static Point tri_p3(Tri& self){
  return self.P3();
}

static coord tri_width(Tri& self){
  return self.Width();
}

static coord tri_height(Tri& self){
  return self.Height();
}

static void tri_rotate(Tri& self, const radian& angle, const Optional<Point>& pivot){
  self = pivot.IsSet() ?
    rotated(self, Angle::Rad(angle), pivot.Get()) :
    rotated(self, Angle::Rad(angle), self.P3());
}

static void tri_translate(Tri& self, const Point& delta){
  self = translated(self, delta.x, delta.y);
}

static Point tri_center(Tri& self){
  return center_point(self);
}

static void tri_offset_aligned(Tri& self, const Point& delta){
  self = offset_aligned(self, delta.x, delta.y);
}

// Method table for Python-Tri interface
static PyMethodDef tri_methods[] = {
  {"p0", FORWARDER(tri_p0), METH_NOARGS, "p0()->(x,y)\nReturns a triangle vertex."},
  {"p1", FORWARDER(tri_p1), METH_NOARGS, "p1()->(x,y)\nReturns a triangle vertex."},
  {"p2", FORWARDER(tri_p2), METH_NOARGS, "p2()->(x,y)\nReturns a triangle vertex."},
  {"p3", FORWARDER(tri_p3), METH_NOARGS, "p3()->(x,y)\nReturns p0() mirrored over p1() and p2()"},
  {"translate", FORWARDER(tri_translate), METH_VARARGS, "translate(dx,dy)\nOffsets the Tri by dx,dy aligned with the image"},
  {"offset_aligned", FORWARDER(tri_offset_aligned), METH_VARARGS, "offset_aligned(dx, dy)\nOffset the tri by dx, dy aligned with its axes (dx is along p0p1 and dy along p0p2)"},
  {"rotate", FORWARDER(tri_rotate), METH_VARARGS, "rotate(radians, x, y)\nRotate the tri the given radians around (x,y)"},
  {"width", FORWARDER(tri_width), METH_NOARGS, "Returns the width of the Tri, specified as the distance from p0 to p1"},
  {"height", FORWARDER(tri_height), METH_NOARGS, "Returns the height of the Tri, specified as the distance from p0 to p2"},
  {"center", FORWARDER(tri_center), METH_NOARGS, "Returns the center point of the tri, adjusted for rotation (the intersection of p0p3 and p1p2)." },
  {0,0,0,0} // Sentinel
};

static PyObject* tri_new(PyTypeObject *type, PyObject*, PyObject*)
{
  triObject* self;
  self = (triObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static int tri_init(triObject* self, PyObject* args, PyObject*) {
  coord x0 = 0;
  coord y0 = 0;
  coord x1 = 0;
  coord y1 = 0;
  coord x2 = 0;
  coord y2 = 0;
  if (!PyArg_ParseTuple(args, "(dd)(dd)(dd)", &x0, &y0, &x1, &y1, &x2, &y2)){
    return init_fail;
  }
  self->tri = Tri(Point(x0,y0),Point(x1,y1),Point(x2,y2));
  return init_ok;
}

static PyObject* tri_repr(triObject* self){
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  const Tri& tri(self->tri);
  Point p0 = tri.P0();

  ss << "Tri(" << p0.x << "," << p0.y << "," << tri.Width() << ", " << tri.Height() << ")";
  return Py_BuildValue("s", ss.str().c_str());
}

static radian tri_getter_angle(Tri& self){
  return self.GetAngle().Rad();
}

static void tri_setter_angle(Tri& self, const radian& angle){
  self = rotated(rotated(self, -self.GetAngle(), self.P0()), Angle::Rad(angle), self.P0());
}

static coord tri_getter_skew(Tri& self){
  return self.Skew();
}

static void tri_setter_skew(Tri& self, const coord& skew){
  self = skewed(self, skew);
}

static PyGetSetDef tri_getseters[] = {
  {(char*)"skew",
   GET_FORWARDER(tri_getter_skew),
   SET_FORWARDER(tri_setter_skew),
   (char*)"Tri skew\nSpecifies the skewness of the Tri (the distance between p0 and a line perpendicular to p0p1 through p2)", nullptr},

  {(char*)"angle",
   GET_FORWARDER(tri_getter_angle),
   SET_FORWARDER(tri_setter_angle),
   (char*)"angle\nThe angle between p0()->p1() and the horizon expressed in radians.", nullptr},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject TriType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
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
  tri_getseters, // tp_getset */
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
  0, // tp_version_tag
  0  // tp_finalize
};

PyObject* pythoned(const Tri& tri){
  triObject* py_tri = (triObject*) TriType.tp_alloc(&TriType, 0);
  py_tri->tri = tri;
  return (PyObject*)py_tri;
}

}
