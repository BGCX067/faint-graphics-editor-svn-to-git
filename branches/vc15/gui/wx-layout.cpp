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

#include "wx/dialog.h"
#include "wx/sizer.h"
#include "gui/layout.hh"
#include "gui/wx-layout.hh"
#include "util/iter.hh"

namespace faint{namespace layout{

SizerItem::SizerItem(wxWindow* in_item, int in_proportion, int in_align)
  : align(in_align),
    proportion(in_proportion),
    sizerItem(nullptr),
    windowItem(in_item)
{}

SizerItem::SizerItem(wxWindow* in_item)
  : align(wxALIGN_CENTER_VERTICAL),
    proportion(0),
    sizerItem(nullptr),
    windowItem(in_item)
{}

SizerItem::SizerItem(wxSizer* in_item, int in_proportion, int in_align)
  : align(in_align),
    proportion(in_proportion),
    sizerItem(in_item),
    windowItem(nullptr)
{}

SizerItem::SizerItem(wxSizer* in_item)
  : align(wxALIGN_CENTER_VERTICAL),
    proportion(0),
    sizerItem(in_item),
    windowItem(nullptr)
{}

wxSizer* create_row(const std::vector<SizerItem>& items){
  wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
  row->AddSpacer(panel_padding);
  for (auto& item : but_last(items)){
    if (item.windowItem != nullptr){
      row->Add(item.windowItem, item.proportion, item.align);
    }
    else{
      row->Add(item.sizerItem, item.proportion, item.align);
    }
    row->AddSpacer(item_spacing);
  }
  auto& last = items.back();
  if (last.windowItem != nullptr){
    row->Add(last.windowItem, last.proportion, last.align);
  }
  else{
    row->Add(last.sizerItem, last.proportion, last.align);
  }
  row->AddSpacer(panel_padding);
  return row;
}

wxSizer* create_row_no_pad(const std::vector<SizerItem>& items){
  wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
  for (auto& item : items){
    if (item.windowItem != nullptr){
      row->Add(item.windowItem, item.proportion, item.align);
    }
    else{
      row->Add(item.sizerItem, item.proportion, item.align);
    }
  }
  return row;
}

wxSizer* create_column(const std::vector<SizerItem>& items){
  wxBoxSizer* column = new wxBoxSizer(wxVERTICAL);
  column->AddSpacer(panel_padding);
  for (auto& item : but_last(items)){
    if (item.windowItem != nullptr){
      column->Add(item.windowItem, item.proportion, item.align);
    }
    else{
      column->Add(item.sizerItem, item.proportion, item.align);
    }
    column->AddSpacer(item_spacing);
  }
  auto& last = items.back();
  if (last.windowItem != nullptr){
    column->Add(last.windowItem, last.proportion, last.align);
  }
  else{
    column->Add(last.sizerItem, last.proportion, last.align);
  }
  column->AddSpacer(panel_padding);

  return column;
}

SizerItem grow(wxSizer* sizer){
  return SizerItem(sizer, 1, wxEXPAND);
}

SizerItem grow(wxWindow* window){
  return SizerItem(window, 1, wxEXPAND);
}

SizerItem center(wxSizer* sizer){
  return SizerItem(sizer, 0, wxALIGN_CENTER);
}

wxWindow* make_default(wxDialog* parent, wxWindow* item){
  parent->SetDefaultItem(item);
  return item;
}

wxStaticText* label(wxWindow* parent, const wxString& text){
  return new wxStaticText(parent, wxID_ANY, text);
}

}} // namespace
