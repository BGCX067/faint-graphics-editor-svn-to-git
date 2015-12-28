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
#include "gui/imagetoggle.hh"
#include "gui/toolbar.hh"

const IntSetting g_layerSetting;

Toolbar::Toolbar( wxWindow* parent, StatusInterface& status, ArtContainer& art )
  : wxPanel( parent ),
    m_activeButton(0),
    m_status(status)
{
  wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
  m_layerChoice = new ImageToggleCtrl( this, g_layerSetting, wxSize(30, 32), status, wxHORIZONTAL, 3, 3 );
  m_layerChoice->AddButton( art.Get("layer_raster"), Layer::RASTER, "Select the Raster Layer");
  m_layerChoice->AddButton( art.Get("layer_object"), Layer::OBJECT, "Select the Object Layer");
  m_layerChoice->SetToolTip("Raster or Object layer");

  outerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition ), 0, wxEXPAND|wxDOWN, 5);
  outerSizer->Add(m_layerChoice, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5 );

  m_sizer = new wxGridSizer(0, 2, 0, 0 );
  AddTools(art);
  outerSizer->Add(m_sizer, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( outerSizer );
}

void Toolbar::AddTools( ArtContainer& art ){
  AddTool( art.Get( "toolicon_rectsel" ),
    tooltip_t("Raster Selection"),
    description_t("Selects rectangular areas."),
    T_RECT_SEL);
  AddTool( art.Get("toolicon_selobject"),
    tooltip_t("Object Selection"),
    description_t("Selects and manipulates objects."),
    T_OBJ_SEL);
  AddTool( art.Get("toolicon_pen"),
    tooltip_t("Pen"),
    description_t("Draws single pixels."),
    T_PEN);
  AddTool( art.Get("toolicon_brush"),
    tooltip_t("Brush"),
    description_t("Free hand drawing with selected brush shape and size."),
    T_BRUSH);
  AddTool(art.Get( "toolicon_picker"),
    tooltip_t("Color picker"),
    description_t("Selects a color in the image."),
    T_PICKER);
  AddTool(art.Get("toolicon_line"),
    tooltip_t("Line"),
    description_t("Draws lines."),
    T_LINE);
  AddTool(art.Get("toolicon_spline"),
    tooltip_t("Spline"),
    description_t("Draws curved lines."),
    T_SPLINE);
  AddTool(art.Get("toolicon_rectangle"),
    tooltip_t("Rectangle"),
    description_t("Draws rectangles or squares."),
    T_RECTANGLE);
  AddTool(art.Get("toolicon_ellipse"),
    tooltip_t("Ellipse"),
    description_t("Draws ellipses or circles."),
    T_ELLIPSE);
  AddTool(art.Get("toolicon_polygon"),
    tooltip_t("Polygon"),
    description_t("Draws polygons."),
    T_POLYGON);
  AddTool(art.Get("toolicon_text"),
    tooltip_t("Text"),
    description_t("Writes text."),
    T_TEXT);
  AddTool(art.Get("toolicon_floodfill"),
    tooltip_t("Fill"),
    description_t("Fills contiguous areas."),
    T_FLOODFILL);
}

void Toolbar::AddTool( wxBitmap bmp, const tooltip_t& toolTip, const description_t& description, ToolId id ){
  ToggleStatusButton* button = new ToggleStatusButton( this, id, wxSize(25,25), m_status, bmp, toolTip, description);
  m_idToButton[ id ] = button;
  m_sizer->Add(button);
  if ( m_activeButton == 0 ){
    m_activeButton = button;
  }
}

void Toolbar::OnButton( wxCommandEvent& event ){
  SendToolChoiceEvent( ToolId( event.GetId() ) );
  event.Skip();
}

void Toolbar::OnLayerType( wxCommandEvent& event ){
  ImageToggleCtrl* eventObject = dynamic_cast<ImageToggleCtrl*>(event.GetEventObject());
  int layer = eventObject->GetValue();
  LayerChangeEvent newEvent(to_layer(layer));
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

void Toolbar::SendLayerChoiceEvent( int layer ){
  m_layerChoice->SetValue( layer );
  m_layerChoice->SendChangeEvent();
}

BEGIN_EVENT_TABLE(Toolbar, wxPanel)
EVT_TOGGLEBUTTON( -1, Toolbar::OnButton )
EVT_COMMAND(-1, EVT_SETTING_CHANGE, Toolbar::OnLayerType)
END_EVENT_TABLE()
