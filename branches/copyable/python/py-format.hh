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

#ifndef FAINT_PY_FORMAT_HH
#define FAINT_PY_FORMAT_HH
#include "formats/format.hh"

namespace faint{

class PyFileFormat;
typedef Distinct<PyObject*, PyFileFormat, 0> load_callback_t;
typedef Distinct<PyObject*, PyFileFormat, 1> save_callback_t;

class PyFileFormat : public Format {
public:
  PyFileFormat( const load_callback_t&, const save_callback_t&, const label_t&, const faint::FileExtension& );
  ~PyFileFormat();
  void Load( const faint::FilePath&, ImageProps& ) override;
  SaveResult Save( const faint::FilePath&, Canvas& ) override;
private:
  PyObject* m_callLoad;
  PyObject* m_callSave;
};

}

#endif
