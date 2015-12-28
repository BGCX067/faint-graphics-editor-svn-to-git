#include "valuetextctrl.hh"
#include <sstream>
#include "wx/textctrl.h"
#include "parsevalue.hh"

DEFINE_EVENT_TYPE(EVT_VALUE_TEXT_CONTROL_UPDATE)

std::string FormatInactive( int value, int originalValue ){
  std::stringstream ss;
  ss << value << " (" << int((float(value) / originalValue)*100 + 0.5) << "%)";
  return ss.str();
}

std::string FormatActive( int value, int /*originalValue*/ ){
  std::stringstream ss;
  ss << value;
  return ss.str();
}

// I use this impl-class, ValueTextCtrlImpl, to avoid showing all
// public functions from wxTextCtrl in the public interface, ValueTextCtrl
class ValueTextControlImpl : public wxTextCtrl {
public:
  ValueTextControlImpl( wxWindow* parent, int value ) :
    wxTextCtrl( parent, wxID_ANY, "", wxPoint(1,1) )
  {
    m_originalValue = value;
    SetIntValue( value );
#ifndef __WXMSW__ 
    // Fixme: What is this about?
    SetInitialSize(wxSize(115,-1));
    #endif
    
  }
  void SetIntValue( int value ){
    m_value = value;
    SetValue( FormatInactive( m_value, m_originalValue ) );
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
    SetValue(FormatActive(m_value, m_originalValue));
    SelectAll();
    event.Skip();
  }

  int UpdateValue(){
    int value = ::ParseValue((std::string)GetValue(), m_originalValue);
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
    SetValue(FormatInactive(m_value, m_originalValue));
    focusEvent.Skip();

    if ( m_value != lastValue ){
      wxCommandEvent updateEvent( EVT_VALUE_TEXT_CONTROL_UPDATE, GetId() );
      // Clients don't know about this class, instead use the
      // parent (ValueTextCtrl) as the event object.
      updateEvent.SetEventObject( GetParent() );
      GetEventHandler()->ProcessEvent( updateEvent );
    }
  }
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ValueTextControlImpl, wxTextCtrl)
EVT_SET_FOCUS(ValueTextControlImpl::OnSetFocus)
EVT_KILL_FOCUS(ValueTextControlImpl::OnKillFocus)
END_EVENT_TABLE()

ValueTextControl::ValueTextControl( wxWindow* parent, int value )
  : wxPanel(parent, wxID_ANY)
{
  m_textCtrl = new ValueTextControlImpl( this, value );
  SetValue( value );
  wxSize size( m_textCtrl->GetSize() );
  SetMinSize(wxSize(size.x + 2, size.y + 2));
}

void ValueTextControl::SetValue( int value ){
  m_textCtrl->SetIntValue(value);
}

int ValueTextControl::GetValue() const{  
  return m_textCtrl->GetIntValue();
}

int ValueTextControl::GetOldValue() const{
  return m_textCtrl->GetOldIntValue();
}

bool ValueTextControl::HasFocus() const{
  return m_textCtrl->HasFocus();
}
