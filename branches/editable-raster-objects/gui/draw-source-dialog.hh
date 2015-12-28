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

#ifndef FAINT_DRAW_SOURCE_DIALOG_HH
#define FAINT_DRAW_SOURCE_DIALOG_HH
#include "util/drawsource.hh"
#include "util/unique.hh"

class StatusInterface;
class wxString;
class wxWindow;

Optional<faint::Color> show_color_only_dialog(wxWindow* parent,
  const wxString& title, const faint::Color& initial );

Optional<faint::DrawSource> show_drawsource_dialog( wxWindow* parent,
  const wxString& title, const faint::DrawSource& initial, StatusInterface& );

#endif
