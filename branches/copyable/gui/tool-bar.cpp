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

#include "wx/statline.h"
#include "gui/events.hh"
#include "gui/tool-bar.hh"

namespace faint{

const IntSetting g_layerSetting;

Toolbar::Toolbar(wxWindow* parent, StatusInterface& status, ArtContainer& art)
  : wxPanel(parent),
    m_activeButton(nullptr),
    m_status(status)
{
  wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
  m_layerChoice = new_image_toggle(this, g_layerSetting, wxSize(30, 32), status, tooltip_t("Raster or Object layer"),
    ToggleImage(art.Get(Icon::LAYER_RASTER), to_int(Layer::RASTER), "Select the Raster Layer"),
    ToggleImage(art.Get(Icon::LAYER_OBJECT), to_int(Layer::OBJECT), "Select the Object Layer"),
    wxHORIZONTAL, 3, 3);

  outerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition), 0, wxEXPAND|wxDOWN, 5);
  outerSizer->Add(m_layerChoice, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5);

  m_sizer = new wxGridSizer(0, 2, 0, 0);
  AddTools(art);
  outerSizer->Add(m_sizer, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit(outerSizer);

  Bind(wxEVT_TOGGLEBUTTON, &Toolbar::OnButton, this);
  // Fixme: Bind to layer-control?
  Bind(EVT_FAINT_INT_SETTING_CHANGE,
    [&](const SettingEvent<IntSetting>& e){
      int layer = e.GetValue();
      LayerChangeEvent newEvent(to_layerstyle(layer));
      ProcessEvent(newEvent);
    });
}

void Toolbar::AddTools(ArtContainer& art){
  AddTool(art.Get(Icon::TOOL_RECTSEL),
    tooltip_t("Raster Selection"),
    description_t("Selects rectangular areas."),
    ToolId::RECTANGLE_SELECTION);
  AddTool(art.Get(Icon::TOOL_SELOBJECT),
    tooltip_t("Object Selection"),
    description_t("Selects and manipulates objects."),
    ToolId::OBJECT_SELECTION);
  AddTool(art.Get(Icon::TOOL_PEN),
    tooltip_t("Pen"),
    description_t("Draws single pixels."),
    ToolId::PEN);
  AddTool(art.Get(Icon::TOOL_BRUSH),
    tooltip_t("Brush"),
    description_t("Free hand drawing with selected brush shape and size."),
    ToolId::BRUSH);
  AddTool(art.Get(Icon::TOOL_PICKER),
    tooltip_t("Color picker"),
    description_t("Selects a color in the image."),
    ToolId::PICKER);
  AddTool(art.Get(Icon::TOOL_LINE),
    tooltip_t("Line"),
    description_t("Draws lines."),
    ToolId::LINE);
  AddTool(art.Get(Icon::TOOL_PATH),
    tooltip_t("Path"),
    description_t("Draws paths."),
    ToolId::PATH);
  AddTool(art.Get(Icon::TOOL_RECTANGLE),
    tooltip_t("Rectangle"),
    description_t("Draws rectangles or squares."),
    ToolId::RECTANGLE);
  AddTool(art.Get(Icon::TOOL_ELLIPSE),
    tooltip_t("Ellipse"),
    description_t("Draws ellipses or circles."),
    ToolId::ELLIPSE);
  AddTool(art.Get(Icon::TOOL_POLYGON),
    tooltip_t("Polygon"),
    description_t("Draws polygons."),
    ToolId::POLYGON);
  AddTool(art.Get(Icon::TOOL_TEXT),
    tooltip_t("Text"),
    description_t("Writes text."),
    ToolId::TEXT);
  AddTool(art.Get(Icon::TOOL_FLOODFILL),
    tooltip_t("Fill"),
    description_t("Fills contiguous areas."),
    ToolId::FLOOD_FILL);
}

void Toolbar::AddTool(wxBitmap bmp, const tooltip_t& toolTip, const description_t& description, ToolId id){
  ToggleStatusButton* button = new ToggleStatusButton(this, to_int(id), wxSize(25,25), m_status, bmp, toolTip, description);
  m_idToButton[ id ] = button;
  m_sizer->Add(button);
  if (m_activeButton == nullptr){
    m_activeButton = button;
  }
}

void Toolbar::OnButton(wxCommandEvent& event){
  SendToolChoiceEvent(to_tool_id(event.GetId()));
  event.Skip();
}

void Toolbar::SendToolChoiceEvent(ToolId id){
  auto it = m_idToButton.find(id);
  if (it != end(m_idToButton)){
    ToggleStatusButton* button = it->second;
    if (!button->IsEnabled()){
      return;
    }
    m_activeButton->SetValue(false);
    m_activeButton = button;
    m_activeButton->SetValue(true);
  }
  else{
    m_activeButton->SetValue(false);
  }
  ToolChangeEvent evt(id);
  ProcessEvent(evt);
}

void Toolbar::SendLayerChoiceEvent(Layer layer){
  m_layerChoice->SetValue(to_int(layer));
  m_layerChoice->SendChangeEvent();
}

} // namespace
