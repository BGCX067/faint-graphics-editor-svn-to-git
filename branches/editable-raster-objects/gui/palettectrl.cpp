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

#include "wx/dir.h"
#include "bitmap/bitmap.hh"
#include "gui/colordataobject.hh"
#include "gui/draw-source-dialog.hh"
#include "gui/dropsource.hh"
#include "gui/events.hh"
#include "gui/palettectrl.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"
#include "util/util.hh"

static Optional<faint::CellPos> highlight( const faint::CellPos& pos ){
  return Optional<faint::CellPos>(pos);
}

static Optional<faint::CellPos> no_highlight(){
  return no_option();
}

PaletteCtrl::PaletteCtrl( wxWindow* parent, const Settings& s, SettingNotifier& n, PaletteContainer& palettes, StatusInterface& status)
: wxPanel( parent, wxID_ANY ),
  ColorDropTarget( this ),
  m_notifier( n ),
  m_settings( s ),
  m_statusInterface(status)
{
  #ifdef __WXMSW__
  SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
  #endif
  faint::DrawSourceMap& pal = palettes["default"];
  SetPalette(pal);
}

void PaletteCtrl::Add( const faint::DrawSource& src ){
  m_drawSourceMap.Append(src);
  CreateBitmap(no_highlight());
}

void PaletteCtrl::CreateBitmap( const Optional<faint::CellPos>& highlight ){
  faint::CellSize cellSize(GetButtonSize());
  faint::CellSpacing spacing(GetButtonSpacing());
  faint::Color bg(to_faint(GetBackgroundColour()));
  faint::Bitmap bmp(m_drawSourceMap.CreateBitmap(cellSize, spacing, bg));
  if ( highlight.IsSet() ){
    add_cell_border(bmp, highlight.Get(), cellSize, spacing);
  }
  m_bitmap = to_wx_bmp(bmp);
  SetSize(m_bitmap.GetSize());
  SetInitialSize(m_bitmap.GetSize());
  faint::send_control_resized_event(this);
  Refresh();
  return;
}

faint::CellSize PaletteCtrl::GetButtonSize() const{
  return faint::CellSize(IntSize(24, 24));
}

faint::CellSpacing PaletteCtrl::GetButtonSpacing() const{
  return faint::CellSpacing(IntSize(2,2));
}

faint::CellPos PaletteCtrl::MousePosToPalettePos(const IntPoint& pos) const{
  return faint::view_to_cell_pos(pos, GetButtonSize(), GetButtonSpacing());
}

void PaletteCtrl::OnCaptureLost( wxMouseCaptureLostEvent& ){
  // Required on MSW
}

wxDragResult PaletteCtrl::OnDropColor( const IntPoint& pos, const faint::Color& /* color */ ){
  // Fixme: This ignores the dropped color and only uses indexes, while assumes this panel is the source. This makes the drag and drop work for patterns etc. despite no support in ColorDataObject, but isn't proper dnd.

  bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
  if ( ctrlHeld ){
    m_drawSourceMap.Copy( Old(MousePosToPalettePos(to_faint(m_dragStart))),
      New(MousePosToPalettePos(pos) ) );
  }
  else {
    m_drawSourceMap.Move( Old(MousePosToPalettePos(to_faint(m_dragStart))),
      New(MousePosToPalettePos(pos) ) );
  }
  CreateBitmap(no_highlight());
  return wxDragMove;
}

void PaletteCtrl::OnLeaveWindow( wxMouseEvent& ){
  m_statusInterface.SetMainText("");
}

void PaletteCtrl::OnLeftDoubleClick( wxMouseEvent& event ){
  faint::CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if ( !m_drawSourceMap.Has(pos)){
    return;
  }
  CreateBitmap(highlight(pos));
  Optional<faint::DrawSource> picked = show_drawsource_dialog(0, "Edit Palette Color", m_drawSourceMap.Get(pos), m_statusInterface );
  if ( picked.IsSet() ){
    m_drawSourceMap.Replace(pos, picked.Get());
    SetFg(picked.Get());
  }
  CreateBitmap(no_highlight());
}

