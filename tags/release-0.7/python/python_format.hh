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

class PyFileFormat;
typedef Unique<PyObject*, PyFileFormat, 0> load_callback_t;
typedef Unique<PyObject*, PyFileFormat, 1> save_callback_t;

class PyFileFormat : public Format {
public:
  PyFileFormat( const load_callback_t&, const save_callback_t&, const label_t&, const extension_t& );
  ~PyFileFormat();
  void Load( const std::string& filename, ImageProps& );
  SaveResult Save( const std::string& filename, CanvasInterface& );
private:
  PyObject* m_callLoad;
  PyObject* m_callSave;
};

#endif
