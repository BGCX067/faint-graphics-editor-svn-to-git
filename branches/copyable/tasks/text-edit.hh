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

#ifndef FAINT_TEXT_EDIT_HH
#define FAINT_TEXT_EDIT_HH
#include "tasks/task.hh"
#include "text/utf8-string.hh"
#include "util/keycode.hh"

namespace faint{
class ObjText;

Task* edit_text_task(const Rect&, const faint::utf8_string&, Settings&);
Task* edit_text_task(ObjText*, Settings&);

}

#endif
