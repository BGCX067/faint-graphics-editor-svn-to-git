#include "pythoninclude.hh"
#ifdef _WIN32
// Py_DECREF uses while (0), which causes warning C4127: conditional
// expression is constant (Python2.7, Visual Studio 2010)
#pragma warning (disable : 4127)
#endif

namespace faint{
  void py_xdecref( PyObject* o ){
    if ( o != nullptr ){
      Py_DECREF(o);
    }
  }
}
