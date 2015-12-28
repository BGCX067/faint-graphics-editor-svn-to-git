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

#include <vector>
#include "wx/combo.h"
#include "wx/listctrl.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "geo/int-size.hh"
#include "gui/art-container.hh"
#include "gui/events.hh"
#include "gui/image-toggle-ctrl.hh"
#include "gui/setting-events.hh"
#include "gui/status-button.hh"
#include "gui/tool-bar.hh"
#include "gui/tool-drop-down-button.hh"
#include "util/setting-id.hh"
#include "util-wx/convert-wx.hh"

#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "util/color-bitmap-util.hh"

namespace faint{

const wxSize g_toolButtonSize(25,25);

const IntSetting g_layerSetting;

// Create the shaded "selected" tool graphic
static wxBitmap create_selected_bitmap(const wxBitmap& image){
  auto bg(with_border(Bitmap(to_faint(image.GetSize()), Color(113,152,207,209)),
    Color(113,153,208,115), LineStyle::SOLID));
  blend(at_top_left(to_faint(image)), onto(bg));
  return to_wx_bmp(bg);
}

class ToolbarImpl : public wxPanel{
public:
  ToolbarImpl(wxWindow* parent, StatusInterface& status,
    ArtContainer& art)
    : wxPanel(parent),
      m_activeButton(nullptr),
      m_groupButton(nullptr),
      m_status(status)
  {
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    m_layerChoice = new_image_toggle(this, g_layerSetting,
      IntSize(30, 32),
      status,
      tooltip_t("Raster or Object layer"),
      ToggleImage(art.Get(Icon::LAYER_RASTER), to_int(Layer::RASTER),
        "Select the Raster Layer"),
      ToggleImage(art.Get(Icon::LAYER_OBJECT), to_int(Layer::OBJECT),
        "Select the Object Layer"),
      Axis::HORIZONTAL,
      IntSize(3, 3));

    outerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition), 0,
      wxEXPAND|wxDOWN, 5);
    outerSizer->Add(m_layerChoice, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5);

    m_sizer = new wxGridSizer(0, 2, 0, 0);
    AddTools(art);
    outerSizer->Add(m_sizer, 0, wxALIGN_CENTER_HORIZONTAL);
    SetSizerAndFit(outerSizer);

    Bind(wxEVT_TOGGLEBUTTON,
      [&](wxCommandEvent& event){
        if (event.GetEventObject() != m_groupButton){
          SendToolChoiceEvent(to_tool_id(event.GetId()));
        }
        event.Skip();
      });

    Bind(EVT_FAINT_INT_SETTING_CHANGE,
      [&](const SettingEvent<IntSetting>& e){
        int layer = e.GetValue();
        LayerChangeEvent newEvent(to_layerstyle(layer));
        ProcessEvent(newEvent);
      });
  }

  void SendToolChoiceEvent(ToolId id){
    auto it = m_idToButton.find(id);
    if (it != end(m_idToButton)){
      wxBitmapToggleButton* button = it->second;
      if (!button->IsEnabled()){
        return;
      }
      m_activeButton->SetValue(false);
      m_activeButton = button;
      m_activeButton->SetValue(true);
    }
    else{
      m_activeButton->SetValue(false);
      m_activeButton = m_groupButton;
      m_activeButton->SetValue(true);
    }
    ToolChangeEvent evt(id);
    ProcessEvent(evt);
  }

  void SendLayerChoiceEvent(Layer layer){
    m_layerChoice->SetValue(to_int(layer));
    m_layerChoice->SendChangeEvent();
  }

private:
  void AddTool(const wxBitmap& bmpInactive,
    const tooltip_t& toolTip,
    const description_t& description,
    ToolId id)
  {
    wxBitmap bmpActive(create_selected_bitmap(bmpInactive));
    ToggleStatusButton* button = new ToggleStatusButton(this,
      to_int(id),
      g_toolButtonSize,
      m_status,
      bmpInactive,
      bmpActive,
      toolTip,
      description);
    m_idToButton[id] = button;
    m_sizer->Add(button);
    if (m_activeButton == nullptr){
      m_activeButton = button;
    }
  }

  wxBitmapToggleButton* AddToolGroup(const std::vector<ToolInfo>& tools){
    auto* button = tool_drop_down_button(this,
      to_faint(g_toolButtonSize), tools);
    m_sizer->Add(button);
    button->Bind(EVT_FAINT_TOOL_CHANGE,
      [&](ToolChangeEvent& event){
        SendToolChoiceEvent(event.GetTool());
      });
    return button;
  }

  void AddTools(ArtContainer& art){
    AddTool(art.Get(Icon::TOOL_SELECTION),
      tooltip_t("Selection"),
      description_t("Adjusts the selection."),
      ToolId::SELECTION);
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

    m_groupButton = AddToolGroup({
        { art.Get(Icon::TOOL_CALIBRATE),
          create_selected_bitmap(art.Get(Icon::TOOL_CALIBRATE)),
          tooltip_t("Calibrate"),
          description_t("Defines image measurements."),
          ToolId::CALIBRATE},
        { art.Get(Icon::TOOL_LEVEL),
          create_selected_bitmap(art.Get(Icon::TOOL_LEVEL)),
          tooltip_t("Level"),
          description_t("Image alignment"),
          ToolId::LEVEL},
        { art.Get(Icon::TOOL_HOT_SPOT),
          create_selected_bitmap(art.Get(Icon::TOOL_HOT_SPOT)),
          tooltip_t("Hot spot"),
          description_t("Sets the image hot spot."),
          ToolId::HOT_SPOT}});

    AddTool(art.Get(Icon::TOOL_FLOODFILL),
      tooltip_t("Fill"),
      description_t("Fills contiguous areas."),
      ToolId::FLOOD_FILL);
  }

private:
  wxBitmapToggleButton* m_activeButton;
  wxBitmapToggleButton* m_groupButton;
  std::map<ToolId, wxBitmapToggleButton*> m_idToButton;
  IntSettingCtrl* m_layerChoice;
  wxGridSizer* m_sizer;
  StatusInterface& m_status;
};

Toolbar::Toolbar(wxWindow* parent, StatusInterface& status, ArtContainer& art){
  m_impl = make_dumb<ToolbarImpl>(parent, status, art);
}

wxWindow* Toolbar::AsWindow(){
  return m_impl.get();
}

void Toolbar::SendLayerChoiceEvent(Layer layer){
  m_impl->SendLayerChoiceEvent(layer);
}

void Toolbar::SendToolChoiceEvent(ToolId toolId){
  m_impl->SendToolChoiceEvent(toolId);
}

} // namespace
