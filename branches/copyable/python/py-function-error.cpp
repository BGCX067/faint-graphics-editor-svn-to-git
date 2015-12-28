// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#include "python/py-function-error.hh"
#include "python/py-interface.hh"
#include "python/py-util.hh"

namespace faint{

PythonError::PythonError(PyObject* errorType, const utf8_string& error)
  : m_errorType(errorType),
    m_error(error)
{}

PythonError::~PythonError(){
}

void PythonError::SetError() const{
  ScopedRef obj(build_unicode(m_error));
  PyErr_SetObject(m_errorType, *obj);
}

ValueError::ValueError(const utf8_string& error)
  : PythonError(PyExc_ValueError, error)
{}

TypeError::TypeError(const utf8_string& error)
  : PythonError(PyExc_TypeError, error)
{}

PresetFunctionError::PresetFunctionError()
{}

} // namespace
