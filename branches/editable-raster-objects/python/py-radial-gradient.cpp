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

#include "python/pythoninclude.hh"
#include "python/py-radial-gradient.hh"
#include "python/py-util.hh"

static int RadialGradient_compare(radialGradientObject* self, PyObject* otherRaw ){
  if ( !PyObject_IsInstance( otherRaw, (PyObject*)&RadialGradientType) ){
    // Use less for type mismatch
    return faint::cmp_less;
  }
  radialGradientObject* other = (radialGradientObject*)otherRaw;
  const faint::RadialGradient& lhs(*self->gradient);
  const faint::RadialGradient& rhs(*other->gradient);
  return faint::py_compare(lhs, rhs);
}

static int RadialGradient_init(radialGradientObject* self, PyObject* args, PyObject* ){
  int numArgs = PySequence_Length(args);
  if ( numArgs == 0 ){
    PyErr_SetString(PyExc_ValueError, "Atleast one color stop required");
    return faint::init_fail;
  }

  // Possibly unwrapped sequence
  PyObject* unwrapped = nullptr;

  // The center point is an optional argument (although the first!)
  Point center(0,0);

  // If the first argument is an point, the color stop parsing should be offset
  int firstColorStop = 0;

  if ( numArgs == 1 ){
    // Check if this is a single color stop
    PyObject* pyStop = PySequence_GetItem(args, 0);
    faint::ColorStop stop;
    if ( parse_color_stop(pyStop, stop) ){
      faint::py_xdecref(pyStop);
      std::vector<faint::ColorStop> v;
      v.push_back(stop);
      self->gradient = new faint::RadialGradient(Point(0,0), Radii(1.0,1.0), v);
      return faint::init_ok;
    }
    else if ( !PySequence_Check(pyStop) ){
      PyErr_SetString(PyExc_ValueError, "Atleast one color stop required");
      faint::py_xdecref(pyStop);
      return faint::init_fail;
    }
    numArgs = PySequence_Length(pyStop);
    if ( numArgs == 0 ){
      PyErr_SetString(PyExc_ValueError, "Atleast one color stop required");
      return faint::init_fail;
    }
    args = pyStop;
    unwrapped = pyStop;
  }
  else if ( numArgs > 1 ){
    // Try to parse the first argument as a center point
    faint::ScopedRef firstArg(PySequence_GetItem(args,0));
    if ( PySequence_Check(*firstArg) && PySequence_Length(*firstArg) == 2 ){
      if ( point_from_sequence_noerr(*firstArg, center)){
	firstColorStop += 1;
      }
    }
  }

  std::vector<faint::ColorStop> v;
  for ( int i = firstColorStop; i != numArgs; i++ ){
    PyObject* pyStop = PySequence_GetItem(args, i);
    faint::ColorStop stop;
    bool ok = parse_color_stop(pyStop, stop);
    faint::py_xdecref(pyStop);
    if ( !ok ){
      faint::py_xdecref(unwrapped);
      return faint::init_fail;
    }
    v.push_back(stop);
  }

  self->gradient = new faint::RadialGradient(center, Radii(1.0,1.0), v);
  faint::py_xdecref(unwrapped);
  return faint::init_ok;
}

static PyObject* RadialGradient_new(PyTypeObject* type, PyObject*, PyObject* ){
  radialGradientObject* self = (radialGradientObject*)type->tp_alloc(type, 0);
  self->gradient = nullptr;
  return (PyObject*)self;
}

static PyObject* RadialGradient_repr( radialGradientObject* self ){
  return Py_BuildValue("s", get_repr(*self->gradient).c_str());
}

