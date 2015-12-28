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

#include "app/getappcontext.hh"
#include "python/pythoninclude.hh"
#include "python/py-canvas.hh"
#include "python/py-format.hh"
#include "python/py-imageprops.hh"
#include "python/py-util.hh"
#include "python/pyinterface.hh"
#include "util/canvasinterface.hh"
#include "util/formatting.hh"
#include "util/imageprops.hh"

PyFileFormat::PyFileFormat( const load_callback_t& loader, const save_callback_t& saver, const label_t& label, const extension_t& extension )
  : Format( extension, label, can_save(saver.Get() != nullptr), can_load(loader.Get() != nullptr) ),
    m_callLoad(nullptr),
    m_callSave(nullptr)
{
  PyObject* loadCallback = loader.Get();
  PyObject* saveCallback = saver.Get();
  if ( loadCallback != nullptr ){
    Py_INCREF( loadCallback );
    m_callLoad = loadCallback;
  }
  if ( saveCallback != nullptr ){
    Py_INCREF( saveCallback );
    m_callSave = saveCallback;
  }
}

PyFileFormat::~PyFileFormat(){
  if ( m_callLoad != nullptr ){
    faint::py_xdecref( m_callLoad );
  }
  if ( m_callSave != nullptr ){
    faint::py_xdecref( m_callSave );
  }
}

static std::string normal_save_error_prefix( const Format& /*format*/ ){
  return "Save failed\n\n";
}

static std::string internal_save_error_prefix( const Format& format ){
  return "Save Failed (Internal Error)\n\n"
    "An error occurred in the save function for " + format.GetLabel() + "\n\n";
}

static std::string internal_load_error_prefix( const Format& format ){
  return "An Internal Error occured in the load function for " + format.GetLabel() + "\n\n";
}

SaveResult save_error_from_exception(const Format& format){
  if ( faint::py_save_error_occurred() ){
    // A regular save failure was signalled from the Python code
    // (ifaint.SaveError)
    std::string error = normal_save_error_prefix(format) + "\n" +
      faint::py_error_string();
    return SaveResult::SaveFailed(error);
  }

  // Some other exception happened, this is an internal error,
  // Add stack trace etc.
  FaintPyExc error = faint::py_error_info();
  return SaveResult::SaveFailed(internal_save_error_prefix(format) +
    error.type + "\n" + "\n"
    + error.value + "\n" + no_sep(error.stackTrace));
}

SaveResult PyFileFormat::Save( const faint::FilePath& filePath, CanvasInterface& canvas ){
  if ( m_callSave == nullptr ){
    return SaveResult::SaveFailed("This format can not save"); // Fixme: Either improve error message or prevent this case and assert
  }
  PyObject* py_canvas = pythoned( canvas );
  PyObject* argList = Py_BuildValue("sO", filePath.ToAscii().c_str(), py_canvas); // Fixme: Truncates wide filenames
  PyObject* result = PyEval_CallObject( m_callSave, argList );
  faint::py_xdecref(py_canvas);
  faint::py_xdecref(argList);
  if ( result == nullptr ){
    return save_error_from_exception(*this);
  }

  faint::py_xdecref(result);
  return SaveResult::SaveSuccessful();
}

std::string load_error_string_from_exception(const Format& format){
  if ( faint::py_load_error_occurred() ){
    // A regular load failure was signalled from the Python code
    // (ifaint.LoadError)
    return faint::py_error_string();
  }

  // Some other exception happened, this is an internal error
  // Add stack trace etc.
  FaintPyExc error = faint::py_error_info();
  return internal_load_error_prefix(format) + endline_sep(error.type, error.value) + "\n\n" + no_sep(error.stackTrace);
}

void PyFileFormat::Load( const faint::FilePath& filePath, ImageProps& props ){
  if ( m_callLoad == nullptr ){
    props.SetError("Can not load");
    return;
  }

  PyObject* py_props = pythoned(props);
  PyObject* argList = Py_BuildValue("sO", filePath.ToAscii().c_str(), py_props); // Fixme: Truncates wide filenames
  PyObject* result = PyEval_CallObject( m_callLoad, argList );
  faint::py_xdecref(py_props);
  faint::py_xdecref(argList);
  if ( result == nullptr ){
    props.SetError(load_error_string_from_exception(*this));
  }
  else {
    faint::py_xdecref( result );
  }
}
