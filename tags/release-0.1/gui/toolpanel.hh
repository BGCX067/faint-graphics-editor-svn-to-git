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

#ifndef FAINT_TOOLPANEL
#define FAINT_TOOLPANEL

class wxWindow;
class ToolPanelImpl;

// Vertical panel containing a layer choice control, tool selection
// buttons and controls for the settings of the active tool or
// selected objects.
class ToolPanel {
public:
  ToolPanel( wxWindow* parent, SettingNotifier&, ArtContainer& );
  ~ToolPanel();
  wxWindow* AsWindow();
  bool Visible() const;
  void Show( bool );
  void Hide();
  void SelectTool( ToolId );
  void SelectLayer( Layer );
  void ShowSettings( const FaintSettings& );
private:
  ToolPanelImpl* m_impl;
};

#endif