static void RadialGradient_dealloc( radialGradientObject* self ){
  delete self->gradient;
  self->gradient = nullptr;
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* RadialGradient_add_stop( radialGradientObject* self, PyObject* args ){
  double offset;
  int r,g,b;
  int a = 255;
  if ( !PyArg_ParseTuple(args, "d(iii)", &offset, &r, &g, &b) ){
    PyErr_Clear();
    if ( !PyArg_ParseTuple(args, "d(iiii)", &offset, &r, &g, &b, &a) ){
      return nullptr;
    }
  }

  if ( offset < 0.0 || 1.0 < offset ){
    PyErr_SetString(PyExc_ValueError, "Offset must be in range 0.0-1.0");
    return nullptr;
  }
  if ( invalid_color(r,g,b,a) ){
    return nullptr;
  }

  self->gradient->Add(faint::ColorStop(faint::color_from_ints(r,g,b,a), offset));
  return Py_BuildValue("");
}

static PyObject* RadialGradient_get_center( radialGradientObject* self ){
  return build_point( self->gradient->GetCenter() );
}

static PyObject* RadialGradient_get_num_stops( radialGradientObject* self ){
  return Py_BuildValue("i", self->gradient->GetNumStops());
}

static PyObject* RadialGradient_get_object_aligned( radialGradientObject* self ){
  return python_bool( self->gradient->GetObjectAligned());
}

static PyObject* RadialGradient_get_radii( radialGradientObject* self ){
  return build_radii( self->gradient->GetRadii() );
}

static PyObject* RadialGradient_get_stop( radialGradientObject* self, PyObject* args ){
  int index;
  if ( !PyArg_ParseTuple(args, "i", &index) ){
    return nullptr;
  }
  if ( index < 0 || self->gradient->GetNumStops() <= index ){
    PyErr_SetString(PyExc_IndexError, "Invalid color-stop index");
    return nullptr;
  }

  return build_color_stop( self->gradient->GetStop(index) );
}

static PyObject* RadialGradient_get_stops( radialGradientObject* self ){
  return build_color_stops(*self->gradient);
}

static PyObject* RadialGradient_set_center( radialGradientObject* self, PyObject* args ){
  Point center;
  if ( !parse_point(args, &center ) ){
    return nullptr;
  }
  self->gradient->SetCenter(center);
  return Py_BuildValue("");
}


static PyObject* RadialGradient_set_object_aligned( radialGradientObject* self, PyObject* args ){
  bool value;
  if ( !parse_bool(args, value) ){
    return nullptr;
  }
  self->gradient->SetObjectAligned(value);
  return Py_BuildValue("");
}

static PyObject* RadialGradient_set_radii( radialGradientObject* self, PyObject* args ){
  Radii radii;
  if ( !parse_radii(args, radii) ){
    return nullptr;
  }
  self->gradient->SetRadii(radii);
  return Py_BuildValue("");
}

static PyMethodDef RadialGradient_methods[] = {
  {"add_stop", (PyCFunction)RadialGradient_add_stop, METH_VARARGS,
   "add_stop(offset,(r,g,b[,a]))\nAdds the specified color stop to the gradient"},
  {"get_center", (PyCFunction)RadialGradient_get_center, METH_NOARGS,
   "get_center()->x,y\nReturns the center point of the gradient"},
  {"get_object_aligned", (PyCFunction)RadialGradient_get_object_aligned, METH_NOARGS,
   "get_object_aligned()->b\nTrue if the gradient is aligned with the objects, False if aligned with the image"},
  {"get_radii", (PyCFunction)RadialGradient_get_radii, METH_NOARGS,
   "get_radii()->(rx,ry)\nReturns the radiuses of the gradient"},
  {"get_stop", (PyCFunction)RadialGradient_get_stop, METH_VARARGS,
   "get_stop(i)->(offset,(r,g,b[,a])\nGets the color stop with the specified index"},
  {"get_stops", (PyCFunction)RadialGradient_get_stops, METH_NOARGS,
   "get_stops()->list\nReturns a list of color stops"},
  {"get_num_stops", (PyCFunction)RadialGradient_get_num_stops, METH_NOARGS,
   "get_num_stops()->i\nReturns the number of color stops in the gradient"},
  {"set_center", (PyCFunction)RadialGradient_set_center, METH_VARARGS,
   "set_center(x,y)\n\nSets the center point of the gradient"},
  {"set_object_aligned", (PyCFunction)RadialGradient_set_object_aligned, METH_VARARGS,
   "set_object_aligned(b)\n\nSets whether the gradient is aligned with objects"},
  {"set_radii", (PyCFunction)RadialGradient_set_radii, METH_VARARGS,
   "set_radii(rx,ry)\n\nSets the radiuses of the gradient"},
  {0,0,0,0} // Sentinel
};

PyTypeObject RadialGradientType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Gradient", // tp_name
  sizeof(radialGradientObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)RadialGradient_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  (cmpfunc)RadialGradient_compare, // tp_compare
  (reprfunc)RadialGradient_repr, // tp_repr
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
  "Faint Color Gradient", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  RadialGradient_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)RadialGradient_init, // tp_init
  0, // tp_alloc
  RadialGradient_new, // tp_new
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
