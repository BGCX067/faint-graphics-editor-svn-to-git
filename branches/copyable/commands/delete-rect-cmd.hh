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

#ifndef FAINT_DELETE_RECT_CMD_HH
#define FAINT_DELETE_RECT_CMD_HH
#include "commands/command.hh"

namespace faint{

Command* get_delete_rect_command(const IntRect&, const Paint&);

Command* get_delete_rect_command( const IntRect&,
  const Paint&,
  const Alternative<Paint>&);

}

#endif