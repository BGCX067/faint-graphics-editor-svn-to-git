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

#include "bitmap/bitmap.hh"
#include "bitmap/pattern.hh"
#include "python/mapped-type.hh"
#include "python/py-include.hh"
#include "python/py-bitmap.hh"
#include "python/py-pattern.hh"
#include "python/py-ugly-forward.hh"

namespace faint{

template<>
struct MappedType<Pattern&>{
  using PYTHON_TYPE = patternObject;

  static Pattern& GetCppObject(patternObject* self){
    return *self->pattern;
  }

  static bool Expired(patternObject*){
    return false;
  }

  static void ShowError(patternObject*){
  }
};

PyObject* Pattern_richcompare(patternObject* self, PyObject* otherRaw, int op){
  if (!PyObject_IsInstance(otherRaw, (PyObject*)&PatternType)){
    Py_RETURN_NOTIMPLEMENTED;
  }
  patternObject* other((patternObject*)otherRaw);
  const Pattern& lhs(*self->pattern);
  const Pattern& rhs(*other->pattern);
  return py_rich_compare(lhs, rhs, op);
}

static void Pattern_init(patternObject& self,
  const Bitmap& bmp,
  const Optional<IntPoint>& anchor)
{
  self.pattern = new Pattern(bmp, anchor.Or({0,0}), object_aligned_t(false));
}

static PyObject* Pattern_new(PyTypeObject* type, PyObject*, PyObject*){
  patternObject* self = (patternObject*)(type->tp_alloc(type,0));
  self->pattern = nullptr;
  return (PyObject*)self;
}

static void Pattern_dealloc(patternObject* self){
  delete self->pattern;
  self->pattern = nullptr;
  self->ob_base.ob_type->tp_free((PyObject*)self);
}

/* method: "get_anchor()->x,y\n
Returns the anchor point of the pattern." */
static IntPoint Pattern_get_anchor(Pattern& self){
  return self.GetAnchor();
}

/* method: "get_bitmap()->bmp\n
Returns a copy of the pattern bitmap." */
static Bitmap Pattern_get_bitmap(Pattern& self){
  return self.GetBitmap();
}

/* method: "get_object_aligned()->b\n
True if pattern is applied in object-coordinates" */
static bool Pattern_get_object_aligned(Pattern& self){
  return self.GetObjectAligned();
}

/* method: "set_anchor(x,y)\n
Sets the anchor point to x,y." */
static void Pattern_set_anchor(Pattern& self, const IntPoint& p){
  self.SetAnchor(p);
}
/* method: "set_bitmap(bmp)\n
Sets the pattern bitmap to a copy of the specified bitmap." */
static void Pattern_set_bitmap(Pattern& self, const Bitmap& bmp){
  self.SetBitmap(bmp);
}

/* method: "set_object_aligned(b)\n
Sets whether the pattern should be applied in object-coordinates." */
static void Pattern_set_object_aligned(Pattern& self, const bool& aligned){
  self.SetObjectAligned(aligned);
}

#include "generated/python/method-def/py-pattern-methoddef.hh"

PyTypeObject PatternType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "Pattern", // tp_name
  sizeof(patternObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)Pattern_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // reserved (formerly tp_compare)
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
  (richcmpfunc)Pattern_richcompare, // tp_richcompare
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
  INIT_FORWARDER(Pattern_init), // tp_init
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
  0, // tp_version_tag
  0  // tp_finalize
};

} // namespace
