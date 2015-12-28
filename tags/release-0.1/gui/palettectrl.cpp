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

#include "palettectrl.hh"
#include "wx/sizer.h"
#include "wx/dir.h"
#include "wx/colordlg.h"
#include "colordataobject.hh"
#include "util/convertwx.hh"
#include "dropsource.hh"
#include "getappcontext.hh"
#include "paletteparser.hh"
#include "tools/settingid.hh"
#include "util/colorutil.hh"
#include "util/formatting.hh"
#include "util/util.hh"

DECLARE_EVENT_TYPE(EVT_REMOVE_COLOR, -1)
DEFINE_EVENT_TYPE(EVT_REMOVE_COLOR)

PaletteCtrl::PaletteCtrl( wxWindow* parent, const FaintSettings& s, SettingNotifier& n, PaletteContainer& palettes)
: wxPanel( parent, wxID_ANY ),
  ColorDropTarget( this ),
  m_notifier( n ),
  m_settings( s )
{
  m_sizer = new wxGridSizer( 2, 0, 0, 0 );
  std::vector<faint::Color>& pal = palettes["default"];
  for ( unsigned int i = 0; i!= pal.size(); i++ ){
    AddColor( pal[i] );
  }
  SetSizer( m_sizer );
}

wxDragResult PaletteCtrl::OnDropColor( const wxPoint& pos, const faint::Color& color ){
  // Fixme: Insert, move or copy the color depending on
  // source etc. - Also: Parents must resize
  AddColor( color );
  return wxDragMove;
}

void PaletteCtrl::AddColor( const faint::Color& color ){
  m_sizer->Add( new ColorButton( this, color ), 0, wxRIGHT | wxDOWN, 1 );
}

// Fixme: Move this - use events to communicate this
void PaletteCtrl::SetFg(const faint::Color& color ){
  m_prevFg = m_settings.Get( ts_FgCol );
  m_notifier.Notify( ts_FgCol, color );
}

void PaletteCtrl::UndoSetFg(){
  m_notifier.Notify( ts_FgCol, m_prevFg  );

}

void PaletteCtrl::SetBg(const faint::Color& color){
  m_notifier.Notify( ts_BgCol, color );
}

void PaletteCtrl::OnRemoveColor( wxCommandEvent& event ){
  wxWindow* window = dynamic_cast<wxWindow*>( event.GetEventObject() );
  m_sizer->Detach( window );
  delete window;
}

BEGIN_EVENT_TABLE(PaletteCtrl, wxWindow)
EVT_COMMAND( -1, EVT_REMOVE_COLOR, PaletteCtrl::OnRemoveColor )
END_EVENT_TABLE()

ColorButton::ColorButton( wxWindow* parent, const faint::Color& color )
: wxWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE ),
  m_bitmap(0)
{
  SetSize(20, 20);
  SetInitialSize(wxSize( 20, 20 ) );
  SetColor( color );
  SetBackgroundColour( to_wx(color) );
  SetForegroundColour( wxColor(255,255,255) );
  m_color = color;
}

ColorButton::~ColorButton(){
  delete m_bitmap;
}

PaletteCtrl* ColorButton::GetParentPalette(){
  PaletteCtrl* parent = dynamic_cast<PaletteCtrl*>( GetParent() );
  assert( parent != 0 );
  return parent;
}

void ColorButton::SetColor( const faint::Color& color ){
  m_color = color;
  SetToolTip( StrSmartRGBA( color ) );
  delete m_bitmap;
  if ( m_color.a != 255 ){
    m_bitmap = ColorBitmap( color, 20, 20, false );
  }
  else {
    m_bitmap = 0;
  }
}

void ColorButton::OnPaint( wxPaintEvent& event ){
  if ( m_color.a == 255 ){
    event.Skip();
    return;
  }

  wxPaintDC dc(this);
  dc.DrawBitmap( *m_bitmap, 0, 0 );
}

void ColorButton::OnDoubleClick( wxMouseEvent& ){
  wxColourData data;
  data.SetColour( to_wx( m_color ) );
  data.SetChooseFull( true );

  wxColourDialog dlg( this, &data );
  if ( dlg.ShowModal() == wxID_OK ){
    wxColour color = dlg.GetColourData().GetColour();

    m_color = to_faint(color);
    SetBackgroundColour( color );
    Refresh();
  }
}

void ColorButton::OnLeftUp( wxMouseEvent& ){
  if ( HasCapture() ){
    ReleaseMouse();
  }
  return GetParentPalette()->SetFg( m_color );
}

void ColorButton::OnLeftDown( wxMouseEvent& /* event */ ){
  // Fixme: Drag and drop of colors disabled
  //m_dragStart = event.GetPosition();
  //CaptureMouse();
  GetParentPalette()->SetFg( m_color );
}

void ColorButton::OnRightDown( wxMouseEvent& ){
  GetParentPalette()->SetBg( m_color );
}

void ColorButton::OnMotion( wxMouseEvent& event ){
  std::string statusText = Bracketed(StrSmartRGBA(m_color, true)) + ", " + StrHex(m_color);  
  GetAppContext().GetStatusInfo().SetMainText( statusText );
  if ( HasCapture() ){
    if ( distance( to_faint( event.GetPosition() ), to_faint( m_dragStart ) ) > 15 ){
      // Start drag operation
      ReleaseMouse();
      GetParentPalette()->UndoSetFg();
      ColorDataObject colorObj( m_color );
      FaintDropSource source( this, colorObj );
      wxDragResult result = source.CustomDoDragDrop();
      if ( result == wxDragMove ){
        wxCommandEvent newEvent( EVT_REMOVE_COLOR );
        newEvent.SetEventObject( this );
        ProcessEvent( newEvent );
      }
    }
  }
}

void ColorButton::OnLeave( wxMouseEvent& ){
  GetAppContext().GetStatusInfo().SetMainText("");
}

BEGIN_EVENT_TABLE(ColorButton, wxWindow)
EVT_LEFT_DOWN( ColorButton::OnLeftDown )
EVT_LEFT_UP( ColorButton::OnLeftUp )
EVT_RIGHT_DOWN( ColorButton::OnRightDown )
EVT_LEAVE_WINDOW( ColorButton::OnLeave )
EVT_MOTION( ColorButton::OnMotion )
EVT_LEFT_DCLICK( ColorButton::OnDoubleClick )
EVT_PAINT( ColorButton::OnPaint )
END_EVENT_TABLE()
