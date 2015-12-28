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

#ifndef FAINT_PYFILEFORMAT_HH
#define FAINT_PYFILEFORMAT_HH
#include "formats/format.hh"

class PyFileFormat : public Format {
public:
  PyFileFormat( PyObject* call_load, PyObject* call_save, const std::string& name, const std::string& extension );
  ~PyFileFormat();
  bool Save( const std::string& filename, CanvasInterface& );
  void Load( const std::string& filename, ImageProps& );
private:
  PyObject* m_callLoad;
  PyObject* m_callSave;
};

#endif
