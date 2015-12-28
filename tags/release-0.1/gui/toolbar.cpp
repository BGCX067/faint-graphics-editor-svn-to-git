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

#include "toolbar.hh"
#include "wx/statline.h"
#include "tools/settingid.hh"
#include "getappcontext.hh"
#include "imagetoggle.hh"

const IntSetting layer;

Toolbar::Toolbar( wxWindow* parent, ArtContainer& art )
  : wxPanel( parent ),
    m_activeButton(0)
{
  wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
  layerChoice = new ImageToggleCtrl( this, layer, wxSize(30, 32), wxHORIZONTAL, 3, 3 );

  layerChoice->AddButton( art.Get("layer_raster"), LAYER_RASTER);
  layerChoice->AddButton( art.Get("layer_object"), LAYER_OBJECT);
  layerChoice->SetToolTip("Raster or Vector layer");

  outerSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition ), 0, wxEXPAND|wxDOWN, 5);
  outerSizer->Add(layerChoice, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5 );

  sizer = new wxGridSizer(0, 2, 0, 0 );

  AddTools(art);
  outerSizer->Add(sizer, 0, wxALIGN_CENTER_HORIZONTAL);
  SetSizerAndFit( outerSizer );
}

void Toolbar::AddTools( ArtContainer& art ){
  AddTool( art.Get( "toolicon_rectsel" ),
    "Rectangular Select",
    "Selects rectangular areas.",
    T_RECT_SEL);
  AddTool( art.Get("toolicon_selobject"),
    "Object selection",
    "Selects and manipulates objects.",
    T_OBJ_SEL);
  AddTool( art.Get("toolicon_pen"),
    "Pen",
    "Draws single pixels.",
    T_PEN);
  AddTool( art.Get("toolicon_brush"),
    "Brush",
    "Free hand drawing with selected brush shape and size.",
    T_BRUSH);
  AddTool(art.Get( "toolicon_picker"),
    "Color picker",
    "Selects a color in the image.",
    T_PICKER);
  AddTool(art.Get("toolicon_line"),
    "Line",
    "Draws lines.",
    T_LINE);
  AddTool(art.Get("toolicon_spline"),
    "Spline",
    "Draws curved lines.",
    T_SPLINE);
  AddTool(art.Get("toolicon_rectangle"),
    "Rectangle",
    "Draws rectangles or squares.",
    T_RECTANGLE);
  AddTool(art.Get("toolicon_ellipse"),
    "Ellipse",
    "Draws ellipses or circles.",
    T_ELLIPSE);
  AddTool(art.Get("toolicon_polygon"),
    "Polygon",
    "Draws polygons.",
    T_POLYGON);
  AddTool(art.Get("toolicon_text"),
    "Text",
    "Writes text.",
    T_TEXT);
  AddTool(art.Get("toolicon_floodfill"),
    "Fill",
    "Fills contiguous areas.",
    T_FLOODFILL);
}

void Toolbar::Enable( ToolId id ){
  idToButton[ id ]->Enable();
}

void Toolbar::Disable( ToolId id ){
  idToButton[ id ]->Disable();
}

void Toolbar::AddTool( wxBitmap* bmp, wxString toolTip, wxString description, ToolId id ){
  bmp->SetMask( new wxMask( *bmp, wxColour(255,0,255)) );
  ToolButton* btn = new ToolButton( this, bmp, id, toolTip, description);
  idToButton[ id ] = btn;
  sizer->Add(btn);
  if ( m_activeButton == 0 ){
    m_activeButton = btn;
  }
}

void Toolbar::OnClick( wxCommandEvent& event ){
  SendToolChoiceEvent( ToolId( event.GetId() ) );
  event.Skip();
}

void Toolbar::OnLayerType( wxCommandEvent& event ){
  wxCommandEvent newEvent( EVT_LAYER_TYPE_CHANGE, wxID_ANY);  
  ImageToggleCtrl* eventObject = dynamic_cast<ImageToggleCtrl*>(event.GetEventObject()); // Fixme: Nasty, dumb cast, use specialized events
  int layer = eventObject->GetValue();
  newEvent.SetInt( layer );
  GetEventHandler()->ProcessEvent( newEvent );
}

void Toolbar::SendToolChoiceEvent( ToolId id ){
  ToolButton* button = idToButton[id];
  if ( !button->IsEnabled() ){
    return;
  }
  m_activeButton->SetValue( false );
  m_activeButton = button;
  m_activeButton->SetValue( true );
  wxCommandEvent newEvent( EVT_TOOL_CHANGE, id );
  newEvent.SetEventObject( this );
  GetEventHandler()->ProcessEvent( newEvent );
}

void Toolbar::SendLayerChoiceEvent( int layer ){
  layerChoice->SetValue( layer );
  layerChoice->SendChangeEvent();
}

// Added wxWANTS_CHARS to style to get rid of msw-pling on keypress when button has focus
ToolButton::ToolButton( wxWindow* parent, wxBitmap* bitmap, int id_, wxString toolTip, wxString description )
  : wxBitmapToggleButton( parent, id_, *bitmap, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
    m_description( description )
{
  SetToolTip( toolTip );
  SetMinSize( wxSize( 25, 25 ) );
}

void ToolButton::OnLeave( wxMouseEvent& event ){
  GetAppContext().GetStatusInfo().SetMainText("");
  event.Skip();
}

void ToolButton::OnMotion( wxMouseEvent& event ){
  if ( GetValue() ){
    // No description for selected tool
    return;
  }
  // Set the status bar description.
  // Done in EVT_MOTION-handler because it did not work well in
  // EVT_ENTER (the enter event appears to be missed for example when
  // moving between buttons)
  GetAppContext().GetStatusInfo().SetMainText( std::string( m_description ) );
  event.Skip();
}

void ToolButton::OnKeyDown( wxKeyEvent& ){
  // Added to prevent invalid-key sound when button has focus
}

BEGIN_EVENT_TABLE( ToolButton, wxBitmapToggleButton )
EVT_LEAVE_WINDOW( ToolButton::OnLeave )
EVT_MOTION( ToolButton::OnMotion )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(Toolbar, wxPanel)
EVT_TOGGLEBUTTON( -1, Toolbar::OnClick )
EVT_COMMAND(-1, EVT_SETTING_CHANGE, Toolbar::OnLayerType)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE( EVT_TOOL_CHANGE )
DEFINE_EVENT_TYPE( EVT_LAYER_TYPE_CHANGE )
