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
#include "python/py-linear-gradient.hh"
#include "python/py-util.hh"

static int LinearGradient_compare(linearGradientObject* self, PyObject* otherRaw ){
  if ( !PyObject_IsInstance( otherRaw, (PyObject*)&LinearGradientType) ){
    // Use less for type mismatch
    return faint::cmp_less;
  }
  linearGradientObject* other = (linearGradientObject*)otherRaw;
  const faint::LinearGradient& lhs(*self->gradient);
  const faint::LinearGradient& rhs(*other->gradient);
  return faint::py_compare(lhs, rhs);
}

static int LinearGradient_init(linearGradientObject* self, PyObject* args, PyObject* ){
  int numArgs = PySequence_Length(args);
  if ( numArgs == 0 ){
    PyErr_SetString(PyExc_ValueError, "Atleast one color stop required");
    return faint::init_fail;
  }

  // The angle is an optional argument (although the first!)
  faint::radian angle = 0.0;

  // If the first argument is an angle, the color stop parsing should be offset
  int firstColorStop = 0;

  PyObject* unwrapped = nullptr;
  if ( numArgs == 1 ){
    // Check if this is a single color stop
    PyObject* pyStop = PySequence_GetItem(args, 0);
    faint::ColorStop stop;
    if ( parse_color_stop(pyStop, stop) ){
      faint::py_xdecref(pyStop);
      std::vector<faint::ColorStop> v;
      v.push_back(stop);
      self->gradient = new faint::LinearGradient(0.0, v);
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
    // Check if the first argument is an angle, if so parse it.
    PyObject* firstArg = PySequence_GetItem(args, 0);
    if ( PyNumber_Check( firstArg ) ){
      PyObject* pyAngleFloat = PyNumber_Float(firstArg);
      angle = PyFloat_AsDouble(pyAngleFloat);
      faint::py_xdecref(pyAngleFloat);
      firstColorStop += 1;
    }
    faint::py_xdecref(firstArg);
  }

  std::vector<faint::ColorStop> stops;
  for ( int i = firstColorStop; i != numArgs; i++ ){
    PyObject* pyStop = PySequence_GetItem(args, i);
    faint::ColorStop stop;
    bool ok = parse_color_stop(pyStop, stop);
    faint::py_xdecref(pyStop);
    if ( !ok ){
      faint::py_xdecref(unwrapped);
      return faint::init_fail;
    }
    stops.push_back(stop);
  }

  self->gradient = new faint::LinearGradient(angle, stops);
  faint::py_xdecref(unwrapped);
  return faint::init_ok;
}

static PyObject* LinearGradient_new(PyTypeObject* type, PyObject*, PyObject* ){
  linearGradientObject* self = (linearGradientObject*)type->tp_alloc(type, 0);
  self->gradient = nullptr;
  return (PyObject*)self;
}

static PyObject* LinearGradient_repr( linearGradientObject* self ){
  return Py_BuildValue("s", get_repr(*self->gradient).c_str());
}

static void LinearGradient_dealloc( linearGradientObject* self ){
  delete self->gradient;
  self->gradient = nullptr;
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* LinearGradient_add_stop( linearGradientObject* self, PyObject* args ){
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

static PyObject* LinearGradient_get_angle( linearGradientObject* self, PyObject* ){
  return build_radian(self->gradient->GetAngle());
}

static PyObject* LinearGradient_get_num_stops( linearGradientObject* self ){
  return Py_BuildValue("i", self->gradient->GetNumStops());
}

static PyObject* LinearGradient_get_stop( linearGradientObject* self, PyObject* args ){
  int index;
  if ( !PyArg_ParseTuple(args, "i", &index) ){
    return nullptr;
  }

  if ( index < 0 || self->gradient->GetNumStops() <= index ){
    PyErr_SetString(PyExc_IndexError, "Index out of range");
    return nullptr;
  }

  return build_color_stop( self->gradient->GetStop(index) );
}
static PyObject* LinearGradient_get_stops( linearGradientObject* self ){
  return build_color_stops(*self->gradient);
}

static PyObject* LinearGradient_set_angle( linearGradientObject* self, PyObject* args ){
  faint::radian angle;
  if ( !parse_radian(args, &angle) ){
    return nullptr;
  }
  self->gradient->SetAngle(angle);
  return Py_BuildValue("");
}

static PyMethodDef LinearGradient_methods[] = {
  {"add_stop", (PyCFunction)LinearGradient_add_stop, METH_VARARGS,
   "add_stop(offset,(r,g,b[,a]))\nAdds the specified color stop to the gradient"},
  {"get_angle", (PyCFunction)LinearGradient_get_angle, METH_NOARGS,
   "get_angle()->angle\nReturns the angle, specified in radians."},
  {"get_stop", (PyCFunction)LinearGradient_get_stop, METH_VARARGS,
   "get_stop(i)->(offset,(r,g,b[,a])\nGets the color stop with the specified index"},
  {"get_stops", (PyCFunction)LinearGradient_get_stops, METH_NOARGS,
   "get_stops()->list\nReturns a list of color stops"},
  {"get_num_stops", (PyCFunction)LinearGradient_get_num_stops, METH_NOARGS,
   "get_num_stops()->i\nReturns the number of color stops in the gradient"},
  {"set_angle", (PyCFunction)LinearGradient_set_angle, METH_VARARGS,
   "set_angle(a)\nSet the angle, specified in radians."},
  {0,0,0,0} // Sentinel
};

PyTypeObject LinearGradientType = {
  PyObject_HEAD_INIT(nullptr)
  0, // ob_size
  "Gradient", // tp_name
  sizeof(linearGradientObject), // tp_basicsize
  0, // tp_itemsize
  (destructor)LinearGradient_dealloc, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  (cmpfunc)LinearGradient_compare, // tp_compare
  (reprfunc)LinearGradient_repr, // tp_repr
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
  LinearGradient_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)LinearGradient_init, // tp_init
  0, // tp_alloc
  LinearGradient_new, // tp_new
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
