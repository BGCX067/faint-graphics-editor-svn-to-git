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

#ifndef FAINT_PY_INCLUDE_HH
#define FAINT_PY_INCLUDE_HH

#ifdef _WIN32
// pymath.h:s round conflicts with math.h round
#define HAVE_ROUND

#pragma warning(push)
// Warnings emitted by Python header since 3.4
#pragma warning(disable:4510) // default constructor could not be generated
#pragma warning(disable:4512) // 'class' : assignment operator could not be generated
#pragma warning(disable:4610) // 'class' can never be instantiated - user-defined constructor required
#endif

// On Windows, debug python (python27_d.lib) would be linked if _DEBUG
// is defined temporarily undefine to allow linking release Python
// with a debug build of Faint.
#ifdef _DEBUG
#define FAINT_UNDEFFED_DEBUG
#undef _DEBUG
#endif

#include <Python.h>

#ifdef _WIN32
#pragma warning(pop)
#endif
// Redefine _DEBUG
#ifdef  FAINT_UNDEFFED_DEBUG
#define _DEBUG
#undef FAINT_UNDEFFED_DEBUG
#endif

#ifdef _WIN32
// The GETTER-wrapper silences C4191 in VC10,
#define GETTER(func)(getter)((void*)(func)) // Silence C4191 in vc
#define SETTER(func) (setter)(func) // for symmetry

// Again, silence C4191. This time for inserting a METH_VARARGS function in a PyMethodDef
// Hopefully benign..
#define FAINT_KW_TO_PY(func)(PyCFunction)((void*)(func))
#else
// With g++, the WIN32-workaround causes "ISO C++ forbids casting
// between pointer-to-function and pointer-to-object,
// - The normal (getter)(func) causes no warnings.
#define GETTER(func)(getter)(func)
#define SETTER(func)(setter)(func)
#define FAINT_KW_TO_PY(func)(PyCFunction)(func)
#endif


namespace faint{
  // Custom variant of Py_XDECREF, to avoid visual studio warning
  // C4127: conditional expression is constant
  void py_xdecref( PyObject* );

  class ScopedRef{
  public:
    explicit ScopedRef( PyObject* ref ) : m_ref(ref)
    {}
    ScopedRef()
      : m_ref(nullptr)
    {}
    ~ScopedRef(){
      py_xdecref(m_ref);
    }
    PyObject* operator*(){
      return m_ref;
    }
    void Release(){
      m_ref = nullptr;
    }
    void Reset( PyObject* ref ){
      m_ref = ref;
    }
  private:
    ScopedRef( const ScopedRef& );
    ScopedRef& operator=(const ScopedRef&);
    PyObject* m_ref;
  };

  const int init_ok = 0;
  const int init_fail = -1;
  const int setter_ok = 0;
  const int setter_fail = -1;
  const int cmp_less = -1;
  const int cmp_equal = 0;
  const int cmp_greater = 1;

  template<typename T>
  PyObject* py_rich_compare( const T& lhs, const T& rhs, int op ){
    if ( lhs < rhs ){
      if ( op == Py_LT || op == Py_LE){
        return Py_True;
      }
      return Py_False;
    }
    else if ( lhs == rhs ){
      if ( op == Py_EQ || op == Py_LE || op == Py_GE ){
        return Py_True;
      }
      return Py_False;
    }
    else {
      assert( lhs > rhs );
      if ( op == Py_GT || op == Py_GE ){
        return Py_True;
      }
      return Py_False;
    }
  }

} // namespace

#endif
