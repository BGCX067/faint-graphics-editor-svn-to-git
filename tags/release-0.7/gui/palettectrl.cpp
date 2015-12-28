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

#include <vector>
#include "wx/sizer.h"
#include "wx/dir.h"
#include "app/getappcontext.hh"
#include "gui/colordataobject.hh"
#include "gui/colordialog.hh"
#include "gui/dropsource.hh"
#include "gui/events.hh"
#include "gui/palettectrl.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/paletteparser.hh"
#include "util/util.hh"

DECLARE_EVENT_TYPE(EVT_REMOVE_COLOR, -1)
DEFINE_EVENT_TYPE(EVT_REMOVE_COLOR)

const faint::Color& get_color( const IntPoint& pt, const color_map_t& colorMap ){
  return colorMap.find(pt)->second;
}

bool color_at( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  return m.find(pt) != m.end() && get_color(pt,m) == c;
}

bool case_a( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  IntPoint right(pt.x + 1, pt.y );
  IntPoint below(pt.x, pt.y + 1 );
  IntPoint belowright(pt.x + 1, pt.y + 1 );

  return color_at(pt, m, c) && color_at(right, m, c) && !(color_at(belowright,m, c) && color_at(below,m, c));
}

bool case_e( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  IntPoint above(pt.x, pt.y - 1);
  IntPoint right(pt.x + 1, pt.y);
  IntPoint below(pt.x, pt.y + 1);
  return color_at(pt,m,c) && color_at(below,m,c) && color_at(above,m,c) && !color_at(right,m,c);
}

bool case_b( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  IntPoint right(pt.x + 1, pt.y );
  IntPoint below(pt.x, pt.y + 1 );
  return color_at(pt, m,c) && !color_at(right, m,c) && color_at(below, m,c) && !case_e(pt, m,c);
}

bool case_c( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  IntPoint right(pt.x + 1, pt.y );
  IntPoint above(pt.x, pt.y - 1);
  IntPoint aboveright(pt.x + 1, pt.y - 1);
  IntPoint below(pt.x, pt.y + 1);
  return color_at(pt,m,c) && color_at(above,m,c) && color_at(aboveright,m,c) && !color_at(below,m,c);
}

bool case_d( const IntPoint& pt, const color_map_t& m, const faint::Color& c ){
  IntPoint right(pt.x + 1, pt.y);
  IntPoint below(pt.x, pt.y + 1);
  IntPoint belowRight(pt.x + 1, pt.y + 1);
  return color_at(pt,m,c) && color_at(right,m,c) && color_at(below,m,c) && color_at(belowRight,m,c);
}

PaletteCtrl::PaletteCtrl( wxWindow* parent, const Settings& s, SettingNotifier& n, PaletteContainer& palettes)
: wxPanel( parent, wxID_ANY ),
  ColorDropTarget( this ),
  m_notifier( n ),
  m_settings( s ),
  m_paletteSize(15,2),
  m_highLight(-1,-1)
{
  #ifdef __WXMSW__
  SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
  #endif
  std::vector<faint::Color>& pal = palettes["default"];
  SetPalette(pal);
}

void PaletteCtrl::AddColor( const faint::Color& color ){
  DoAddColor(color);
  CreateBitmap();
}

void PaletteCtrl::SetPalette( const std::vector<faint::Color>& colors ){
  m_colors.clear();
  for ( size_t i = 0; i!= colors.size(); i++ ){
    DoAddColor( colors[i] );
  }
  CreateBitmap();
}

