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
#include <sstream>
#include "gui/events.hh"
#include "gui/statusbutton.hh"
#include "gui/zoomctrl.hh"
#include "util/statusinterface.hh"
#include "util/zoomlevel.hh"

#ifdef __WXMSW__
#define ZOOMCONTROL_BORDER_STYLE wxBORDER_THEME
#else
#define ZOOMCONTROL_BORDER_STYLE wxBORDER_NONE
#endif

void send_zoom_event( ZoomCtrlImpl* zoomControl, int eventId );

class ZoomCtrlImpl : public wxPanel {
public:
  ZoomCtrlImpl( wxWindow* parent, StatusInterface& status )
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|ZOOMCONTROL_BORDER_STYLE ),
      m_btnZoomFitOr100(nullptr),
      m_btnZoomIn(nullptr),
      m_btnZoomOut(nullptr),
      m_currentZoomText(nullptr),
      m_fit(false)
  {
    wxSizer* hSizer = new wxBoxSizer( wxHORIZONTAL );
    m_currentZoomText = new wxStaticText( this, wxID_ANY, "####", wxDefaultPosition, wxSize(40,-1), wxALIGN_CENTRE | wxST_NO_AUTORESIZE );
    hSizer->Add( m_currentZoomText, 0, wxALIGN_CENTER_VERTICAL );

    wxSizer* vSizer = new wxBoxSizer( wxVERTICAL );

    // wxWANTS_CHARS prevents sound on keypress when key has focus
    m_btnZoomIn = new StatusButton( this, wxSize(40,25), status, "+", tooltip_t("Zoom In"), description_t("Click to Zoom In, Ctrl=All Images") );
    vSizer->Add( m_btnZoomIn );

    m_btnZoomOut = new StatusButton( this, m_btnZoomIn->GetSize(), status, "-", tooltip_t("Zoom Out"), description_t("Click to Zoom Out, Ctrl=All Images") );
    vSizer->Add( m_btnZoomOut  );

    hSizer->Add( vSizer, 0, wxEXPAND );

    m_btnZoomFitOr100 = new StatusButton( this, wxSize(60,50), status, "1:1", tooltip_t(""), description_t("") );
    hSizer->Add( m_btnZoomFitOr100,
      1,
      wxEXPAND );
    SetSizerAndFit( hSizer );
    UpdateZoom( ZoomLevel() );
  }

  void UpdateZoom( const ZoomLevel& zoom ){
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
    if ( zoom.GetPercentage() == 100 ){
      m_btnZoomFitOr100->UpdateText("Fit", tooltip_t("Zoom to Fit"), description_t("Click to fit image in view, Ctrl=All images") );
      m_fit = true;
    }
    else {
      m_btnZoomFitOr100->UpdateText("1:1", tooltip_t("Use Actual Size"), description_t("Click to show image at 1:1, Ctrl=All images"));
      m_fit = false;
    }
    m_btnZoomIn->Enable( !zoom.AtMax() );
    m_btnZoomOut->Enable( !zoom.AtMin() );
  }
private:
  void OnButton( wxCommandEvent& evt ){
    int id = evt.GetId();
    bool ctrl = wxGetKeyState(WXK_CONTROL);
    if ( id == m_btnZoomIn->GetId() ){
      send_zoom_event( this, ctrl ? ID_ZOOM_IN_ALL : ID_ZOOM_IN );
    }
    else if ( id == m_btnZoomOut->GetId() ){
      send_zoom_event( this, ctrl ? ID_ZOOM_OUT_ALL : ID_ZOOM_OUT );
    }
    else if ( id == m_btnZoomFitOr100->GetId() ){
      if ( m_fit ){
	send_zoom_event( this, ctrl ? ID_ZOOM_FIT_ALL : ID_ZOOM_FIT );
      }
      else{
	send_zoom_event( this, ctrl ? ID_ZOOM_100_ALL : ID_ZOOM_100 );
      }
    }
  }

  StatusButton* m_btnZoomFitOr100;
  StatusButton* m_btnZoomIn;
  StatusButton* m_btnZoomOut;
  wxStaticText* m_currentZoomText;
  bool m_fit;
  DECLARE_EVENT_TABLE()
};

void send_zoom_event( ZoomCtrlImpl* zoomControl, int eventId ){
  wxCommandEvent zoomEvent( wxEVT_COMMAND_MENU_SELECTED, eventId );
  zoomEvent.SetEventObject( zoomControl );
  zoomControl->GetEventHandler()->ProcessEvent( zoomEvent );
}

BEGIN_EVENT_TABLE( ZoomCtrlImpl, wxPanel )
EVT_BUTTON( -1, ZoomCtrlImpl::OnButton )
END_EVENT_TABLE()

ZoomCtrl::ZoomCtrl( wxWindow* parent, StatusInterface& status )
  : m_impl(nullptr)
{
  m_impl = new ZoomCtrlImpl(parent, status);
}

ZoomCtrl::~ZoomCtrl(){
  m_impl = nullptr; // Deletion is handled by wxWidgets.
}

wxWindow* ZoomCtrl::AsWindow(){
  return m_impl;
}

void ZoomCtrl::UpdateZoom( const ZoomLevel& zoom ){
  m_impl->UpdateZoom(zoom);
}
