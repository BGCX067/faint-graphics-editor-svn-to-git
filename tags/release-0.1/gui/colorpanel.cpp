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
#include "palettectrl.hh"
#include "selectedcolorctrl.hh"
#include "colorpanel.hh"

class ColorPanelImpl : public wxPanel {
public:
  ColorPanelImpl( wxWindow* parent, const FaintSettings& settings, SettingNotifier& notifier, PaletteContainer& palettes )
    : wxPanel(parent)
  {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedColor = new SelectedColorCtrl( this, notifier );
    sizer->Add( m_selectedColor, 0, wxALL, 5 ); // Fixme: Document. Use border-width constant

    m_palette = new PaletteCtrl( this, settings, notifier, palettes );
    sizer->Add( m_palette, 0, wxALL | wxEXPAND | wxSHRINK, 5 ); // Fixme: Document. Use border-width constant.

    m_zoom = new ZoomCtrl( this );
    sizer->Add( m_zoom, 0, wxALL, 5 );

    SetSizer( sizer );
    Layout();
  }
  ZoomCtrl* m_zoom;
  SelectedColorCtrl* m_selectedColor;
  PaletteCtrl* m_palette;
};

ColorPanel::ColorPanel( wxWindow* parent, const FaintSettings& settings, SettingNotifier& notifier, PaletteContainer& palettes ){
  m_impl = new ColorPanelImpl( parent, settings, notifier, palettes );
}

ColorPanel::~ColorPanel(){
  // Note: deletion is handled by wxWidgets.
  m_impl = 0;
}

wxWindow* ColorPanel::AsWindow(){
  return m_impl;
}

bool ColorPanel::Visible() const{
  return m_impl->IsShown();
}

void ColorPanel::Show( bool show ){
  m_impl->Show( show );
}

void ColorPanel::Hide(){
  Show(false);
}

void ColorPanel::AddToPalette( const faint::Color& c ){
  m_impl->m_palette->AddColor(c);
  m_impl->Layout();
}

void ColorPanel::UpdateZoom( const ZoomLevel& zoom ){
  m_impl->m_zoom->Update( zoom );
}

void ColorPanel::UpdateSelectedColors( const ColorChoice& c ){
  m_impl->m_selectedColor->Update( c.fg, c.bg );
}

void ColorPanel::Freeze(){
  m_impl->Freeze();
}

void ColorPanel::Thaw(){
  m_impl->Thaw();
}
