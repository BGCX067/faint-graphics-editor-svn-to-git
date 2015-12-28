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

#include "selectedcolorctrl.hh"
#include "wx/colordlg.h"
#include "convertwx.hh"
#include "getappcontext.hh"
#include "colorutil.hh"
#include "formatting.hh"
// Menu items
const int menu_swap = wxNewId();
const int menu_add = wxNewId();
const int menu_copyRgb = wxNewId();
const int menu_copyHex = wxNewId();

// Color identifier for events
const int ADD_FG = 1;
const int ADD_BG = 2;

// Events emitted by SelectedColorCtrl
DEFINE_EVENT_TYPE(EVT_ADD_COLOR)
DEFINE_EVENT_TYPE(EVT_SWAP_COLORS)
DEFINE_EVENT_TYPE(EVT_COPY_HEX)
DEFINE_EVENT_TYPE(EVT_COPY_RGB)

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
  : wxWindow( parent, wxID_ANY ),
    ColorDropTarget( this ),
    m_fgCol( 0, 0, 0 ),
    m_bgCol( 0, 0, 0 ),
    m_fgBmp(0),
    m_bgBmp(0),
    m_fgRect( 2, 2, 20, 20 ),
    m_bgRect( 12, 12, 20, 20 ),
    m_menuEventColor( HIT_NEITHER ),
    m_notifier( notifier )
{
  SetInitialSize(wxSize( 40, 40) );
}

SelectedColorCtrl::~SelectedColorCtrl(){
  delete m_fgBmp;
  delete m_bgBmp;
}

wxDragResult SelectedColorCtrl::OnDropColor( const wxPoint& p, const faint::Color& color ){
  // Check which item the color was dropped on and report
  // this upwards.
  which hit = HitTest( p );
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
  dc.DrawBitmap( *m_bgBmp, m_bgRect.x, m_bgRect.y );
  dc.DrawBitmap( *m_fgBmp, m_fgRect.x, m_fgRect.y );
}

void SelectedColorCtrl::OnEnter( wxMouseEvent& ){
}

void SelectedColorCtrl::OnLeave( wxMouseEvent& ){
  GetAppContext().GetStatusInfo().SetMainText("");
}

void SelectedColorCtrl::Update( const faint::Color& fg, const faint::Color& bg ){
  m_fgCol = fg;
  m_bgCol = bg;

  delete m_fgBmp;
  m_fgBmp = ColorBitmap( m_fgCol, m_fgRect.width, m_fgRect.height, true );
  delete m_bgBmp;
  m_bgBmp = ColorBitmap( m_bgCol, m_bgRect.width, m_bgRect.height, true );
  Refresh();
}

void SelectedColorCtrl::OnDoubleClick( wxMouseEvent& event ){
  wxPoint pos( event.GetPosition() );
  which hit = HitTest( pos );
  if ( hit == HIT_NEITHER ){
    return;
  }

  GetAppContext().GetStatusInfo().SetMainText("");

  // Prepare and show a color dialog for changing either the
  // foreground or background color.
  wxColourData* data = new wxColourData();
  data->SetColour( hit == HIT_FG ? to_wx( m_fgCol ) : to_wx( m_bgCol ) );
  data->SetChooseFull(true);
  wxColourDialog dlg( this, data );
  if ( dlg.ShowModal() == wxID_OK ){
    wxColour color( dlg.GetColourData().GetColour() );
    if ( hit == HIT_FG ){
      m_notifier.Notify( ts_FgCol, to_faint( color ) );
    }
    else {
      m_notifier.Notify( ts_BgCol, to_faint( color ) );
    }
  }
  dlg.Destroy();
  delete data;
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

  statusInfo.SetMainText( "Double click for color dialog, right click for options." );
  const faint::Color& color = (hit == HIT_FG ) ? m_fgCol : m_bgCol;
  statusInfo.SetText( StrSmartRGBA(color) + ", " + StrHex(color), 0 );
}

void SelectedColorCtrl::OnMenuChoice( wxCommandEvent& event ){
  int id = event.GetId();
  if ( id == menu_swap ){
    wxCommandEvent newEvent( EVT_SWAP_COLORS );
    ProcessEvent( newEvent );
  }
  else if ( id == menu_add ){
    wxCommandEvent newEvent( EVT_ADD_COLOR );
    newEvent.SetInt( m_menuEventColor == HIT_FG ? ADD_FG : ADD_BG );
    ProcessEvent( newEvent );
  }
  else if ( id == menu_copyHex ){
    wxCommandEvent newEvent( EVT_COPY_HEX );
    newEvent.SetInt( m_menuEventColor == HIT_FG ? ADD_FG : ADD_BG );
    ProcessEvent( newEvent );
  }
  else if ( id == menu_copyRgb ){
    wxCommandEvent newEvent( EVT_COPY_RGB );
    newEvent.SetInt( m_menuEventColor == HIT_FG ? ADD_FG : ADD_BG );
    ProcessEvent( newEvent );

  }
}

BEGIN_EVENT_TABLE( SelectedColorCtrl, wxWindow )
EVT_PAINT( SelectedColorCtrl::OnPaint )
EVT_LEFT_DCLICK( SelectedColorCtrl::OnDoubleClick )
EVT_RIGHT_DOWN( SelectedColorCtrl::OnRightDown )
EVT_MOTION(SelectedColorCtrl::OnMotion)
EVT_ENTER_WINDOW( SelectedColorCtrl::OnEnter )
EVT_LEAVE_WINDOW( SelectedColorCtrl::OnLeave )
EVT_MENU( wxID_ANY, SelectedColorCtrl::OnMenuChoice )
END_EVENT_TABLE()
