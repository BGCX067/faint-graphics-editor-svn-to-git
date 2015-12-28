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

#ifdef _WIN32
// Py_DECREF uses while (0), which causes warning C4127: conditional
// expression is constant (Python2.7, Visual Studio 2010)
#pragma warning (disable : 4127)
#endif
#include "canvasinterface.hh"
#include "pythoninclude.hh"
#include "python_format.hh"
#include "py_imageprops.hh"
#include "py_canvas.hh"
#include "pyinterface.hh"
#include "getappcontext.hh"

PyFileFormat::PyFileFormat( PyObject* callback_load, PyObject* callback_save, const std::string& name, const std::string& extension )
  : Format( extension, name, callback_save != 0, callback_load != 0 )
{
  if ( callback_load != 0 ){
    Py_INCREF( callback_load );
    m_callLoad = callback_load;
  }
  if ( callback_save != 0 ){
    Py_INCREF( callback_save );
    m_callSave = callback_save;
  }
}

bool PyFileFormat::Save( const std::string& filename, CanvasInterface& canvas ){
  if ( m_callSave == 0 ){
    return false;
  }
  PyObject* py_canvas = Pythoned( canvas );
  PyObject* argList = Py_BuildValue("sO", filename.c_str(), py_canvas);
  PyObject* result = PyEval_CallObject( m_callSave, argList );
  Py_DECREF( py_canvas );
  if ( result == 0 ){
    return false;
  }
  Py_DECREF( result );
  return true;
}

void PyFileFormat::Load( const std::string& filename, ImageProps& props ){
  if ( m_callLoad == 0 ){
    props.SetError("Can not load");
    return;
  }

  PyObject* py_props = Pythoned(props);
  PyObject* argList = Py_BuildValue("sO", filename.c_str(), py_props);
  PyObject* result = PyEval_CallObject( m_callLoad, argList );
  Py_DECREF( py_props );
  if ( result == 0 ){
    return;
  }
  Py_DECREF( result );
}
