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

#include "gui/status-button.hh"
#include "util/convert-wx.hh"

namespace faint{

StatusButton::StatusButton(wxWindow* parent, const wxSize& size, StatusInterface& status, const faint::utf8_string& label, const tooltip_t& tooltip, const description_t& description)
  : wxButton(parent, wxNewId(), to_wx(label), wxDefaultPosition, size, wxWANTS_CHARS), // wxWANTS_CHARS prevents sound on keypress when button has focus
    m_description(description.Get()),
    m_status(status)
{
  SetToolTip(to_wx(tooltip.Get()));

  Bind(wxEVT_LEAVE_WINDOW, &StatusButton::OnLeave, this);
  Bind(wxEVT_MOTION, &StatusButton::OnMotion, this);
}

void StatusButton::OnLeave(wxMouseEvent& event){
  m_status.SetMainText("");
  event.Skip();
}

void StatusButton::OnMotion(wxMouseEvent& event){
  // Set the status bar description. Done in EVT_MOTION-handler
  // because it did not work well in EVT_ENTER (the enter event
  // appears to be missed for example when moving between buttons)
  m_status.SetMainText(m_description);
  event.Skip();
}

void StatusButton::UpdateText(const faint::utf8_string& label, const tooltip_t& tooltip, const description_t& description){
  SetLabel(to_wx(label));
  m_description = description.Get();
  SetToolTip(to_wx(tooltip.Get()));
}

ToggleStatusButton::ToggleStatusButton(wxWindow* parent, int id, const wxSize& size, StatusInterface& status, const wxBitmap& bmp, const tooltip_t& tooltip, const description_t& description)
: wxBitmapToggleButton(parent, id, bmp, wxDefaultPosition, size, wxWANTS_CHARS),
  m_description(description.Get()),
  m_status(status)
{
  SetToolTip(to_wx(tooltip.Get()));
  Bind(wxEVT_LEAVE_WINDOW, &ToggleStatusButton::OnLeave, this);
  Bind(wxEVT_MOTION, &ToggleStatusButton::OnMotion, this);
}

void ToggleStatusButton::OnLeave(wxMouseEvent& event){
  m_status.SetMainText("");
  event.Skip();
}

void ToggleStatusButton::OnMotion(wxMouseEvent& event){
  // Set the status bar description. Done in EVT_MOTION-handler
  // because it did not work well in EVT_ENTER (the enter event
  // appears to be missed for example when moving between buttons)
  m_status.SetMainText(m_description);
  event.Skip();
}

}