void PaletteCtrl::CreateBitmap(){
  IntPoint maxPos = GetMaxPos();
  IntSize btnSz = GetButtonSize();
  IntSize spSz = GetButtonSpacing();
  IntSize offset = btnSz + spSz;

  wxSize sz( ( maxPos.x + 1 ) * (btnSz.w + spSz.w), ( maxPos.y + 1 ) * (btnSz.h + spSz.h) );
  m_bitmap = wxBitmap( sz );

  wxMemoryDC dc(m_bitmap);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  for ( std::map<IntPoint, faint::Color>::const_iterator it = m_colors.begin(); it != m_colors.end(); ++it ){
    const IntPoint& p = it->first;
    const faint::Color& c = it->second;

    // Grow this color-cell to merge it with any same-color adjacent
    // cells. Ignore this for translucent cells, since the ColorBitmap
    // function won't handle this well.
    IntSize thisSz(btnSz);
    int offsetY = 0;
    if (opaque(c)){
      if ( case_d(p, m_colors,c) ){
	thisSz.w += spSz.w;
	thisSz.h += spSz.h;
      }
      else if ( case_a(p, m_colors,c) ){
	thisSz.w += spSz.w;
      }
      else if ( case_b(p, m_colors,c) ){
	thisSz.h += spSz.h;
      }
      else if ( case_c(p, m_colors,c) ){
	thisSz.h += spSz.h;
	offsetY = -spSz.h;
      }
      else if ( case_e(p, m_colors,c) ){
	thisSz.h += spSz.h * 2;
	offsetY -= spSz.h;
      }
    }

    if ( translucent(c) ){
      wxBitmap colorBitmap = color_bitmap( c, thisSz, false );
      dc.DrawBitmap( colorBitmap, wxPoint(p.x * offset.w, p.y * offset.h) );
    }
    else {
      dc.SetBrush( to_wx(c) );
      dc.SetPen( to_wx(c) );
      dc.DrawRectangle( p.x * offset.w, p.y * offset.h + offsetY, thisSz.w, thisSz.h );
    }
  }

  if ( m_highLight != IntPoint(-1,-1) ){
    dc.SetBrush( *wxTRANSPARENT_BRUSH );
    dc.SetPen( wxPen( wxColour(0,0,0), 1) );
    const IntPoint p(m_highLight);
    dc.DrawRectangle( p.x * offset.w, p.y * offset.h, btnSz.w, btnSz.h );
  }

  SetInitialSize(sz);
  SetSize(sz);
  faint::send_control_resized_event(this);
  Refresh();
}

void PaletteCtrl::DoAddColor( const faint::Color& color ){
  size_t numColors = m_colors.size();
  IntPoint pos = IndexToPalettePos(numColors);
  if ( pos.y >= m_paletteSize.h ){
    pos.y = 0;
    pos.x = m_paletteSize.w;
    m_paletteSize.w += 1;

  }
  m_colors[pos] = color;
}

void PaletteCtrl::DoRemoveColor( const IntPoint& remPos ){
  color_map_t newColors;
  for ( color_map_t::const_iterator it = m_colors.begin(); it != m_colors.end(); ++it ){
    const IntPoint& p = it->first;
    if ( p == remPos ){
      continue;
    }
    if ( p.x >= remPos.x && p.y == remPos.y ){
      newColors[IntPoint(p.x - 1, p.y)] = it->second;
    }
    else {
      newColors[p] = it->second;
    }
  }
  m_colors = newColors;
}

IntSize PaletteCtrl::GetButtonSize() const{
  return IntSize(24, 24); // Fixme: Compute
}

IntSize PaletteCtrl::GetButtonSpacing() const{
  return IntSize(2,2);
}

const faint::Color& PaletteCtrl::GetColor( const IntPoint& pos ) const{
  color_map_t::const_iterator it = m_colors.find(pos);
  assert( it != m_colors.end() );
  return it->second;
}

IntPoint PaletteCtrl::GetMaxPos(){
  if ( m_colors.empty() ){
    return IntPoint(0,0);
  }
  IntPoint maxPos = m_colors.begin()->first;
  for ( std::map<IntPoint, faint::Color>::const_iterator it = m_colors.begin(); it != m_colors.end(); ++it ){
    const IntPoint& p = it->first;
    maxPos.x = std::max(maxPos.x, p.x);
    maxPos.y = std::max(maxPos.y, p.y);
  }
  return maxPos;
}

bool PaletteCtrl::HasColorAt( const IntPoint& pos ) const{
  return m_colors.find(pos) != m_colors.end();
}

void PaletteCtrl::Highlight( const IntPoint& pos, bool update ){
  m_highLight = pos;
  if ( update ){
    CreateBitmap();
  }
}

IntPoint PaletteCtrl::IndexToPalettePos( size_t num ){
  return IntPoint( num % m_paletteSize.w, num / m_paletteSize.w );
}

void PaletteCtrl::DoInsertColor( const IntPoint& insPos, const faint::Color& color ){
  color_map_t newColors;
  for ( color_map_t::const_iterator it = m_colors.begin(); it != m_colors.end(); ++it ){
    const IntPoint& p = it->first;
    if ( p.x >= insPos.x && p.y == insPos.y ){
      newColors[IntPoint(p.x + 1, p.y)] = it->second;
    }
    else {
      newColors[p] = it->second;
    }
  }
  newColors[insPos] = color;
  m_colors = newColors;
}

IntPoint PaletteCtrl::MousePosToPalettePos(const wxPoint& pos ){
  IntSize btnSize(GetButtonSize());
  IntSize spSize(GetButtonSpacing());
  return IntPoint( pos.x / (btnSize.w + spSize.w), pos.y / (btnSize.h + spSize.h ) );
}

