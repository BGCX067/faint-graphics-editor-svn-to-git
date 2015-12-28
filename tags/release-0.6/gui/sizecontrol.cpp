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

#include "sizecontrol.hh"
#include "wx/wx.h"

class FocusRelayingSpinCtrl : public wxSpinCtrl{
public:
  FocusRelayingSpinCtrl( wxWindow* parent, wxSize size ) :
    wxSpinCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER )
  {}

  void OnSetFocus(wxFocusEvent& focusEvent){
    wxCommandEvent newEvent( EVT_SET_FOCUS_ENTRY_CONTROL, wxID_ANY );
    newEvent.SetEventObject( this );
    GetEventHandler()->ProcessEvent( newEvent );
    focusEvent.Skip();
  }

  void OnKillFocus(wxFocusEvent& focusEvent){
    wxCommandEvent newEvent( EVT_KILL_FOCUS_ENTRY_CONTROL, wxID_ANY );
    newEvent.SetEventObject( this );
    GetEventHandler()->ProcessEvent( newEvent );
    focusEvent.Skip();    
}
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FocusRelayingSpinCtrl, wxSpinCtrl)
EVT_SET_FOCUS(FocusRelayingSpinCtrl::OnSetFocus)
EVT_KILL_FOCUS(FocusRelayingSpinCtrl::OnKillFocus)
END_EVENT_TABLE()

IntSizeControl::IntSizeControl(wxWindow* parent, wxSize size, const IntSetting& setting, int value, const std::string& label)
  : IntSettingCtrl(parent, setting)
{
  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  if ( label.size() > 0 ){
    sizer->Add(new wxStaticText(this, wxID_ANY, label));
  }
  m_spinCtrl = new FocusRelayingSpinCtrl(this, size);
  m_spinCtrl->SetValue(value);
  m_spinCtrl->SetRange(1, 255);
  m_spinCtrl->SetBackgroundColour( wxColour( 255, 255, 255 ) );
  sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( sizer );
}

int IntSizeControl::GetValue() const{
  return m_spinCtrl->GetValue();
}

void IntSizeControl::SetValue( int val ){
  m_spinCtrl->SetValue( val );
}

void IntSizeControl::OnSpin( wxSpinEvent& event ){
  event.Skip();
  SendChangeEvent();
}

void IntSizeControl::OnEnter(wxCommandEvent&){
  SendChangeEvent();
}

BEGIN_EVENT_TABLE(IntSizeControl, ToolSettingCtrl)
EVT_SPINCTRL(-1, IntSizeControl::OnSpin)
EVT_TEXT_ENTER(-1, IntSizeControl::OnEnter)
END_EVENT_TABLE()

FloatSizeControl::FloatSizeControl(wxWindow* parent, wxSize size, const FloatSetting& setting, faint::coord value, const std::string& label)
: FloatSettingControl(parent, setting)
{

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  if ( label.size() > 0 ){
    sizer->Add(new wxStaticText(this, wxID_ANY, label));
  }
  m_spinCtrl = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, size, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER);
  m_spinCtrl->SetValue(value);
  m_spinCtrl->SetRange(0.1, 255);
  m_spinCtrl->SetBackgroundColour( wxColour( 255, 255, 255 ) );

  sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( sizer );
}

faint::coord FloatSizeControl::GetValue() const{
  return m_spinCtrl->GetValue();
}

void FloatSizeControl::SetValue( faint::coord val ){
  m_spinCtrl->SetValue( val );
}

void FloatSizeControl::OnSpin(wxSpinDoubleEvent& event){
  event.Skip();
  SendChangeEvent();
}

void FloatSizeControl::OnEnter(wxCommandEvent&){
  SendChangeEvent();
}

BEGIN_EVENT_TABLE(FloatSizeControl, ToolSettingCtrl)
EVT_SPINCTRLDOUBLE(-1, FloatSizeControl::OnSpin)
EVT_TEXT_ENTER(-1, FloatSizeControl::OnEnter)
END_EVENT_TABLE()

SemiFloatSizeControl::SemiFloatSizeControl(wxWindow* parent, wxSize size, const FloatSetting& setting, faint::coord value, const std::string& label)
  : FloatSettingControl(parent, setting)
{
  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  if ( label.size() > 0 ){
    sizer->Add(new wxStaticText(this, wxID_ANY, label));
  }
  m_spinCtrl = new FocusRelayingSpinCtrl(this, size );
  m_spinCtrl->SetValue(static_cast<int>(value));
  m_spinCtrl->SetRange(1, 255);
  m_spinCtrl->SetBackgroundColour( wxColour( 255, 255, 255 ) );
  sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( sizer );
}

faint::coord SemiFloatSizeControl::GetValue() const{
  return static_cast<faint::coord>(m_spinCtrl->GetValue());
}

void SemiFloatSizeControl::SetValue( faint::coord value ){
  m_spinCtrl->SetValue( static_cast<int>(value) );
}

void SemiFloatSizeControl::OnSpin(wxSpinEvent& event){
  event.Skip();
  SendChangeEvent();
}

void SemiFloatSizeControl::OnEnter(wxCommandEvent&){
  SendChangeEvent();
}

BEGIN_EVENT_TABLE(SemiFloatSizeControl, ToolSettingCtrl)
EVT_SPINCTRL(-1, SemiFloatSizeControl::OnSpin)
EVT_TEXT_ENTER(-1, SemiFloatSizeControl::OnEnter)
END_EVENT_TABLE()
