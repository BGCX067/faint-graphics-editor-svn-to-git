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
#include "gui/colorpanel.hh"
#include "gui/events.hh"
#include "gui/framectrl.hh"
#include "gui/gridctrl.hh"
#include "gui/palettectrl.hh"
#include "gui/selectedcolorctrl.hh"
#include "gui/zoomctrl.hh"
#include "util/artcontainer.hh"
#include "util/statusinterface.hh"

class ColorPanelImpl : public wxPanel {
public:
  ColorPanelImpl( wxWindow* parent, const Settings& settings, SettingNotifier& notifier, PaletteContainer& palettes, StatusInterface& status, const ArtContainer& art )
    : wxPanel(parent)
  {
    // The spacing between controls in this panel
    const int spacing = 5;

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedColor = new SelectedColorCtrl( this, notifier, status );
    sizer->Add( m_selectedColor, 0, wxALL, spacing );

    m_palette = new PaletteCtrl( this, settings, notifier, palettes, status );
    sizer->Add( m_palette, 0, wxALL | wxEXPAND | wxSHRINK, spacing );

    m_zoom = new ZoomCtrl( this, status );
    sizer->Add( m_zoom->AsWindow(), 0, wxALL, spacing );

    m_grid = new GridCtrl( this, art );
    sizer->Add( m_grid, 0, wxALL, spacing );

    m_frameCtrl = new FrameControl( this, art );
    sizer->Add( m_frameCtrl->AsWindow(), 0, wxALL, spacing );
    SetSizer( sizer );
    Layout();

    // Handle resizing child controls (e.g. color added to palette
    // or grid-control expanded).
    GetEventHandler()->Connect(wxID_ANY,
      ID_CONTROL_RESIZED,
      wxCommandEventHandler(ColorPanelImpl::OnChildResized),
      nullptr,
      this);
  }
  ~ColorPanelImpl(){
    delete m_zoom; // wxWrapper
  }

  void OnChildResized(wxCommandEvent&){
    Layout();
    Refresh();
  }

  ZoomCtrl* m_zoom;
  GridCtrl* m_grid;
  SelectedColorCtrl* m_selectedColor;
  PaletteCtrl* m_palette;
  FrameControl* m_frameCtrl;
};

ColorPanel::ColorPanel( wxWindow* parent, const Settings& settings, SettingNotifier& notifier, PaletteContainer& palettes, StatusInterface& status, const ArtContainer& art ){
  m_impl = new ColorPanelImpl( parent, settings, notifier, palettes, status, art );
}

ColorPanel::~ColorPanel(){
  m_impl = nullptr; // Deletion is handled by wxWidgets.
}

void ColorPanel::AddToPalette( const faint::DrawSource& src ){
  m_impl->m_palette->Add(src);
}

wxWindow* ColorPanel::AsWindow(){
  return m_impl;
}

void ColorPanel::Freeze(){
  m_impl->Freeze();
}

void ColorPanel::Hide(){
  Show(false);
}

void ColorPanel::SetPalette( const faint::DrawSourceMap& drawSourceMap ){
  m_impl->m_palette->SetPalette(drawSourceMap);
}

void ColorPanel::Show( bool show ){
  m_impl->Show( show );
}

void ColorPanel::Thaw(){
  m_impl->Thaw();
}

void ColorPanel::UpdateFrames( const index_t& numFrames ){
  m_impl->m_frameCtrl->SetNumFrames(numFrames);
  m_impl->Layout();
  m_impl->Refresh();
}

void ColorPanel::UpdateGrid(){
  m_impl->m_grid->Update();
}

void ColorPanel::UpdateSelectedColors( const ColorChoice& c ){
  m_impl->m_selectedColor->UpdateColors( c.fg, c.bg );
}

void ColorPanel::UpdateZoom( const ZoomLevel& zoom ){
  m_impl->m_zoom->UpdateZoom( zoom );
}

bool ColorPanel::Visible() const{
  return m_impl->IsShown();
}
