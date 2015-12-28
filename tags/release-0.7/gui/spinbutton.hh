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
#ifndef FAINT_SPINBUTTON_HH
#define FAINT_SPINBUTTON_HH
#include <string>
#include "wx/spinbutt.h"
class SpinButtonImpl;
class wxWindow;
class IntSize;
class wxSize;

class SpinButton{
  // A spinbutton which disables tooltip during size-changes
  // so that it doesn't cover the edited control. The tooltip is
  // re-enabled the next time the pointer enters the control.
public:
  SpinButton( wxWindow* parent, const IntSize&, const std::string& toolTip );
  wxWindow* GetRaw();
private:
  SpinButton( const SpinButton& );
  SpinButtonImpl* m_impl;
};

#endif
