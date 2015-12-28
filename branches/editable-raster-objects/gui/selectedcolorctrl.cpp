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

#include "gui/draw-source-dialog.hh"
#include "gui/events.hh"
#include "gui/selectedcolorctrl.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/formatting.hh"

// Menu items
const int menu_swap = wxNewId();
const int menu_add = wxNewId();
const int menu_copyRgb = wxNewId();
const int menu_copyHex = wxNewId();

DEFINE_EVENT_TYPE(EVT_SWAP_COLORS)

// Fixme: This control uses a lot of hard-coded sizes, would be nicer
// if it used a size for the constructor and adapted accordingly.

static ColorSetting to_setting(SelectedColorCtrl::which hit){
  assert( hit != SelectedColorCtrl::HIT_NEITHER);
  return hit == SelectedColorCtrl::HIT_FG ?
    ts_FgCol : ts_BgCol;
}

SelectedColorCtrl::which SelectedColorCtrl::HitTest( const wxPoint& pos ){
  if ( m_fgRect.Contains(pos) ){
    return HIT_FG;
  }
  else if ( m_bgRect.Contains( pos ) ){
    return HIT_BG;
  }
  else {
    return HIT_NEITHER;
  }
}

SelectedColorCtrl::SelectedColorCtrl( wxWindow* parent, SettingNotifier& notifier, StatusInterface& statusInfo )
  : wxPanel( parent, wxID_ANY ),
    ColorDropTarget( this ),
    m_fgRect( 2, 2, 20, 20 ),
    m_bgRect( 12, 12, 20, 20 ),
    m_fg( faint::Color(0,0,0) ),
    m_bg( faint::Color(0,0,0) ),
    m_fgBmp(to_wx_bmp(draw_source_bitmap(m_fg, to_faint(m_fgRect.GetSize())))),
    m_bgBmp(to_wx_bmp(draw_source_bitmap(m_bg, to_faint(m_bgRect.GetSize())))),
    m_menuEventColor( HIT_NEITHER ),
    m_notifier( notifier ),
    m_statusInfo(statusInfo)
{
  #ifdef __WXMSW__
  // Prevent flicker on full refresh
  SetBackgroundStyle( wxBG_STYLE_PAINT );
  #endif
  SetInitialSize(wxSize(40, 40));
}

const faint::DrawSource& SelectedColorCtrl::GetClickedDrawSource(which hit) const{
  if ( hit == HIT_FG ){
    return m_fg;
  }
  else if ( hit == HIT_BG ){
    return m_bg;
  }
  assert(false);
  return m_fg;
}

const faint::DrawSource& SelectedColorCtrl::MenuTargetColor() const{
  // Return the color the current right click menu refers to
  return GetClickedDrawSource(m_menuEventColor);
}

wxDragResult SelectedColorCtrl::OnDropColor( const IntPoint& pos, const faint::Color& color ){
  // Check which item the color was dropped on and report this
  // upwards.
  which hit = HitTest( to_wx(pos) );
  if ( hit == HIT_FG || hit == HIT_NEITHER ){
    m_notifier.Notify( ts_FgCol, faint::DrawSource(color) );
  }
  else if ( hit == HIT_BG ){
    m_notifier.Notify( ts_BgCol, faint::DrawSource(color) );
  }
  return wxDragCopy;
}

void SelectedColorCtrl::OnPaint( wxPaintEvent& ){
  wxPaintDC dc( this );
  #ifdef __WXMSW__
  // Clear the background (for wxBG_STYLE_PAINT). Not required on GTK,
  // and GetBackgroundColour gives a darker gray for some reason
  dc.SetBackground( GetBackgroundColour() );
  dc.Clear();
  #endif
  dc.DrawBitmap( m_bgBmp, m_bgRect.x, m_bgRect.y );
  dc.DrawBitmap( m_fgBmp, m_fgRect.x, m_fgRect.y );
}

void SelectedColorCtrl::OnEnter( wxMouseEvent& ){
}

void SelectedColorCtrl::OnLeave( wxMouseEvent& ){
  m_statusInfo.SetMainText("");
}

void SelectedColorCtrl::UpdateColors( const faint::DrawSource& fg, const faint::DrawSource& bg ){
  m_fg = fg;
  m_bg = bg;
  m_fgBmp = to_wx_bmp(with_border(draw_source_bitmap( m_fg, IntSize(m_fgRect.width, m_fgRect.height) )));
  m_bgBmp = to_wx_bmp(with_border(draw_source_bitmap( m_bg, IntSize(m_bgRect.width, m_bgRect.height) )));
  Refresh();
}