void PaletteCtrl::OnDoubleClick( wxMouseEvent& event ){
  IntPoint pos = MousePosToPalettePos(event.GetPosition());
  if ( !HasColorAt(pos)){
    return;
  }
  Highlight(pos, true);
  ColorDialog dlg(0);
  bool ok = dlg.ShowModal(GetColor(pos) );
  Highlight(IntPoint(-1,-1), !ok);
  if ( ok ){
    SetColor(pos, dlg.GetColor() );
  }
}

wxDragResult PaletteCtrl::OnDropColor( const IntPoint& pos, const faint::Color& color ){
  // Fixme: Adjust this, the colors should shift depending on drag-direction
  bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
  if ( !ctrlHeld ){
    DoRemoveColor(MousePosToPalettePos(m_dragStart));
  }
  DoInsertColor( MousePosToPalettePos(to_wx(pos)), color );
  CreateBitmap();
  return wxDragMove;
}

void PaletteCtrl::OnLeaveWindow( wxMouseEvent& ){
  GetAppContext().GetStatusInfo().SetMainText("");
}

void PaletteCtrl::OnLeftDown( wxMouseEvent& event ){
  m_dragStart = event.GetPosition();
  IntPoint pos = MousePosToPalettePos(m_dragStart);
  if ( !HasColorAt(pos)){
    return;
  }
  SetFg( GetColor(pos) );

  CaptureMouse();
}

void PaletteCtrl::OnLeftUp( wxMouseEvent& ){
  if ( HasCapture() ){
    ReleaseMouse();
  }
}

void PaletteCtrl::OnMotion( wxMouseEvent& event ){
  IntPoint pos = MousePosToPalettePos(event.GetPosition());
  StatusInterface& status = GetAppContext().GetStatusInfo();
  if ( HasColorAt(pos) ){
    const faint::Color& c = GetColor(pos);
    status.SetMainText( bracketed(str_smart_rgba(c, true ) ) + ", " + str_hex(c));
  }
  else {
    status.SetMainText( "" );
  }

  if ( HasCapture() ){
    if ( distance( to_faint( event.GetPosition() ), to_faint(m_dragStart) ) > GetButtonSize().w ){
      // Start drag operation
      ReleaseMouse();
      UndoSetFg();
      ColorDataObject colorObj( GetColor(MousePosToPalettePos(m_dragStart)) );
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

void PaletteCtrl::OnPaint( wxPaintEvent& ){
  wxPaintDC dc(this);
  #ifdef __WXMSW__
  // Clear the background (for wxBG_STYLE_PAINT). Not required on GTK,
  // and GetBackgroundColour gives a darker gray for some reason
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
  #endif
  dc.DrawBitmap( m_bitmap, 0, 0 );
}

void PaletteCtrl::OnRightDown( wxMouseEvent& event ){
  IntPoint pos = MousePosToPalettePos(event.GetPosition());
  if ( !HasColorAt(pos)){
    return;
  }
  bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
  if ( ctrlHeld ){
    RemoveColor(pos);
  }
  else {
    SetBg( GetColor(pos) );
  }
}

void PaletteCtrl::RemoveColor(const IntPoint& remPos ){
  DoRemoveColor(remPos);
  CreateBitmap();
}

void PaletteCtrl::SetBg(const faint::Color& color){
  m_notifier.Notify( ts_BgCol, color );
}

void PaletteCtrl::SetColor( const IntPoint& pos, const faint::Color& color ){
  m_colors[pos] = color;
  CreateBitmap();
}

void PaletteCtrl::SetFg(const faint::Color& color ){
  m_prevFg = m_settings.Get( ts_FgCol );
  m_notifier.Notify( ts_FgCol, color );
}

void PaletteCtrl::UndoSetFg(){
  m_notifier.Notify( ts_FgCol, m_prevFg  );
}

BEGIN_EVENT_TABLE(PaletteCtrl, wxPanel)
EVT_PAINT( PaletteCtrl::OnPaint )
EVT_MOTION( PaletteCtrl::OnMotion )
EVT_LEAVE_WINDOW( PaletteCtrl::OnLeaveWindow )
EVT_LEFT_UP( PaletteCtrl::OnLeftUp )
EVT_LEFT_DOWN( PaletteCtrl::OnLeftDown )
EVT_RIGHT_DOWN( PaletteCtrl::OnRightDown )
EVT_LEFT_DCLICK( PaletteCtrl::OnDoubleClick )
END_EVENT_TABLE()
