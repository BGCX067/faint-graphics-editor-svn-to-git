#ifndef FAINT_VALUE_TEXT_CONTROL
#define FAINT_VALUE_TEXT_CONTROL
#include "wx/panel.h"

class ValueTextControlImpl;

// Event sent by ValueTextCtrl when the value is changed by user entry
DECLARE_EVENT_TYPE(EVT_VALUE_TEXT_CONTROL_UPDATE, -1)

class ValueTextControl : public wxPanel{
public:
  ValueTextControl( wxWindow* parent, int value );
  void FitSizeTo( const wxString& );
  void SetValue( int );
  int GetValue() const;
  int GetOldValue() const;
  virtual bool HasFocus() const;
private:
  ValueTextControlImpl* m_textCtrl;
};

#endif
