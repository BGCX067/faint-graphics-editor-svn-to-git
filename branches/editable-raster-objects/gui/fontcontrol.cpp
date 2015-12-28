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
#include "wx/fontdlg.h"
#include "app/getappcontext.hh"
#include "gui/fontcontrol.hh"
#include "util/guiutil.hh"
#include "util/settingid.hh"

FontControl::FontControl( wxWindow* parent )
  : ToolSettingCtrl( parent )
{
  wxBoxSizer* vSizer = new wxBoxSizer( wxVERTICAL );
  wxBoxSizer* hSizer = new wxBoxSizer( wxHORIZONTAL );
  m_button = new wxButton( this, wxID_ANY, "Font", wxDefaultPosition, wxSize( 50, -1 ) );
  hSizer->Add( m_button );
  vSizer->Add( hSizer );
  SetSizerAndFit( vSizer );
}

StrSetting FontControl::GetSetting() const {
  return ts_FontFace;
}

UntypedSetting FontControl::GetUntypedSetting() const {
  return UntypedSetting( ts_FontFace );
}

std::string FontControl::GetValue() const {
  return "comic sans ms";
}

void FontControl::Notify( SettingNotifier& ){
}

void FontControl::OnButton( wxCommandEvent& ){
  wxFontData initial;
  initial.SetInitialFont( m_font );
  initial.EnableEffects( false );
  wxFontDialog dlg( GetParent(), initial );
  if ( faint::show_modal(dlg) == wxID_OK ){
    // Fixme: Use events like the IntSettingControl and the FloatSettingControl
    wxFontData& data( dlg.GetFontData() );
    wxFont f( data.GetChosenFont() );
    AppContext& app = GetAppContext();
    app.Set( ts_FontFace, std::string( f.GetFaceName() ) );
    app.Set( ts_FontSize, f.GetPointSize() );
    app.Set( ts_FontBold, f.GetWeight() == wxFONTWEIGHT_BOLD );
    app.Set( ts_FontItalic, f.GetStyle() == wxFONTSTYLE_ITALIC );
  }
}

void FontControl::SetControlFont( const wxFont& f ){
  m_font = f;
  m_button->SetToolTip( f.GetFaceName() );
}

void FontControl::SetValue( const std::string& ) {
}

bool FontControl::UpdateControl( const Settings& s ){
  if ( !s.Has( ts_FontFace ) ){
    return false;
  }

  const std::string face = s.Get( ts_FontFace );
  const int size = s.Get( ts_FontSize );
  const bool bold = s.Get( ts_FontBold );
  const bool italic = s.Get( ts_FontItalic );
  const bool underline = false;
  int ignoreFamily( wxFONTFAMILY_DEFAULT ); // Font family is ignored when a face name is specified
  SetControlFont( wxFont( size, ignoreFamily,
      italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
      bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
      underline,
      face ) );
  return true;
}

BEGIN_EVENT_TABLE(FontControl, ToolSettingCtrl )
EVT_BUTTON( -1, FontControl::OnButton )
END_EVENT_TABLE()
