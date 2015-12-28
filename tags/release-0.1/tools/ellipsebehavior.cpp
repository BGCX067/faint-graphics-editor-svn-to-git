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

#include "ellipsebehavior.hh"
#include "cursors.hh"
#include "commands/addobjectcommand.hh"
#include "objects/objellipse.hh"
#include "util/angle.hh"
#include "settingid.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "util/objutil.hh"

using std::min;
using std::max;

EllipseCommand::EllipseCommand( const IntRect& r, const FaintSettings& settings )
  : Command( CMD_TYPE_RASTER ), m_settings( settings ),
    m_rect(r)
{}

void EllipseCommand::Do( faint::Image& image ){
  FaintDC dc( image.GetBitmapRef() );
  dc.Ellipse( TriFromRect( floated(m_rect) ), m_settings );
}

EllipseBehavior::EllipseBehavior()
  : ToolBehavior(T_ELLIPSE),
    m_p1(0,0),
    m_p2(0,0),
    m_origP1(0,0)
{
  m_active = false;
  m_settings.Set( ts_LineWidth, 1 );
  m_settings.Set( ts_LineStyle, 1 );
  m_settings.Set( ts_FillStyle, BORDER );
  m_settings.Set( ts_FgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_BgCol, faint::Color(255,255,255) );
  m_settings.Set( ts_SwapColors, false );
}

bool EllipseBehavior::DrawBeforeZoom(Layer layerType) const{
  return layerType == LAYER_RASTER;
}

ToolRefresh EllipseBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );
  m_active = true;
  m_settings.Set( ts_SwapColors, RIGHT_MOUSE == mbtn( modifiers ) );

  if ( ( modifiers & TOOLMODIFIER1 ) == TOOLMODIFIER1 ){
    m_p1 = m_origP1 = Snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
  }
  else {
    m_p1 = m_origP1 = info.pos;
  }
  m_p1 = floated(truncated(m_p1));
  m_p2 = m_p1;
  return TOOL_NONE;
}

ToolRefresh EllipseBehavior::LeftUp( const CursorPositionInfo& info, int ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  m_active = false;

  faint::coord x0 = min( m_p1.x, m_p2.x );
  faint::coord x1 = max( m_p1.x, m_p2.x );
  faint::coord y0 = min( m_p1.y, m_p2.y ) ;
  faint::coord y1 = max( m_p1.y, m_p2.y );
  faint::coord w = x1 - x0 + 1;
  faint::coord h = y1 - y0 + 1;

  if ( w == 0 || h == 0 ){
    m_active = false;
    return TOOL_NONE;
  }

  if ( info.layerType == LAYER_RASTER ){
    m_command.Set( new EllipseCommand( IntRect( truncated(m_p1), truncated(m_p2)) , m_settings ) );
  }
  else {
    m_command.Set( new AddObjectCommand( new ObjEllipse( TriFromRect(Rect(Point(x0, y0), Size(w, h))), m_settings ) ) );
  }
  GetAppContext().GetStatusInfo().SetMainText("");
  return TOOL_COMMIT;
}

ToolRefresh EllipseBehavior::Motion( const CursorPositionInfo& info,  int modifiers ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  if ( !m_active ){
    status.SetText( StrPoint( info.pos ) );
    return TOOL_NONE;
  }

  m_settings.Set( ts_AntiAlias, info.layerType == LAYER_OBJECT );
  m_p2 = info.pos;
  if ( modifiers != 0 ){
    if ( fl( TOOLMODIFIER1, modifiers ) ){

      // Expand from center
      if ( fl( TOOLMODIFIER2, modifiers ) ){
        // Center point ellipse constrained to circle
        m_p2 = AdjustTo( m_origP1, info.pos, 90, 45 );
        double angle_ = rad_angle( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );
        double radius_ = radius( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );
        int rx = static_cast<int>( cos(angle_) * radius_ );
        m_p1.x = m_origP1.x - rx;
        m_p1.y = m_origP1.y - rx;
        m_p2.x = m_origP1.x + rx;
        m_p2.y = m_origP1.y + rx;
        status.SetMainText("");
        status.SetText( StrCenterRadius( m_origP1, rx ) );
      }
      else {
        // Center-point Ellipse
        double angle_ = rad_angle( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );
        double radius_ = radius( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );

        int rx = static_cast<int>( cos( angle_ ) * radius_ );
        int ry = static_cast<int>( sin( angle_ ) * radius_ );

        m_p1.x = m_origP1.x - rx;
        m_p1.y = m_origP1.y - ry;
        m_p2.x = m_origP1.x + rx;
        m_p2.y = m_origP1.y + ry;
        status.SetMainText("Shift=Circle");
        status.SetText( StrCenterRadius( m_origP1, rx, ry) , 0 );
      }
    }
    else {
      if ( ( modifiers & TOOLMODIFIER2 ) == TOOLMODIFIER2 ){
        status.SetMainText("Ctrl=From center");
        // Constrain to circle
        m_p2 = AdjustTo( m_p1, info.pos, 90, 45 );
        m_p2 = floated(truncated(m_p2));
      }
      else {
        // No modifiers eactive
        status.SetMainText("Ctrl=From center Shift=Circle");
      }

      status.SetText( StrFromTo( m_p1, m_p2 ), 0 );
    }
  }
  return TOOL_OVERLAY;
}

ToolRefresh EllipseBehavior::Preempt(){
  if ( m_active ){
    m_active = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool EllipseBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active ){
    dc.Ellipse( TriFromRect( Rect(m_p1, m_p2) ), m_settings );
    return true;
  }
  return false;
}

IntRect EllipseBehavior::GetRefreshRect( const IntRect&, const Point& ){
  return truncated( Inflated(Rect(m_p1, m_p2), m_settings.Get( ts_LineWidth ) ) );
}

int EllipseBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_CROSSHAIR;
}

Command* EllipseBehavior::GetCommand(){
  return m_command.Retrieve();
}

unsigned int EllipseBehavior::GetStatusFieldCount(){
  return 1;
}
