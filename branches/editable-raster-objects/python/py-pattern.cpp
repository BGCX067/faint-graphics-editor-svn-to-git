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

#include "bitmap/bitmap.hh"
#include "python/pythoninclude.hh"
#include "python/py-pattern.hh"
#include "python/py-util.hh"
#include "python/py-bitmap.hh"

static int Pattern_compare(patternObject* self, PyObject* otherRaw ){
  if ( !PyObject_IsInstance( otherRaw, (PyObject*)&PatternType) ){
    // Use less for type mismatch
    return faint::cmp_less;
  }
  patternObject* other = (patternObject*)otherRaw;
  const faint::Pattern& lhs(*self->pattern);
  const faint::Pattern& rhs(*other->pattern);
  return faint::py_compare(lhs, rhs);
}

static int Pattern_init(patternObject* self, PyObject* args, PyObject* ){
  bitmapObject* pyBitmap = nullptr;
  IntPoint anchor;
  if ( PySequence_Length(args) == 1 ){
    if ( !PyArg_ParseTuple(args, "O!", &BitmapType, &pyBitmap ) ){
      return faint::init_fail;
    }
  }
  else if ( !PyArg_ParseTuple(args, "(ii)O!", &anchor.x, &anchor.y, &BitmapType, &pyBitmap) ){
    return faint::init_fail;
  }
  self->pattern = new faint::Pattern(*(pyBitmap->bmp), anchor, object_aligned_t(false));
  return faint::init_ok;
}

static PyObject* Pattern_new(PyTypeObject* type, PyObject*, PyObject*){
  patternObject* self = (patternObject*)(type->tp_alloc(type,0));
  self->pattern = nullptr;
  return (PyObject*)self;
}

static void Pattern_dealloc( patternObject* self ){
  delete self->pattern;
  self->pattern = nullptr;
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* Pattern_get_anchor( patternObject* self ){
  return build_intpoint(self->pattern->GetAnchor());
}

static PyObject* Pattern_get_bitmap( patternObject* self ){
  bitmapObject* pyBitmap = (bitmapObject*)BitmapType.tp_alloc(&BitmapType, 0);
  pyBitmap->bmp = new faint::Bitmap(self->pattern->GetBitmap());
  return (PyObject*)pyBitmap;
}

static PyObject* Pattern_get_object_aligned( patternObject* self ){
  return python_bool( self->pattern->GetObjectAligned() );
}

static PyObject* Pattern_set_anchor( patternObject* self, PyObject* args ){
  IntPoint p(0,0);
  if ( !parse_intpoint(args, &p ) ){
    return nullptr;
  }
  self->pattern->SetAnchor(p);
  return Py_BuildValue("");
}

static PyObject* Pattern_set_bitmap( patternObject* self, PyObject* args ){
  bitmapObject* pyBitmap = nullptr;
  if ( !PyArg_ParseTuple(args, "O!", &BitmapType, &pyBitmap ) ){
    return nullptr;
  }
  self->pattern->SetBitmap(*pyBitmap->bmp);
  return Py_BuildValue("");
}

static PyObject* Pattern_set_object_aligned( patternObject* self, PyObject* args ){
  if ( PySequence_Length(args) != 1 ){
    PyErr_SetString(PyExc_ValueError, "Expected True/False");
    return nullptr;
  }
  PyObject* obj = PySequence_GetItem(args, 0);
  bool t = (1 == PyObject_IsTrue(obj));
  faint::py_xdecref(obj);
  self->pattern->SetObjectAligned(t);
  return Py_BuildValue("");
}

static PyMethodDef Pattern_methods[] = {
  {"get_bitmap", (PyCFunction)Pattern_get_bitmap, METH_NOARGS,
   "get_bitmap()->bmp\nReturns a copy of the pattern bitmap"},
  {"get_anchor", (PyCFunction)Pattern_get_anchor, METH_NOARGS,
   "get_anchor()->x,y\nReturns the anchor point"},
  {"get_object_aligned", (PyCFunction)Pattern_get_object_aligned, METH_NOARGS,
   "get_object_aligned()->b\nTrue if the pattern is applied in object-coordinates"},
  {"set_anchor", (PyCFunction)Pattern_set_anchor, METH_VARARGS,
   "set_anchor(x,y)\nSets the anchor point to x,y"},
  {"set_bitmap", (PyCFunction)Pattern_set_bitmap, METH_VARARGS,
   "set_bitmap(bmp)\nSets the pattern bitmap to a copy of the "
   "passed in bitmap"},
  {"set_object_aligned", (PyCFunction)Pattern_set_object_aligned, METH_VARARGS,
   "set_object_aligned(b)\nSets if the pattern should be applied in object-coordinates"},
  {0,0,0,0} // Sentinel
};

PyTypeObject PatternType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Pattern", // tp_name
  sizeof(patternObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)Pattern_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  (cmpfunc)Pattern_compare, // tp_compare
  0, // tp_repr
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
  "Faint Pattern:\n"
  " Pattern(bmp[,anchor])\n  creates a pattern from a Bitmap, anchored around the anchor point", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  Pattern_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)Pattern_init, // tp_init
  0, // tp_alloc
  Pattern_new, // tp_new
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
