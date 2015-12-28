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

#ifndef FAINT_ROTATE_DIALOG_HH
#define FAINT_ROTATE_DIALOG_HH
#include "gui/command-dialog.hh"
#include "util/art-container.hh"

namespace faint{

Optional<Command*> show_rotate_dialog(wxWindow&, DialogFeedback&, const Canvas&, const ArtContainer&);

dialog_func bind_show_rotate_dialog(const ArtContainer&);

}
#endif
