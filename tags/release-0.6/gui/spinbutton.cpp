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
#include "wx/wx.h"
#include "wx/spinbutt.h"
#include "gui/spinbutton.hh"
#include "util/convertwx.hh"

#ifdef __WXMSW__
class SpinButtonImpl : public wxSpinButton {
public:
  SpinButtonImpl( wxWindow* parent, const wxSize& size, const std::string& toolTip )
    : wxSpinButton(parent, wxID_ANY, wxDefaultPosition, size),
      m_toolTip(toolTip)
  {
    SetToolTip(m_toolTip);
    m_toolTipOn = true;
  }

  void OnLeftDown( wxMouseEvent& evt ){
    SetToolTip("");
    m_toolTipOn = false;
    evt.Skip();
  }

  void OnMouseEnter( wxMouseEvent& evt ){
    if ( !m_toolTipOn ){
      SetToolTip(m_toolTip);
      m_toolTipOn = true;
    }
    evt.Skip();
  }

private:
  bool m_toolTipOn;
  std::string m_toolTip;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SpinButtonImpl, wxSpinButton)
EVT_LEFT_DOWN(SpinButtonImpl::OnLeftDown)
EVT_ENTER_WINDOW(SpinButtonImpl::OnMouseEnter)
END_EVENT_TABLE()

#else

class SpinButtonImpl : public wxPanel{
  // The spin-button-impl doesn't resize the same way in GTK, so just use two stacked buttons
public:
  SpinButtonImpl( wxWindow* parent, const wxSize& size, const std::string& toolTip )
    : wxPanel(parent, wxID_ANY)
  {
    SetInitialSize(size);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_btnPlus = new wxButton(this, wxNewId(), "+", wxDefaultPosition, wxSize(40,25));
    m_btnMinus = new wxButton(this, wxNewId(), "-", wxDefaultPosition, wxSize(40,25));
    sizer->Add(m_btnPlus);
    sizer->Add(m_btnMinus);
    SetSizerAndFit(sizer);
  }

  void OnButton(wxCommandEvent& evt){
    int id = evt.GetId();
    int plusId = m_btnPlus->GetId();
    wxSpinEvent spinEvent(( id == plusId ) ? wxEVT_SPIN_UP : wxEVT_SPIN_DOWN,
			  id);
    spinEvent.SetEventObject(this);
    ProcessEvent(spinEvent);
  }

private:
  wxButton* m_btnPlus;
  wxButton* m_btnMinus;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SpinButtonImpl, wxPanel)
EVT_BUTTON( -1, SpinButtonImpl::OnButton )
END_EVENT_TABLE()
#endif

SpinButton::SpinButton( wxWindow* parent, const IntSize& size, const std::string& toolTip ){
  m_impl = new SpinButtonImpl(parent, to_wx(size), toolTip);
}

wxWindow* SpinButton::GetRaw(){
  return m_impl;
}
