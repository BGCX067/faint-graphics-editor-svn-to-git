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

#include "wx/gbsizer.h"
#include "wx/statline.h"
#include "gui/fontcontrol.hh"
#include "gui/imagetoggle.hh"
#include "gui/sizecontrol.hh"
#include "gui/toolsettingctrl.hh"
#include "gui/toolsettingpanel.hh"
#include "util/artcontainer.hh"
#include "util/settingid.hh"
#include "util/settings.hh"

ToolSettingPanel::ToolSettingPanel( wxWindow* parent, SettingNotifier& notifier, StatusInterface& status, ArtContainer& art ) :
  wxPanel( parent ),
  m_notifier( notifier )
{
  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  // Initial separator
  sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition ), 0, wxEXPAND|wxUP|wxDOWN, 5);

  // Various tool settings
  ToolSettingCtrl* tmp = 0;

  BoolImageToggle* editPointsToggle = new BoolImageToggle(this, ts_EditPoints, art.Get("sett_edit_points"), status);
  sizer->Add(editPointsToggle, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back(editPointsToggle);
  editPointsToggle->SetToolTip("Edit Points");

  // Fixme: The FloatSizeControl sometimes breaks, so we use a SemiFloatSizeControl for now
  tmp = new SemiFloatSizeControl(this, wxSize(50, -1), ts_LineWidth, 1, "Linewidth");
  sizer->Add(tmp, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5);
  m_toolControls.push_back(tmp);

  tmp = new IntSizeControl(this, wxSize(50, -1), ts_BrushSize, 5, "Brush Size");
  sizer->Add(tmp, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5);
  m_toolControls.push_back(tmp);

  tmp = new FontControl( this );
  sizer->Add( tmp, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back(tmp);

  tmp = new IntSizeControl(this, wxSize(50, -1), ts_FontSize, 12, "Size" );
  sizer->Add(tmp, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5);
  m_toolControls.push_back(tmp);

  ImageToggleCtrl* lineStyles = new ImageToggleCtrl( this, ts_LineStyle, wxSize(28, 15), status );
  lineStyles->AddButton(art.Get("sett_linestyle_solid"), LineStyle::SOLID, "Solid Lines");
  lineStyles->AddButton(art.Get("sett_linestyle_long_dash"), LineStyle::LONG_DASH, "Dashed lines");
  lineStyles->SetValue( LineStyle::SOLID );
  sizer->Add( lineStyles, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5  );
  m_toolControls.push_back( lineStyles );
  lineStyles->SetToolTip("Line style");

  ImageToggleCtrl* fillStyles = new ImageToggleCtrl( this, ts_FillStyle, wxSize(28, 23), status );
  fillStyles->AddButton( art.Get( "sett_fill_border"), FillStyle::BORDER, "Border Only" );
  fillStyles->AddButton( art.Get( "sett_fill_fill"), FillStyle::FILL, "Fill Only" );
  fillStyles->AddButton( art.Get( "sett_fill_border_fill"), FillStyle::BORDER_AND_FILL, "Border and Fill" );
  fillStyles->SetValue( FillStyle::BORDER );
  sizer->Add( fillStyles, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5 );
  m_toolControls.push_back ( fillStyles );
  fillStyles->SetToolTip("Fill style");

  ImageToggleCtrl* brushShape = new ImageToggleCtrl( this, ts_BrushShape, wxSize(28, 23), status );
  brushShape->AddButton( art.Get( "sett_brush_circle" ), BrushShape::CIRCLE, "Circular Brush" );
  brushShape->AddButton( art.Get( "sett_brush_rectangle" ), BrushShape::SQUARE, "Square Brush" );
  brushShape->SetValue( BrushShape::SQUARE );
  sizer->Add( brushShape, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back( brushShape );
  brushShape->SetToolTip( "Brush shape" );

  ImageToggleCtrl* arrowHead = new ImageToggleCtrl( this, ts_LineArrowHead, wxSize( 28, 23 ), status );
  arrowHead->AddButton( art.Get( "sett_line_no_arrow" ), 0, "No arrowhead" );
  arrowHead->AddButton( art.Get( "sett_line_arrow_front" ), 1, "Forward arrowhead" );
  arrowHead->SetValue( 0 );
  sizer->Add( arrowHead, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back( arrowHead );
  arrowHead->SetToolTip( "Line end style" );

  ImageToggleCtrl* transparency = new ImageToggleCtrl( this,
    ts_BackgroundStyle, wxSize(28, 23), status );
  transparency->AddButton( art.Get( "sett_transparent_bg"), BackgroundStyle::MASKED, "Transparent Background" );
  transparency->AddButton( art.Get( "sett_opaque_bg"), BackgroundStyle::SOLID, "Opaque Background" );
  sizer->Add( transparency, 0, wxALIGN_CENTER_HORIZONTAL|wxDOWN, 5 );
  m_toolControls.push_back( transparency );
  transparency->SetToolTip("Transparent or opaque background color");

  BoolImageToggle* polylineToggle = new BoolImageToggle(this, ts_PolyLine, art.Get("sett_line_polyline"),
    status);
  sizer->Add(polylineToggle, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back(polylineToggle);
  polylineToggle->SetToolTip("Polyline");

  BoolImageToggle* alphaToggle = new BoolImageToggle(this,
    ts_AlphaBlending, art.Get("sett_alpha_blend"), status);
  sizer->Add(alphaToggle, 0, wxALIGN_CENTER_HORIZONTAL | wxDOWN, 5 );
  m_toolControls.push_back(alphaToggle);
  alphaToggle->SetToolTip("Alpha blending");

  // Finishing separator
  sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition ), 0, wxEXPAND|wxUP|wxDOWN, 5);
  SetSizerAndFit(sizer);
}

void ToolSettingPanel::ShowSettings( const Settings& settings ){
  Freeze();
  for ( ToolCtrlList::iterator it = m_toolControls.begin(); it != m_toolControls.end(); ++it ){
    ToolCtrlList::value_type ctrl = *it;
    if ( ctrl->UpdateControl( settings ) ){
      ctrl->Show( true );
    }
    else {
      ctrl->Show( false );
    }
  }
  Thaw();
  Layout();
}

void ToolSettingPanel::OnSettingChange( wxCommandEvent& event ){
  ToolSettingCtrl* control( dynamic_cast<ToolSettingCtrl*>( event.GetEventObject() ) );
  control->Notify( m_notifier );
}

BEGIN_EVENT_TABLE( ToolSettingPanel, wxPanel )
EVT_COMMAND(-1, EVT_SETTING_CHANGE, ToolSettingPanel::OnSettingChange)
END_EVENT_TABLE()
