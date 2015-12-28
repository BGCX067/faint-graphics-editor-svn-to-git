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

#include "wx/wx.h"
#include "zoomctrl.hh"
#include "zoomlevel.hh"
#include <sstream>

ZoomCtrl::ZoomCtrl( wxWindow* parent )
  : wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER ),
    m_btnZoom100(0)
{
  wxSizer* hSizer = new wxBoxSizer( wxHORIZONTAL );
  m_currentZoomText = new wxStaticText( this, wxID_ANY, "#####" );
  hSizer->Add( m_currentZoomText, 1, wxALIGN_CENTER_VERTICAL );

  wxSizer* vSizer = new wxBoxSizer( wxVERTICAL );

  m_btnZoomIn = new wxButton( this, wxNewId(), "+", wxDefaultPosition, wxSize( 40, 25 ) );
  m_btnZoomIn->SetToolTip( "Zoom In" );
  vSizer->Add( m_btnZoomIn );

  m_btnZoomOut = new wxButton( this, wxNewId(), "-", wxDefaultPosition, m_btnZoomIn->GetSize() );
  m_btnZoomOut->SetToolTip( "Zoom Out" );
  vSizer->Add( m_btnZoomOut  );

  hSizer->Add( vSizer, 1, wxEXPAND );

  m_btnZoom100 = new wxButton( this, wxNewId(), "1:1", wxDefaultPosition, wxSize(60, 50) );
  m_btnZoom100->SetToolTip( "Use Actual Size" );
  hSizer->Add( m_btnZoom100,
    1,
    wxEXPAND );
  SetSizerAndFit( hSizer );
  Update( ZoomLevel(0) );
}

void ZoomCtrl::Update( const ZoomLevel& zoom ){
  std::stringstream ss;
  if ( zoom.GetScaleFactor() >= 1.0f ){
    ss << zoom.GetScaleFactor() << ":1";
  }
  else {
    ss << zoom.GetPercentage() << " %";
  }
  if ( m_currentZoomText->GetLabel() == ss.str() ){
    // Reduces flicker in MSW
    return;
  }
  m_currentZoomText->SetLabel( ss.str() );
  m_btnZoom100->Enable( zoom.GetPercentage() != 100 );
  m_btnZoomIn->Enable( !zoom.AtMax() );
  m_btnZoomOut->Enable( !zoom.AtMin() );
}

void SendEvent( ZoomCtrl* zoomControl, int eventId ){
  wxCommandEvent zoomEvent( wxEVT_COMMAND_MENU_SELECTED, eventId );
  zoomEvent.SetEventObject( zoomControl );
  zoomControl->GetEventHandler()->ProcessEvent( zoomEvent );
}

void ZoomCtrl::OnButton( wxCommandEvent& evt ){
  int id = evt.GetId();
  if ( id == m_btnZoomIn->GetId() ){
    SendEvent( this, wxID_ZOOM_IN );
  }
  else if ( id == m_btnZoomOut->GetId() ){
    SendEvent( this, wxID_ZOOM_OUT );
  }
  else if ( id == m_btnZoom100->GetId() ){
    SendEvent( this, wxID_ZOOM_100 );
  }
}

BEGIN_EVENT_TABLE( ZoomCtrl, wxPanel )
EVT_BUTTON( -1, ZoomCtrl::OnButton )
END_EVENT_TABLE()
