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

#include "wx/button.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/spinbutt.h"
#include "gui/spin-button.hh"
#include "util-wx/convert-wx.hh"

namespace faint{

#ifdef __WXMSW__
// MSW-implementation
class SpinButtonImpl : public wxSpinButton {
public:
  SpinButtonImpl(wxWindow* parent, const wxSize& size, const std::string& toolTip)
    : wxSpinButton(parent, wxID_ANY, wxDefaultPosition, size)
  {
    SetToolTip(toolTip);
  }
private:
};

#else

class SpinButtonImpl : public wxPanel{
  // The wxSpinButton doesn't resize the same way in GTK, so here's a
  // variant which just uses two stacked buttons (A spinbutton would
  // be preferable as it repeats while held down).
public:
  SpinButtonImpl(wxWindow* parent, const wxSize& size, const std::string& toolTip)
    : wxPanel(parent, wxID_ANY)
  {
    SetInitialSize(size);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_btnPlus = new wxButton(this, wxNewId(),
      "+", wxDefaultPosition, wxSize(40,25));
    m_btnMinus = new wxButton(this, wxNewId(),
      "-", wxDefaultPosition, wxSize(40,25));
    sizer->Add(m_btnPlus);
    sizer->Add(m_btnMinus);
    SetSizerAndFit(sizer);
    SetToolTip(toolTip);

    Bind(wxEVT_BUTTON,
      [this](wxCommandEvent& evt){
        int id = evt.GetId();
        int plusId = m_btnPlus->GetId();
        wxSpinEvent spinEvent((id == plusId) ? wxEVT_SPIN_UP : wxEVT_SPIN_DOWN,
          id);
        spinEvent.SetEventObject(this);
        ProcessEvent(spinEvent);
      });
  }

private:
  wxButton* m_btnPlus;
  wxButton* m_btnMinus;
};

#endif

SpinButton::SpinButton(wxWindow* parent,
  const IntSize& size,
  const std::string& toolTip)
{
  m_impl = new SpinButtonImpl(parent, to_wx(size), toolTip);
}

wxWindow* SpinButton::GetRaw(){
  return m_impl;
}

} // namespace
