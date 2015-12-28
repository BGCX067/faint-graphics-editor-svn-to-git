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
#include <map>
#include "wx/wx.h"
#include "gui/imagetoggle.hh"
#include "gui/statusbutton.hh"
#include "tools/toolid.hh"
#include "util/artcontainer.hh"
#include "util/statusinterface.hh"

class ArtContainer;

class Toolbar : public wxPanel{
public:
  Toolbar( wxWindow* parent, StatusInterface&, ArtContainer& );
  void SendLayerChoiceEvent( int layer ); // Fixme: Layer
  void SendToolChoiceEvent( ToolId );
private:
  void AddTool( wxBitmap, const tooltip_t&, const description_t&, ToolId );
  void AddTools( ArtContainer& );
  void OnButton(wxCommandEvent& );
  void OnEnter( wxMouseEvent& );
  void OnLayerType( wxCommandEvent& );
  ToggleStatusButton* m_activeButton;
  std::map< ToolId, ToggleStatusButton* > m_idToButton;
  ImageToggleCtrl* m_layerChoice;
  wxGridSizer* m_sizer;
  StatusInterface& m_status;
  DECLARE_EVENT_TABLE()
};

#endif
