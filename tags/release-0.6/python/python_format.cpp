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
#include "app/getappcontext.hh"
#include "python/pythoninclude.hh"
#include "python/py_canvas.hh"
#include "python/py_imageprops.hh"
#include "python/pyinterface.hh"
#include "python/python_format.hh"
#include "util/canvasinterface.hh"
#include "util/image.hh"

PyFileFormat::PyFileFormat( const load_callback_t& loader, const save_callback_t& saver, const label_t& label, const extension_t& extension )
  : Format( extension, label, can_save(saver.Get() != 0), can_load(loader.Get() != 0) ),
    m_callLoad(0),
    m_callSave(0)
{
  PyObject* loadCallback = loader.Get();
  PyObject* saveCallback = saver.Get();
  if ( loadCallback != 0 ){
    Py_INCREF( loadCallback );
    m_callLoad = loadCallback;
  }
  if ( saveCallback != 0 ){
    Py_INCREF( saveCallback );
    m_callSave = saveCallback;
  }
}

PyFileFormat::~PyFileFormat(){
  if ( m_callLoad != 0 ){
    Py_DECREF( m_callLoad );
  }
  if ( m_callSave != 0 ){
    Py_DECREF( m_callSave );
  }
}

SaveResult PyFileFormat::Save( const std::string& filename, CanvasInterface& canvas ){
  if ( m_callSave == 0 ){
    return SaveResult::SaveFailed("This format can not save"); // Fixme: Either improve error message or prevent this case and assert
  }
  PyObject* py_canvas = pythoned( canvas );
  PyObject* argList = Py_BuildValue("sO", filename.c_str(), py_canvas);
  PyObject* result = PyEval_CallObject( m_callSave, argList );
  Py_DECREF( py_canvas );
  if ( result == 0 ){
    return SaveResult::SaveFailed("");
  }
  Py_DECREF( result );
  return SaveResult::SaveSuccessful();
}

void PyFileFormat::Load( const std::string& filename, ImageProps& props ){
  if ( m_callLoad == 0 ){
    props.SetError("Can not load");
    return;
  }

  PyObject* py_props = pythoned(props);
  PyObject* argList = Py_BuildValue("sO", filename.c_str(), py_props);
  PyObject* result = PyEval_CallObject( m_callLoad, argList );
  Py_DECREF( py_props );
  if ( result == 0 ){
    return;
  }
  Py_DECREF( result );
}
