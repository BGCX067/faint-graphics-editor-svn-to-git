#include <sstream>
#include "wx/textctrl.h"
#include "geo/basic.hh"
#include "gui/valuetextctrl.hh"
#include "util/parse-math-string.hh"

DEFINE_EVENT_TYPE(EVT_VALUE_TEXT_CONTROL_UPDATE)

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


class ValueTextControlImpl : public wxTextCtrl {
  // I use this impl-class to avoid showing all public functions from
  // wxTextCtrl in the public interface of ValueTextControl.
public:
  ValueTextControlImpl( wxWindow* parent, int value ) :
    wxTextCtrl( parent, wxID_ANY, "", wxPoint(1,1) )
  {
    m_originalValue = value;
    SetIntValue( value, false );
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
  m_textCtrl->SetIntValue(value, m_textCtrl->HasFocus());
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

void ValueTextControl::FitSizeTo( const wxString& str ){
  wxSize extents = m_textCtrl->GetTextExtent(str);
  m_textCtrl->SetInitialSize(wxSize(extents.x + 10, -1));
  wxSize size( m_textCtrl->GetSize() );
  SetMinSize(wxSize(size.x + 2, size.y + 2));
}
