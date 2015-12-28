// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_PY_FUNCTION_ERROR_HH
#define FAINT_PY_FUNCTION_ERROR_HH
#include "python/py-include.hh"
#include "text/utf8-string.hh"

namespace faint{
// Exceptions for errors in Python interface functions. Caught by the
// function forwarding in py-ugly-forward.hh.

class PythonError{
public:
  // Sets the error using PyErr_...
  void SetError() const;
  virtual ~PythonError() = 0;
protected:
  PythonError(PyObject*, const faint::utf8_string&);
private:
  PyObject* m_errorType;
  faint::utf8_string m_error;
};

class ValueError : public PythonError{
public:
  ValueError(const faint::utf8_string&);
};

class TypeError : public faint::PythonError{
public:
  TypeError(const faint::utf8_string&);
};

class PresetFunctionError{
  // Exception for errors in Python interface functions. Should be
  // thrown if a specific error has already been set with
  // PyErr_SetString. Caught by the function forwarding in
  // py-ugly-forward.hh (which will not set any Python-error).
public:
  PresetFunctionError();

private:
};

} // namespace

#endif
