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

#include <sstream>
#include "wx/textctrl.h"
#include "geo/primitive.hh"
#include "gui/math-text-ctrl.hh"
#include "util/gui-util.hh"
#include "util/parse-math-string.hh"

namespace faint{

DEFINE_EVENT_TYPE(EVT_MATH_TEXT_CONTROL_UPDATE)

static std::string format_inactive( int value, int originalValue ){
  std::stringstream ss;
  ss << value << " (" << rounded( floated(value) / floated(originalValue) * 100 ) << "%)";
  return ss.str();
}

std::string format_active( int value ){
  std::stringstream ss;
  ss << value;
  return ss.str();
}


class MathTextCtrlImpl : public wxTextCtrl {
  // Using an impl-class avoids showing all public functions from
  // wxTextCtrl in the public interface of MathTextCtrl.
public:
  MathTextCtrlImpl( wxWindow* parent, int value ) :
    wxTextCtrl( parent, wxID_ANY, "", wxPoint(1,1) )
  {
    m_originalValue = value;
    SetIntValue( value, false );

    Bind(wxEVT_SET_FOCUS, &MathTextCtrlImpl::OnSetFocus, this);
    Bind(wxEVT_KILL_FOCUS, &MathTextCtrlImpl::OnKillFocus, this);
  }

  void SetIntValue( int value, bool hasFocus ){
    m_value = value;
    SetValue( hasFocus ? format_active(m_value) :
      format_inactive( m_value, m_originalValue ) );
  }
  int GetIntValue(){
    UpdateValue();
    return m_value;
  }

  int GetOldIntValue(){
    return m_originalValue;
  }

private:
  int m_value;
  int m_originalValue;

  // When focus is gained, switch to entry-mode formatted value
  void OnSetFocus(wxFocusEvent& event){
    SetValue(format_active(m_value));
    SelectAll();
    event.Skip();
  }

  int UpdateValue(){
    int value = parse_math_string((std::string)GetValue(), m_originalValue);
    int lastValue = m_value;
    if ( value > 0 ){
      m_value = value;
    }
    return lastValue;
  }

  // When focus is lost, display the value with a percentage relation
  // and update the current value
  void OnKillFocus(wxFocusEvent& focusEvent){
    int lastValue = UpdateValue();
    SetValue( format_inactive(m_value, m_originalValue) );
    focusEvent.Skip();

    if ( m_value != lastValue ){
      wxCommandEvent updateEvent( EVT_MATH_TEXT_CONTROL_UPDATE, GetId() );
      // Clients don't know about this class, instead use the
      // parent (MathTextCtrl) as the event object.
      updateEvent.SetEventObject( GetParent() );
      GetEventHandler()->ProcessEvent( updateEvent );
    }
  }
};

MathTextCtrl::MathTextCtrl( wxWindow* parent, int value )
  : wxPanel(parent, wxID_ANY)
{
  m_textCtrl = new MathTextCtrlImpl( this, value );
  SetValue( value );
  wxSize size( m_textCtrl->GetSize() );
  SetMinSize(wxSize(size.x + 2, size.y + 2));
}

void MathTextCtrl::SetValue( int value ){
  m_textCtrl->SetIntValue(value, m_textCtrl->HasFocus());
}

int MathTextCtrl::GetValue() const{
  return m_textCtrl->GetIntValue();
}

int MathTextCtrl::GetOldValue() const{
  return m_textCtrl->GetOldIntValue();
}

bool MathTextCtrl::HasFocus() const{
  return m_textCtrl->HasFocus();
}

void MathTextCtrl::FitSizeTo( const wxString& str ){
  fit_size_to(m_textCtrl, str);
  wxSize size( m_textCtrl->GetSize() );
  SetMinSize(wxSize(size.x + 2, size.y + 2));
}

} // namespace
