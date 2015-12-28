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

#ifndef FAINT_TEXT_ENTRY_CMD_HH
#define FAINT_TEXT_ENTRY_CMD_HH
#include "util/distinct.hh"

namespace faint{
class Command;
class ObjText;
class utf8_string;

using NewText = Order<utf8_string>::New;
using OldText = Order<utf8_string>::Old;

Command* text_entry_command(ObjText*, const NewText&, const OldText&);

} // namespace

#endif
