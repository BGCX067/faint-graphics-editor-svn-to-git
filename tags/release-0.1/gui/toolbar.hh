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

#ifndef FAINT_TOOLBAR_HH
#define FAINT_TOOLBAR_HH
#include "wx/wx.h"
#include "artcontainer.hh"
#include "gui/imagetoggle.hh"
#include "wx/tglbtn.h"
#include "toolid.hh"
#include <map>
DECLARE_EVENT_TYPE( EVT_TOOL_CHANGE, -1 )
DECLARE_EVENT_TYPE( EVT_LAYER_TYPE_CHANGE, -1 )

class ToolButton : public wxBitmapToggleButton {
public:
  ToolButton( wxWindow* parent, wxBitmap*, int id, wxString toolTip ="", wxString description = "" );
  void OnMotion( wxMouseEvent& );
  void OnLeave( wxMouseEvent& );
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  void OnKeyDown( wxKeyEvent& );
private:
  wxString m_description;
  DECLARE_EVENT_TABLE()
};

class Toolbar : public wxPanel{
public:
  Toolbar( wxWindow* parent, ArtContainer& );
  void OnClick(wxCommandEvent& );
  void OnEnter( wxMouseEvent& );
  void OnLayerType( wxCommandEvent& );
  void SendToolChoiceEvent( ToolId );
  void SendLayerChoiceEvent( int layer );
  void Enable( ToolId );
  void Disable( ToolId );
private:
  void AddTool( wxBitmap*, wxString, wxString, ToolId );
  void AddTools( ArtContainer& );
  wxGridSizer* sizer;
  ImageToggleCtrl* layerChoice;
  std::map< ToolId, ToolButton* > idToButton;
  ToolButton* m_activeButton;
  DECLARE_EVENT_TABLE()
};

#endif
