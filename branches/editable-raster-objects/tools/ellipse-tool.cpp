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

#include "geo/grid.hh"
#include "objects/objellipse.hh"
#include "rendering/faintdc.hh"
#include "tools/ellipse-tool.hh"
#include "util/angle.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

EllipseTool::EllipseTool()
  : Tool(ToolId::ELLIPSE),
    m_p1(0,0),
    m_p2(0,0),
    m_origP1(0,0)
{
  m_active = false;
  m_settings = default_ellipse_settings();
}

void EllipseTool::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active ){
    dc.Ellipse( tri_from_rect( Rect(m_p1, m_p2) ), m_settings );
  }
}

bool EllipseTool::DrawBeforeZoom(Layer layerType) const{
  return layerType == Layer::RASTER;
}

Command* EllipseTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor EllipseTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

IntRect EllipseTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return floored( inflated(Rect(m_p1, m_p2), m_settings.Get( ts_LineWidth ) ) );
}

ToolResult EllipseTool::LeftDown( const CursorPositionInfo& info ){
  m_settings.Set( ts_AntiAlias, info.layerType == Layer::OBJECT );
  m_active = true;
  m_settings.Set( ts_SwapColors, RIGHT_MOUSE == mbtn( info.modifiers ) );

  if ( fl(TOOLMODIFIER1, info.modifiers ) ){
    m_p1 = m_origP1 = snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
  }
  else {
    m_p1 = m_origP1 = info.pos;
  }
  m_p2 = m_p1;
  return TOOL_NONE;
}

ToolResult EllipseTool::LeftUp( const CursorPositionInfo& info ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  m_active = false;

  faint::coord x0 = std::min( m_p1.x, m_p2.x );
  faint::coord x1 = std::max( m_p1.x, m_p2.x );
  faint::coord y0 = std::min( m_p1.y, m_p2.y ) ;
  faint::coord y1 = std::max( m_p1.y, m_p2.y );
  faint::coord w = x1 - x0 + 1;
  faint::coord h = y1 - y0 + 1;

  if ( w == 0 || h == 0 ){
    m_active = false;
    return TOOL_NONE;
  }
  m_command.Set( add_or_draw(new ObjEllipse(tri_from_rect(Rect(Point(x0, y0), Size(w,h))), m_settings),
    info.layerType) );
  info.status->SetMainText("");
  return TOOL_COMMIT;
}

ToolResult EllipseTool::Motion( const CursorPositionInfo& info ){
  if ( !m_active ){
    info.status->SetText( str( info.pos ) );
    return TOOL_NONE;
  }
  m_settings.Set( ts_AntiAlias, info.layerType == Layer::OBJECT );
  m_p2 = info.pos;

  const bool centerPoint = fl( TOOLMODIFIER1, info.modifiers );
  const bool constrainToCircle = fl(TOOLMODIFIER2, info.modifiers );

  if ( !centerPoint ){
    // Non-centered ellipses should always originate from the initial
    // click position, m_p1 may have been modified by a
    // (since-released) center-point constraining.
    m_p1 = m_origP1;
  }

  if ( centerPoint && constrainToCircle ) {

    m_p2 = adjust_to( m_origP1, info.pos, 90, 45 );
    double angle_ = rad_angle( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );
    double radius_ = radius( m_origP1.x, m_origP1.y, m_p2.x, m_p2.y );
    faint::coord rx = cos(angle_) * radius_;
    m_p1.x = m_origP1.x - rx;
    m_p1.y = m_origP1.y - rx;
    m_p2.x = m_origP1.x + rx;
    m_p2.y = m_origP1.y + rx;
    info.status->SetMainText("");
    info.status->SetText( str_center_radius( m_origP1, rx ) );
  }
  else if ( centerPoint ){
    Point delta = abs(m_origP1 - m_p2);
    m_p1 = m_origP1 - delta;
    m_p2 = m_origP1 + delta;
    info.status->SetMainText("Shift=Circle");
    info.status->SetText( str_center_radius( m_origP1, radii_from_point(delta)) , 0 );
  }
  else if ( constrainToCircle ){
    info.status->SetMainText("Ctrl=From center");
    m_p2 = adjust_to( m_p1, info.pos, 90, 45 );
    info.status->SetText( str_from_to( m_p1, m_p2 ), 0 );
  }
  else {
    info.status->SetMainText("Shift=Circle Ctrl=From center");
    info.status->SetText( str_from_to( m_p1, m_p2 ), 0 );
  }
  return TOOL_DRAW;
}

ToolResult EllipseTool::Preempt( const CursorPositionInfo& ){
  if ( m_active ){
    m_active = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}
