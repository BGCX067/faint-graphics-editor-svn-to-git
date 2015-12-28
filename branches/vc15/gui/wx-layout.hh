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

#ifndef FAINT_WX_LAYOUT_HH
#define FAINT_WX_LAYOUT_HH
#include <vector>
#include "wx/stattext.h"

class wxDialog;
class wxString;
class wxWindow;
class wxSizer;

namespace faint{ namespace layout{

class SizerItem{
public:
  SizerItem(wxWindow*, int in_proportion, int in_align);
  SizerItem(wxWindow* in_item);
  SizerItem(wxSizer* in_item, int in_proportion, int in_align);
  SizerItem(wxSizer* in_item);
  int align;
  int proportion;
  wxSizer* sizerItem;
  wxWindow* windowItem;
};

wxSizer* create_row(const std::vector<SizerItem>&);
wxSizer* create_row_no_pad(const std::vector<SizerItem>&);
wxSizer* create_column(const std::vector<SizerItem>&);
SizerItem grow(wxSizer*);
SizerItem grow(wxWindow*);
SizerItem center(wxSizer*);

wxWindow* make_default(wxDialog*, wxWindow*);

wxStaticText* label(wxWindow* parent, const wxString&);

}} // Namespace

#endif
