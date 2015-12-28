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
#ifndef PYTHON_INCLUDE_HH
#define PYTHON_INCLUDE_HH

#ifdef _WIN32
// On MSW, Microsofts math.h will redefine
// Pythons _hypot in pyconfig.h, issuing a warning: "warning C4211:
// nonstandard extension used : redefined extern to static".
// If <cmath> is included first, this doesn't happen. See:
// http://connect.microsoft.com/VisualStudio/feedback/details/633988/warning-in-math-h-line-162-re-nonstandard-extensions-used
// (Python 2.7, Visual Studio 2010)
#include <cmath>
#endif

#include <Python.h>
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
  int py_compare( const T& lhs, const T& rhs ){
    if ( lhs < rhs ){
      return cmp_less;
    }
    else if ( lhs == rhs ){
      return cmp_equal;
    }
    return cmp_greater;
  }
}



#endif