void PaletteCtrl::OnLeftDown( wxMouseEvent& event ){
  m_dragStart = event.GetPosition();
  faint::CellPos pos = MousePosToPalettePos(to_faint(m_dragStart));
  if ( !m_drawSourceMap.Has(pos)){
    return;
  }

  faint::DrawSource src(m_drawSourceMap.Get(pos));
  SetFg( src );

  // Capture the mouse to determine distance for drag and drop
  CaptureMouse();
}

void PaletteCtrl::OnLeftUp( wxMouseEvent& ){
  if ( HasCapture() ){
    ReleaseMouse();
  }
}

void PaletteCtrl::OnMotion( wxMouseEvent& event ){
  faint::CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if ( m_drawSourceMap.Has(pos) ){
    const faint::DrawSource& src = m_drawSourceMap.Get(pos);
    m_statusInterface.SetMainText( str(src) );
  }
  else {
    m_statusInterface.SetMainText( "" );
  }

  if ( HasCapture() ){
    const int minDistance(GetButtonSize().Get().w / 2);
    const int dragDistance = floored(distance( to_faint(event.GetPosition()),  to_faint(m_dragStart) ) );
    if ( dragDistance > minDistance ){
      // Start drag operation
      ReleaseMouse();

      // The selected foreground color should not be changed due to
      // clicking a color for dragging.
      UndoSetFg();

      faint::CellPos pos(MousePosToPalettePos(to_faint(m_dragStart)));
      faint::DrawSource src(m_drawSourceMap.Get(pos));
      faint::Color c( src.IsColor() ? src.GetColor() : faint::Color(0,0,0) ); // Fixme: ColorDataObject doesn't support pattern, gradient
      ColorDataObject colorObj(c);
      FaintDropSource source( this, colorObj );
      source.CustomDoDragDrop();
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
  faint::CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if ( !m_drawSourceMap.Has(pos)){
    return;
  }
  bool ctrlHeld = wxGetKeyState( WXK_CONTROL );
  if ( ctrlHeld ){
    m_drawSourceMap.Erase(pos);
    CreateBitmap(no_highlight());
  }
  else {
    SetBg( m_drawSourceMap.Get(pos) );
  }
}

void PaletteCtrl::SetBg(const faint::DrawSource& src){
  m_notifier.Notify( ts_BgCol, src );
}

void PaletteCtrl::SetFg(const faint::DrawSource& src ){
  m_prevFg = m_settings.Get( ts_FgCol );
  m_notifier.Notify( ts_FgCol, src );
}

void PaletteCtrl::SetPalette( const faint::DrawSourceMap& drawSourceMap ){
  m_drawSourceMap = drawSourceMap;
  CreateBitmap(no_highlight());
}

void PaletteCtrl::UndoSetFg(){
  m_notifier.Notify( ts_FgCol, faint::DrawSource(m_prevFg)  );
}

BEGIN_EVENT_TABLE(PaletteCtrl, wxPanel)
EVT_LEAVE_WINDOW( PaletteCtrl::OnLeaveWindow )
EVT_LEFT_DCLICK( PaletteCtrl::OnLeftDoubleClick )
EVT_LEFT_DOWN( PaletteCtrl::OnLeftDown )
EVT_LEFT_UP( PaletteCtrl::OnLeftUp )
EVT_MOTION( PaletteCtrl::OnMotion )
EVT_MOUSE_CAPTURE_LOST( PaletteCtrl::OnCaptureLost )
EVT_PAINT( PaletteCtrl::OnPaint )
EVT_RIGHT_DOWN( PaletteCtrl::OnRightDown )
EVT_RIGHT_DCLICK( PaletteCtrl::OnRightDown )
END_EVENT_TABLE()
