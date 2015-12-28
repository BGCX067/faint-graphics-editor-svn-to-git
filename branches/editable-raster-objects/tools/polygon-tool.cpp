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
#include "objects/objpolygon.hh"
#include "polygon-tool.hh"
#include "rendering/faintdc.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

static bool should_snap( int modifiers ){
  return fl( TOOLMODIFIER1, modifiers ) && !fl( TOOLMODIFIER2, modifiers );
}

static std::vector<Point> get_point_intersections( const Points& in_points, size_t pointIndex ){
  std::vector<Point> points(in_points.GetPointsDumb());
  size_t numPoints = points.size();
  if ( numPoints <= 1 ){
    return std::vector<Point>();
  }
  if ( pointIndex >= numPoints ){
    return std::vector<Point>();
  }
  assert( pointIndex < numPoints );
  const bool firstPoint = pointIndex == 0;
  const bool lastPoint = pointIndex == numPoints - 1;
  Point p0 = firstPoint ? points[numPoints - 1] : points[pointIndex - 1];
  Point p1 = lastPoint ? points[0] : points[pointIndex + 1];
  std::vector<Point> pts;
  pts.push_back(Point(p0.x, p1.y)); // Corner
  pts.push_back(Point(p1.x, p0.y)); // Corner

  pts.push_back(Point(p1.y - ( p0.y - p0.x), p1.y )); // Diagonal to horizontal
  pts.push_back(Point((p0.y + p0.x) - p1.y, p1.y )); // Diagonal to horizontal

  pts.push_back(Point(p0.y - ( p1.y - p1.x), p0.y )); // Diagonal to horizontal
  pts.push_back(Point((p1.y + p1.x) - p0.y, p0.y )); // Diagonal to horizontal

  pts.push_back(Point(p1.x, p1.x + (p0.y - p0.x))); // Diagonal to vertical
  pts.push_back(Point(p1.x, -p1.x + (p0.y + p0.x))); // Diagonal to vertical

  pts.push_back(Point(p0.x, p0.x + (p1.y - p1.x))); // Diagonal to vertical
  pts.push_back(Point(p0.x, -p0.x + (p1.y + p1.x))); // Diagonal to vertical

  // Diagonal to diagonal
  const double m0 = p0.y - p0.x;
  const double m1 = p1.y + p1.x;
  const double x0 = ( m0 + m1 ) / 2.0 - m0;
  const double y0 = x0 + m0;
  Point snapPt( x0, y0 );
  pts.push_back(snapPt);
  pts.push_back(p1 + (p0 - snapPt));
  return pts;
}

static Point polygon_snap( const Point& pos, CanvasInterface& canvas, const Points& points ){
  return snap(pos, canvas.GetObjects(), canvas.GetGrid(),
    get_point_intersections(points, points.Size() - 1));
}

PolygonTool::PolygonTool()
  : Tool(ToolId::POLYGON),
    m_active(false),
    m_mouseButton(LEFT_MOUSE)
{
  m_settings = default_polygon_settings();
}

ToolResult PolygonTool::AddPoint( const Point& pos, int mouseButton ){
  m_points.Append(pos);
  m_states.Did(pos);
  m_mouseButton = mouseButton;
  return TOOL_DRAW;
}

bool PolygonTool::AllowsGlobalRedo() const{
  return !m_active;
}

bool PolygonTool::CanRedo() const{
  return m_active && m_states.CanRedo();
}

bool PolygonTool::CanUndo() const{
  return m_active && m_states.CanUndo();
}

ToolResult PolygonTool::Commit( Layer layerType ){
  m_command.Set(add_or_draw(new ObjPolygon(m_points, m_settings), layerType));
  Reset();
  return TOOL_COMMIT;
}

Point PolygonTool::ConstrainPoint( const Point& p, int modifiers ){
  if ( !fl(TOOLMODIFIER2, modifiers ) ){
    return p;
  }

  std::vector<Point> pts = m_points.GetPointsDumb();

  // Constrain relative to the first point if both modifiers used
  const Point oppositePos = fl( TOOLMODIFIER1, modifiers ) ? pts[0] :
    pts[ pts.size() - 2 ];
  return adjust_to_45( oppositePos, p );
}

void PolygonTool::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active && m_points.Size() >= 2 ){
    dc.Polygon( m_points.GetTri(), m_points.GetPointsDumb(), m_settings );
  }
}

bool PolygonTool::DrawBeforeZoom(Layer layer) const{
  return layer == Layer::RASTER;
}

Command* PolygonTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor PolygonTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::SQUARE_CROSS;
}

std::string PolygonTool::GetRedoName() const{
  return "Add Point";
}

IntRect PolygonTool::GetRefreshRect( const IntRect&, const Point& ) const{
  // Fixme: Kind of hackily grow more than linewidth to account for e.g. miter-join protrusions, and it doesn't even work.
  const faint::coord inflateFactor = LITCRD(10.0) * m_settings.Get( ts_LineWidth );
  return floored( inflated(bounding_rect( m_points.GetTri() ),  inflateFactor ) );
}

std::string PolygonTool::GetUndoName() const{
  return "Add Point";
}

ToolResult PolygonTool::LeftDoubleClick( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult PolygonTool::LeftDown( const CursorPositionInfo& info ){
  m_settings.Set( ts_AntiAlias, info.layerType == Layer::OBJECT );
  int mouseButton = mbtn( info.modifiers );
  Point pos = info.pos;
  if ( should_snap( info.modifiers ) ){
    pos = polygon_snap(pos, *info.canvas, m_points);
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
    pos = ConstrainPoint( pos, info.modifiers );
  }

  if ( mouseButton == m_mouseButton ){
    m_points.AdjustBack( pos );
    return AddPoint( pos, mouseButton );
  }
  else {
    // Switched mouse button means commit
    return Commit(info.layerType);
  }
}

ToolResult PolygonTool::LeftUp( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult PolygonTool::Motion( const CursorPositionInfo& info ){
  info.status->SetText( str(info.pos), 0 );

  if ( !m_active ){
    info.status->SetMainText( "Click to start drawing a polygon." );
  }
  else {
    StrBtn btn( m_mouseButton );
    info.status->SetMainText(space_sep(btn.This(true), "click to add points,",
	btn.Other(true), "click to stop."));
  }
  if ( !m_active || m_points.Size() < 1 ){
    return TOOL_NONE;
  }

  Point pos = ConstrainPoint(info.pos, info.modifiers);
  if ( should_snap( info.modifiers ) ) {
    pos = polygon_snap( pos, *info.canvas, m_points );
  }

  m_points.AdjustBack( pos );
  return TOOL_DRAW;
}

ToolResult PolygonTool::Preempt( const CursorPositionInfo& info ){
  if ( m_active ){
    return Commit( info.layerType );
  }
  return TOOL_NONE;
}

void PolygonTool::Redo(){
  m_points.PopBack();
  Point redone = m_states.Redo();
  m_points.Append(redone);
  m_points.Append(redone); // Cursor pos
}

void PolygonTool::Reset(){
  m_active = false;
  m_points.Clear();
  m_states.Clear();
}

void PolygonTool::Undo(){
  assert(m_states.CanUndo());
  m_points.PopBack();
  m_states.Undo();
}
