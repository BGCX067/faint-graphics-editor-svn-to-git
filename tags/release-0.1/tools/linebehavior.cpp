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

#include <string>
#include "canvasinterface.hh"
#include "commands/addobjectcommand.hh"
#include "cursors.hh"
#include "faintdc.hh"
#include "linebehavior.hh"
#include "objects/objline.hh"
#include "util/objutil.hh"
#include "settingid.hh"
#include "util/angle.hh"
#include "util/util.hh"
#include "getappcontext.hh"
using std::min;
using std::max;

FaintSettings RemoveBackgroundColor( FaintSettings& s ){
  FaintSettings out(s);
  if ( out.Has (ts_BgCol ) ){
    out.Erase( ts_BgCol );
  }
  if ( out.GetDefault( ts_SwapColors, false ) ){
    out.Set( ts_FgCol, s.Get(ts_BgCol ) );
    out.Erase( ts_SwapColors );
  }  
  return out;
}

LineCommand::LineCommand( const Point& p0, const Point& p1, const FaintSettings& settings)
  : Command( CMD_TYPE_RASTER ),
    m_settings( settings ),
    m_p0( p0 ),
    m_p1( p1 )
{}

void LineCommand::Do( faint::Image& img ){
  FaintDC dc( img.GetBitmapRef() );
  dc.Line( m_p0.x, m_p0.y, m_p1.x, m_p1.y, m_settings );
}

LineBehavior::LineBehavior()
  : ToolBehavior( T_LINE ),
    m_p1(0,0),
    m_p2(0,0)
{
  m_settings.Set( ts_FgCol, faint::Color( 0,0,0 ) );
  m_settings.Set( ts_BgCol, faint::Color( 255,255,255 ) );
  m_settings.Set( ts_LineWidth, 1 );
  m_settings.Set( ts_LineCap, faint::CAP_BUTT );
  m_settings.Set( ts_LineStyle, faint::SOLID );
  m_settings.Set( ts_LineArrowHead, faint::ARROW_NONE );
  m_active = false;
}

bool LineBehavior::DrawBeforeZoom( Layer layer ) const{
  return layer == LAYER_RASTER;
}

void LineStatusBar( StatusInterface& status, const Point& p1, const Point& p2, int modifiers ){
  if ( (modifiers & TOOLMODIFIER2) == TOOLMODIFIER2 ) {
    status.SetMainText("");
  }
  else if ( ( modifiers & TOOLMODIFIER1 ) == TOOLMODIFIER1 ){
    status.SetMainText("");
  }
  else {
    status.SetMainText( "Ctrl=Snap Shift=Constrain" );
  }

  status.SetText( StrLineStatus( p1, p2 ), 0 );
}

ToolRefresh LineBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_settings.Set( ts_SwapColors, mbtn(modifiers) == RIGHT_MOUSE );
  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );

  if ( (modifiers & TOOLMODIFIER1) == TOOLMODIFIER1 ) {
    m_p1 = Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
  }
  else {
    m_p1 = info.pos;
  }

  m_p2 = m_p1;
  m_active = true;

  LineStatusBar( GetAppContext().GetStatusInfo(), m_p1, m_p2, modifiers );
  return TOOL_OVERLAY;
}

ToolRefresh LineBehavior::LeftUp( const CursorPositionInfo& info, int modifiers ){
  if ( ! m_active ){
    return TOOL_NONE;
  }

  if ( fl( TOOLMODIFIER2, modifiers ) ){
    m_p2 = AdjustTo45( m_p1, info.pos );
  }
  else if ( fl( TOOLMODIFIER1, modifiers ) ){
    Point snapped = Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
    m_p2 = (snapped == m_p1) ? info.pos : snapped;
  }
  else {
    m_p2 = info.pos;
  }

  m_active = false;

  if ( info.layerType == LAYER_RASTER ){
    m_command.Set( new LineCommand( m_p1, m_p2, m_settings ) );
  }
  else if ( info.layerType == LAYER_OBJECT ){
    if ( m_p1 == m_p2 ){
      // Prevent 0-length object lines, as they're invisible (Such raster lines are allowed - it's the same
      // as drawing a pixel)
      return TOOL_NONE;
    }
    Object* newObj = new ObjLine( m_p1, m_p2, RemoveBackgroundColor(m_settings) );
    m_command.Set( new AddObjectCommand( newObj ) );
  }
  else {
    assert( false );
  }
  return TOOL_COMMIT;
}

ToolRefresh LineBehavior::Preempt(){
  if ( m_active ){
    m_active = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool LineBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active ){
    dc.Line( m_p1.x, m_p1.y, m_p2.x, m_p2.y, m_settings );
    return true;
  }
  return false;
}

ToolRefresh LineBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );
  if ( !m_active ){
    StatusInterface& status = GetAppContext().GetStatusInfo();
    status.SetMainText("");
    status.SetText( StrPoint( info.pos ), 0 );
    return TOOL_NONE;
  }

  if ( (modifiers & TOOLMODIFIER2) == TOOLMODIFIER2 ) {
    // Constrained line
    m_p2 = AdjustTo45( m_p1, info.pos );
  }
  else if ( ( modifiers & TOOLMODIFIER1 ) == TOOLMODIFIER1 ){
    Point snapped = Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
    // Do not snap to points on the line start point
    m_p2 = (snapped == m_p1) ? info.pos : snapped;
  }
  else {
    m_p2 = info.pos;
  }

  LineStatusBar( GetAppContext().GetStatusInfo(), m_p1, m_p2, modifiers );
  return TOOL_OVERLAY;
}

IntRect LineBehavior::GetRefreshRect( const IntRect&, const Point& ){
  faint::coord extraWidth = m_settings.Get( ts_LineWidth );
  if ( m_settings.Get( ts_LineArrowHead ) ){
    extraWidth *= LITCRD(10.0);
  }
  return truncated( Inflated( Rect( m_p1, m_p2 ), extraWidth ) );
}

Command* LineBehavior::GetCommand(){
  return m_command.Retrieve();
}

unsigned int LineBehavior::GetStatusFieldCount(){
  return 1;
}

int LineBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_CROSSHAIR;
}
