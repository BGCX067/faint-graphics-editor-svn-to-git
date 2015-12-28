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

#include "polygonbehavior.hh"
#include <string>
#include "cursors.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "commands/addobjectcommand.hh"
#include "objects/objpolygon.hh"
#include "settingid.hh"
#include "util/util.hh"
#include "util/objutil.hh"

bool ShouldSnap( int modifiers ){
  return fl( TOOLMODIFIER1, modifiers ) && !fl( TOOLMODIFIER2, modifiers );
}

PolygonCommand::PolygonCommand( const Points& points, const FaintSettings& settings )
  : Command( CMD_TYPE_RASTER ),
    m_points( points ),
    m_settings( settings )
{}

void PolygonCommand::Do( faint::Image& img ){
  FaintDC dc(img.GetBitmapRef());
  dc.Polygon( m_points.GetPointsDumb(), m_settings );
}

PolygonBehavior::PolygonBehavior()
  : ToolBehavior(T_POLYGON),
    m_active(false),
    m_mouseButton(LEFT_MOUSE)
{
  m_settings = GetPolygonSettings();
}

bool PolygonBehavior::DrawBeforeZoom(Layer layer) const{
  return layer == LAYER_RASTER;
}

ToolRefresh PolygonBehavior::LeftDoubleClick( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh PolygonBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  int mouseButton = mbtn( modifiers );
  Point pos = info.pos;
  if ( ShouldSnap( modifiers ) ){
    pos = Snap( pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
  }

  if ( !m_active ){
    m_active = true;
    m_settings.Set( ts_SwapColors, mouseButton == RIGHT_MOUSE );
    // Add the first point twice so that the polygon always contains
    // at least two points.
    m_points.Append( pos );
    return AddPoint(pos, mouseButton );
  }
  else {
     // Can only adjust to 45-degrees from the second point on,
    // i.e. if we're already active
    pos = ConstrainPoint( pos, modifiers );
  }

  if ( mouseButton == m_mouseButton ){
    m_points.AdjustBack( pos );
    return AddPoint( pos, mouseButton );
  }
  else {
    // Switched mouse button means commit
    return Commit(info.layerType, m_points, m_settings);
  }
}

ToolRefresh PolygonBehavior::LeftUp( const CursorPositionInfo&, int ){
  return TOOL_NONE;
}

ToolRefresh PolygonBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetText( StrPoint(info.pos), 0 );

  if ( !m_active ){
    status.SetMainText( "Click to start drawing a polygon." );
  }
  else {
    StrBtn btn( m_mouseButton );
    status.SetMainText( btn.This(true) + std::string(" click to add points, " ) + btn.Other(true) + std::string( " click to stop." ) );
  }

  if ( !m_active || m_points.Size() < 1 ){
    return TOOL_NONE;
  }

  Point pos = ConstrainPoint(info.pos, modifiers);
  if ( ShouldSnap( modifiers ) ) {
    const std::vector<Object*>& objects = info.canvas->GetObjects();
    pos = Snap( pos, objects, info.canvas->GetGrid() );
  }

  m_points.AdjustBack( pos );
  return TOOL_OVERLAY;
}

ToolRefresh PolygonBehavior::Preempt(){
  if ( m_active ){
    Reset();
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool PolygonBehavior::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( !m_active || m_points.Size() < 2 ){
    return false;
  }
  dc.Polygon( m_points.GetPointsDumb(), m_settings );
  return true;
}

int PolygonBehavior::GetCursor( const CursorPositionInfo& ){
  return CURSOR_SQUARE_CROSS;
}

IntRect PolygonBehavior::GetRefreshRect( const IntRect&, const Point& ){
  // Fixme: Kind of hackily grow more than linewidth to account for
  // e.g. miter-join protrusions, and it doesn't even work.
  const faint::coord inflateFactor = LITCRD(10.0) * m_settings.Get( ts_LineWidth );
  return truncated( Inflated(BoundingRect( m_points.GetTri() ),  inflateFactor ) );
}

Command* PolygonBehavior::GetCommand(){
  return m_command.Retrieve();
}

unsigned int PolygonBehavior::GetStatusFieldCount(){
  return 1;
}

void PolygonBehavior::Reset(){
  m_points.Clear();
  m_active = false;
}

Point PolygonBehavior::ConstrainPoint( const Point& p, int modifiers ){
  if ( !fl(TOOLMODIFIER2, modifiers ) ){
    return p;
  }

  std::vector<Point> pts = m_points.GetPointsDumb();

  // Constrain relative to the first point if both modifiers used
  const Point oppositePos = fl( TOOLMODIFIER1, modifiers ) ? pts[0] :
    pts[ pts.size() - 2 ];
  return AdjustTo45( oppositePos, p );
}

ToolRefresh PolygonBehavior::AddPoint( const Point& pos, int mouseButton ){
  m_points.Append( pos );
  m_mouseButton = mouseButton;
  return TOOL_OVERLAY;
}

ToolRefresh PolygonBehavior::Commit( int layerType, const Points& points, const FaintSettings& s ){
  if ( layerType == LAYER_RASTER ){
    m_command.Set( new PolygonCommand( points, s ) );
  }
  else {
    m_command.Set( new AddObjectCommand( new ObjPolygon( points, s ) ) );
  }
  Reset();
  return TOOL_COMMIT;
}
