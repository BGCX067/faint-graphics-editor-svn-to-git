// -*- coding: us-ascii-unix -*-
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

#ifndef FAINT_TOOL_BAR_HH
#define FAINT_TOOL_BAR_HH
#include <map>
#include "wx/panel.h"
#include "wx/sizer.h"
#include "gui/image-toggle-ctrl.hh"
#include "gui/status-button.hh"
#include "tools/tool-id.hh"
#include "util/art-container.hh"
#include "util/setting-id.hh"
#include "util/status-interface.hh"

namespace faint{

class Toolbar : public wxPanel{
public:
  Toolbar(wxWindow* parent, StatusInterface&, ArtContainer&);
  void SendLayerChoiceEvent(Layer);
  void SendToolChoiceEvent(ToolId);
private:
  void AddTool(wxBitmap, const tooltip_t&, const description_t&, ToolId);
  void AddTools(ArtContainer&);
  void OnButton(wxCommandEvent&);
  void OnEnter(wxMouseEvent&);
  ToggleStatusButton* m_activeButton;
  std::map< ToolId, ToggleStatusButton* > m_idToButton;
  IntSettingCtrl* m_layerChoice;
  wxGridSizer* m_sizer;
  StatusInterface& m_status;
};

} // namespace

#endif
