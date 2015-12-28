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

#ifndef FAINT_MATH_TEXT_CTRL_HH
#define FAINT_MATH_TEXT_CTRL_HH
#include "wx/panel.h"

namespace faint{

class MathTextCtrlImpl;

// Event sent by MathTextCtrl when the value is changed by user entry
DECLARE_EVENT_TYPE(EVT_MATH_TEXT_CONTROL_UPDATE, -1)

class MathTextCtrl : public wxPanel{
public:
  MathTextCtrl( wxWindow* parent, int value );
  void FitSizeTo( const wxString& );
  void SetValue( int );
  int GetValue() const;
  int GetOldValue() const;
  virtual bool HasFocus() const;
private:
  MathTextCtrlImpl* m_textCtrl;
};

}
#endif
