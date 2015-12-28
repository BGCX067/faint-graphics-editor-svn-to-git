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

#ifndef FAINT_FONTCONTROL_HH
#define FAINT_FONTCONTROL_HH
#include "gui/toolsettingctrl.hh"

class FontControl : public ToolSettingCtrl {
public:
  FontControl( wxWindow* parent );
  StrSetting GetSetting() const;
  virtual UntypedSetting GetUntypedSetting() const;
  virtual std::string GetValue() const;
  virtual void Notify( SettingNotifier& );
  virtual void SetValue( const std::string& );
  virtual bool UpdateControl( const Settings& );
private:
  void OnButton( wxCommandEvent& );
  void SetControlFont( const wxFont& );
  wxFont m_font;
  wxButton* m_button;
  DECLARE_EVENT_TABLE()
};

#endif
