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

#include "app/getappcontext.hh"
#include "gui/colordialog.hh"
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

SelectedColorCtrl::SelectedColorCtrl( wxWindow* parent, SettingNotifier& notifier )
  : wxPanel( parent, wxID_ANY ),
    ColorDropTarget( this ),
    m_fgRect( 2, 2, 20, 20 ),
    m_bgRect( 12, 12, 20, 20 ),
    m_fgCol( 0, 0, 0 ),
    m_bgCol( 0, 0, 0 ),
    m_fgBmp(color_bitmap(m_fgCol, to_faint(m_fgRect.GetSize()), true)),
    m_bgBmp(color_bitmap(m_bgCol, to_faint(m_bgRect.GetSize()), true)),
    m_menuEventColor( HIT_NEITHER ),
    m_notifier( notifier )
{
  #ifdef __WXMSW__
  // Prevent flicker on full refresh
  SetBackgroundStyle( wxBG_STYLE_PAINT );
  #endif
  SetInitialSize(wxSize(40, 40));
}

faint::Color SelectedColorCtrl::AffectedColor() const{
  // Return the color the current right click menu refers to
  return m_menuEventColor == HIT_FG ? m_fgCol : m_bgCol;
}

wxDragResult SelectedColorCtrl::OnDropColor( const IntPoint& pos, const faint::Color& color ){
  // Check which item the color was dropped on and report this
  // upwards.
  which hit = HitTest( to_wx(pos) );
  if ( hit == HIT_FG || hit == HIT_NEITHER ){
    m_notifier.Notify( ts_FgCol, color );
  }
  else if ( hit == HIT_BG ){
    m_notifier.Notify( ts_BgCol, color );
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
  GetAppContext().GetStatusInfo().SetMainText("");
}

void SelectedColorCtrl::UpdateColors( const faint::Color& fg, const faint::Color& bg ){
  m_fgCol = fg;
  m_bgCol = bg;
  m_fgBmp = color_bitmap( m_fgCol, IntSize(m_fgRect.width, m_fgRect.height), true );
  m_bgBmp = color_bitmap( m_bgCol, IntSize(m_bgRect.width, m_bgRect.height), true );
  Refresh();
}

void SelectedColorCtrl::OnLeftDown( wxMouseEvent& event ){
  wxPoint pos( event.GetPosition() );
  which hit = HitTest( pos );
  if ( hit == HIT_NEITHER ){
    return;
  }
  GetAppContext().GetStatusInfo().SetMainText("");
  ColorSetting setting(hit == HIT_FG ? ts_FgCol : ts_BgCol);
  faint::Color color(hit == HIT_FG ? m_fgCol : m_bgCol);
  ColorDialog dlg(0);

  bool ok = dlg.ShowModal(color);
  if ( ok ){
    m_notifier.Notify(setting, dlg.GetColor());
  }
}

void SelectedColorCtrl::OnRightDown( wxMouseEvent& event ){
  // Show the context menu for the right clicked color
  GetAppContext().GetStatusInfo().SetMainText("");
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

  PopupMenu( &contextMenu, event.GetPosition() );
}

void SelectedColorCtrl::OnMotion( wxMouseEvent& event ){
  SelectedColorCtrl::which hit = HitTest( event.GetPosition() );
  StatusInterface& statusInfo = GetAppContext().GetStatusInfo();

  if ( hit == HIT_NEITHER ){
    statusInfo.SetMainText("");
    statusInfo.SetText( "", 0 );
    return;
  }

  statusInfo.SetMainText( "Left click for color dialog, right click for options." );
  const faint::Color& color = (hit == HIT_FG ) ? m_fgCol : m_bgCol;
  statusInfo.SetText( str_smart_rgba(color) + ", " + str_hex(color), 0 );
}

void SelectedColorCtrl::OnMenuChoice( wxCommandEvent& evt ){
  const int action = evt.GetId();
  if ( action == menu_swap ){
    wxCommandEvent newEvent(EVT_SWAP_COLORS);
    ProcessEvent(newEvent);
  }
  else if ( action == menu_add ){
    ColorEvent newEvent(FAINT_ADD_COLOR, AffectedColor());
    ProcessEvent(newEvent);
  }
  else if ( action == menu_copyHex ){
    ColorEvent newEvent(FAINT_COPY_COLOR_HEX, AffectedColor());
    ProcessEvent(newEvent);
  }
  else if ( action == menu_copyRgb ){
    ColorEvent newEvent(FAINT_COPY_COLOR_RGB, AffectedColor());
    ProcessEvent(newEvent);
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