void SelectedColorCtrl::OnLeftDown( wxMouseEvent& event ){
  wxPoint pos( event.GetPosition() );
  which hit = HitTest( pos );
  if ( hit == HIT_NEITHER ){
    return;
  }
  m_statusInfo.SetMainText("");
  const faint::DrawSource& src(GetClickedDrawSource(hit));
  wxString title( hit == HIT_FG ? "Select Foreground Color" : "Select Background Color" );
  Optional<faint::DrawSource> picked = show_drawsource_dialog(0, title, src, m_statusInfo);
  if ( picked.IsSet() ){
    m_notifier.Notify(to_setting(hit), picked.Get());
  }
}

void SelectedColorCtrl::OnRightDown( wxMouseEvent& event ){
  // Show the context menu for the right clicked color
  m_statusInfo.SetMainText("");
  const wxPoint& p = event.GetPosition();
  m_menuEventColor = HitTest( p );
  wxMenu contextMenu;
  contextMenu.Append( menu_swap, "Swap colors" );
  contextMenu.Append( menu_add, "Add to palette");
  contextMenu.AppendSeparator();
  contextMenu.Append( menu_copyHex, "Copy hex" );
  contextMenu.Append( menu_copyRgb, "Copy rgb" );

  // Disable items that require a specific color if neither the
  // foreground or background rectangles were hit
  if ( m_menuEventColor == HIT_NEITHER ){
    contextMenu.Enable( menu_add, false );
    contextMenu.Enable( menu_copyHex, false );
    contextMenu.Enable( menu_copyRgb, false );
  }
  else{
    const faint::DrawSource src(GetClickedDrawSource(m_menuEventColor));
    if ( !src.IsColor() ){
      // Only colors can be copied as rgb or hex
      contextMenu.Enable( menu_copyHex, false );
      contextMenu.Enable( menu_copyRgb, false );
    }
  }
  PopupMenu( &contextMenu, event.GetPosition() );
}

void SelectedColorCtrl::OnMotion( wxMouseEvent& event ){
  SelectedColorCtrl::which hit = HitTest( event.GetPosition() );

  if ( hit == HIT_NEITHER ){
    m_statusInfo.SetMainText("");
    m_statusInfo.SetText( "", 0 );
  }
  else {
    m_statusInfo.SetMainText( "Left click for color dialog, right click for options." );
    m_statusInfo.SetText(str(GetClickedDrawSource(hit)), 0);
  }
}

void SelectedColorCtrl::OnMenuChoice( wxCommandEvent& evt ){
  const int action = evt.GetId();
  if ( action == menu_swap ){
    wxCommandEvent newEvent(EVT_SWAP_COLORS);
    ProcessEvent(newEvent);
    return;
  }
  else{
    faint::DrawSource src(MenuTargetColor());
    if ( action == menu_add ){
      DrawSourceEvent newEvent(FAINT_ADD_TO_PALETTE, src);
      ProcessEvent(newEvent);
    }
    else if ( action == menu_copyHex ){
      assert(src.IsColor()); // Should be unavailable for other than plain color
      ColorEvent newEvent(FAINT_COPY_COLOR_HEX, src.GetColor());
      ProcessEvent(newEvent);
    }
    else if ( action == menu_copyRgb ){
      assert(src.IsColor()); // Should be unavailable for other than plain color
      ColorEvent newEvent(FAINT_COPY_COLOR_RGB, src.GetColor());
      ProcessEvent(newEvent);
    }
  }
}

BEGIN_EVENT_TABLE( SelectedColorCtrl, wxWindow )
EVT_PAINT( SelectedColorCtrl::OnPaint )
EVT_LEFT_DOWN( SelectedColorCtrl::OnLeftDown )
EVT_RIGHT_DOWN( SelectedColorCtrl::OnRightDown )
EVT_MOTION(SelectedColorCtrl::OnMotion)
EVT_ENTER_WINDOW( SelectedColorCtrl::OnEnter )
EVT_LEAVE_WINDOW( SelectedColorCtrl::OnLeave )
EVT_MENU( wxID_ANY, SelectedColorCtrl::OnMenuChoice )
END_EVENT_TABLE()
