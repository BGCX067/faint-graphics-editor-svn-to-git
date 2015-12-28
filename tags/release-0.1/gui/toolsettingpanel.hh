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

#ifndef FAINT_TOOLSETTINGPANEL_HH
#define FAINT_TOOLSETTINGPANEL_HH
#include "wx/wx.h"
#include <list>

class ToolSettingCtrl;
class ArtContainer;
class SettingNotifier;
class FaintSettings;

class ToolSettingPanel : public wxPanel {
public:
  ToolSettingPanel( wxWindow* parent, SettingNotifier&, ArtContainer& );
  void ShowSettings( const FaintSettings& );

private:
  void OnSettingChange( wxCommandEvent& );
  void AppendColour( const wxColour& );
  SettingNotifier& m_notifier;
  typedef std::list<ToolSettingCtrl*> ToolCtrlList;
  ToolCtrlList m_toolControls;
  DECLARE_EVENT_TABLE()
};

#endif
