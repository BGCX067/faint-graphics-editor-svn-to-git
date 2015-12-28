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
#include "gui/toolbar.hh"

const IntSetting g_layerSetting;

Toolbar::Toolbar( wxWindow* parent, StatusInterface& status, ArtContainer& art )
  : wxPanel( parent ),
    m_activeButton(nullptr),
    m_status(status)
{
  wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
  m_layerChoice = new ImageToggleCtrl( this, g_layerSetting, wxSize(30, 32), status, wxHORIZONTAL, 3, 3 );
  m_layerChoice->AddButton( art.Get(Icon::LAYER_RASTER), to_int(Layer::RASTER), "Select the Raster Layer");
  m_layerChoice->AddButton( art.Get(Icon::LAYER_VECTOR), to_int(Layer::OBJECT), "Select the Object Layer"); // Fixme: Rename file to layer_object
  m_layerChoice->SetToolTip("Raster or Object layer");

  outerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition ), 0, wxEXPAND|wxDOWN, 5);
  outerSizer->Add(m_layerChoice, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5 );

  m_sizer = new wxGridSizer(0, 2, 0, 0 );
  AddTools(art);
  outerSizer->Add(m_sizer, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( outerSizer );
}

// Fixme: Rename all TOOLICON_-files to just TOOL_
void Toolbar::AddTools( ArtContainer& art ){
  AddTool( art.Get( Icon::TOOLICON_RECTSEL ),
    tooltip_t("Raster Selection"),
    description_t("Selects rectangular areas."),
    ToolId::RECTANGLE_SELECTION);
  AddTool( art.Get( Icon::TOOLICON_SELOBJECT),
    tooltip_t("Object Selection"),
    description_t("Selects and manipulates objects."),
    ToolId::OBJECT_SELECTION);
  AddTool( art.Get(Icon::TOOLICON_PEN),
    tooltip_t("Pen"),
    description_t("Draws single pixels."),
    ToolId::PEN);
  AddTool( art.Get(Icon::TOOLICON_BRUSH),
    tooltip_t("Brush"),
    description_t("Free hand drawing with selected brush shape and size."),
    ToolId::BRUSH);
  AddTool(art.Get( Icon::TOOLICON_PICKER ),
    tooltip_t("Color picker"),
    description_t("Selects a color in the image."),
    ToolId::PICKER);
  AddTool(art.Get( Icon::TOOLICON_LINE ),
    tooltip_t("Line"),
    description_t("Draws lines."),
    ToolId::LINE);
  AddTool(art.Get( Icon::TOOLICON_SPLINE ),
    tooltip_t("Spline"),
    description_t("Draws curved lines."),
    ToolId::SPLINE);
  AddTool(art.Get( Icon::TOOLICON_RECTANGLE ),
    tooltip_t("Rectangle"),
    description_t("Draws rectangles or squares."),
    ToolId::RECTANGLE);
  AddTool(art.Get( Icon::TOOLICON_ELLIPSE),
    tooltip_t("Ellipse"),
    description_t("Draws ellipses or circles."),
    ToolId::ELLIPSE);
  AddTool(art.Get( Icon::TOOLICON_POLYGON ),
    tooltip_t("Polygon"),
    description_t("Draws polygons."),
    ToolId::POLYGON);
  AddTool(art.Get( Icon::TOOLICON_TEXT ),
    tooltip_t("Text"),
    description_t("Writes text."),
    ToolId::TEXT);
  AddTool(art.Get( Icon::TOOLICON_FLOODFILL ),
    tooltip_t("Fill"),
    description_t("Fills contiguous areas."),
    ToolId::FLOOD_FILL);
}

void Toolbar::AddTool( wxBitmap bmp, const tooltip_t& toolTip, const description_t& description, ToolId id ){
  ToggleStatusButton* button = new ToggleStatusButton( this, to_int(id), wxSize(25,25), m_status, bmp, toolTip, description);
  m_idToButton[ id ] = button;
  m_sizer->Add(button);
  if ( m_activeButton == nullptr ){
    m_activeButton = button;
  }
}

void Toolbar::OnButton( wxCommandEvent& event ){
  SendToolChoiceEvent( to_tool_id(event.GetId() ) );
  event.Skip();
}

void Toolbar::OnLayerType( wxCommandEvent& event ){
  ImageToggleCtrl* eventObject = dynamic_cast<ImageToggleCtrl*>(event.GetEventObject());
  int layer = eventObject->GetValue();
  LayerChangeEvent newEvent(to_layerstyle(layer));
  ProcessEvent(newEvent);
}

void Toolbar::SendToolChoiceEvent( ToolId id ){
  ToggleStatusButton* button = m_idToButton[id];
  if ( !button->IsEnabled() ){
    return;
  }
  m_activeButton->SetValue( false );
  m_activeButton = button;
  m_activeButton->SetValue( true );

  ToolChangeEvent evt( id );
  ProcessEvent(evt);
}

void Toolbar::SendLayerChoiceEvent( Layer layer ){
  m_layerChoice->SetValue( to_int(layer) );
  m_layerChoice->SendChangeEvent();
}

BEGIN_EVENT_TABLE(Toolbar, wxPanel)
EVT_TOGGLEBUTTON( -1, Toolbar::OnButton )
EVT_COMMAND(-1, EVT_SETTING_CHANGE, Toolbar::OnLayerType)
END_EVENT_TABLE()
