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
#include "objects/objline.hh"
#include "rendering/faintdc.hh"
#include "tools/line-tool.hh"
#include "util/angle.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

Point line_constrain( const Point& p0, const Point& p1, bool subPixel ){
  return subPixel ?
    adjust_to_45( p0, p1 ) :
    adjust_to_45( floated(truncated(p0)), floated(truncated(p1)) );
}

void line_status_bar( StatusInterface& status, const Point& p1, const Point& p2, int modifiers ){
  if ( (modifiers & TOOLMODIFIER2) == TOOLMODIFIER2 ) {
    status.SetMainText("");
  }
  else if ( ( modifiers & TOOLMODIFIER1 ) == TOOLMODIFIER1 ){
    status.SetMainText("");
  }
  else {
    status.SetMainText( "Ctrl=Snap Shift=Constrain" );
  }

  status.SetText( str_line_status( p1, p2 ), 0 );
}

Settings clean_line_object_settings( const Settings& oldSettings ){
  Settings s(remove_background_color(oldSettings));
  s.Erase( ts_PolyLine );
  return s;
}

LineTool::LineTool()
  : Tool( T_LINE ),
    m_otherButton(RIGHT_MOUSE)
{
  m_settings = default_line_settings();
  m_settings.Set(ts_PolyLine,false);
  m_settings.Set(ts_BgCol,faint::Color(0,0,0));
  m_active = false;
}

bool LineTool::AllowsGlobalRedo() const{
  return !m_active;
}

bool LineTool::CanRedo() const{
  return m_active && m_states.CanRedo();
}

bool LineTool::CanUndo() const{
  return m_active && m_states.CanUndo();
}

ToolResult LineTool::Char( const KeyInfo& info){
  if ( !m_active || m_settings.Not(ts_PolyLine) ){
    return TOOL_NONE;
  }
  if ( info.keyCode == key::esc ){
    // Commit a drawn polyline on escape
    // (though the preferred method is right click)
    m_command.Set(CreateCommand(info.layerType));
    return TOOL_COMMIT;
  }
  return TOOL_NONE;
}

Command* LineTool::CreateCommand( Layer::type layer ){
  m_active = false;
  std::vector<Point> points(m_points);
  m_points.clear();
  m_states.Clear();
  if ( points.size() < 2 ){
    return 0; // Fixme: Prevent this case
  }

  if ( layer == Layer::OBJECT && points.size() == 2 && points[0] == points[1] ){
    // Prevent 0-length object lines, as they're invisible (Such
    // raster lines are allowed - it's the same as drawing a pixel)
    return 0;
  }

  return add_or_draw( new ObjLine(Points(points), clean_line_object_settings(m_settings)),
    layer );
}

bool LineTool::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( !m_active ){
    return false;
  }
  dc.PolyLine( m_points, clean_line_object_settings(m_settings) );
  return true;
}

bool LineTool::DrawBeforeZoom( Layer::type layer ) const{
  return layer == Layer::RASTER;
}

Command* LineTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type LineTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::CROSSHAIR;
}

std::string LineTool::GetRedoName() const{
  return "Add Point";
}

IntRect LineTool::GetRefreshRect( const IntRect& visible, const Point& ) const{
  if ( m_points.size() == 2 ){
    const Point& p0 = m_points[0];
    const Point& p1 = m_points[1];
    faint::coord extraWidth = m_settings.Get( ts_LineWidth );
    if ( m_settings.Get( ts_LineArrowHead ) ){
      extraWidth *= LITCRD(10.0);
    }
    return truncated( inflated( Rect( p0, p1 ), extraWidth ) );
  }
  else {
    // Fixme
    return visible;
  }
}

std::string LineTool::GetUndoName() const{
  return "Add Point";
}

ToolResult LineTool::LeftDown( const CursorPositionInfo& info ){
  return UpdatePoints( info, !m_active, true );
}

ToolResult LineTool::LeftUp( const CursorPositionInfo& info ){
  if ( m_settings.Get(ts_PolyLine) || !m_active ){
    return TOOL_NONE;
  }
  m_command.Set( CreateCommand(info.layerType) );
  return m_command.Valid() ? TOOL_COMMIT : TOOL_NONE;
}

ToolResult LineTool::Motion( const CursorPositionInfo& info ){
  if ( !m_active ){
    info.status->SetMainText("");
    info.status->SetText( str( info.pos ), 0 );
    return TOOL_NONE;
  }
  UpdatePoints(info, false, false );
  return TOOL_DRAW;
}

ToolResult LineTool::Preempt( const CursorPositionInfo& info ){
  if ( m_active ){
    m_command.Set( CreateCommand(info.layerType) );
    return m_command.Valid() ? TOOL_COMMIT : TOOL_CANCEL;
  }
  return TOOL_NONE;
}

void LineTool::Redo(){
  m_points.pop_back();
  m_points.push_back(m_states.Redo());
  m_points.push_back(m_points.back()); // Cursor position
}

void LineTool::Undo(){
  m_points.pop_back();
  m_states.Undo();
}

ToolResult LineTool::UpdatePoints( const CursorPositionInfo& info, bool first, bool click ){
  if ( first ){
    m_active = true;
    bool rmb = fl(RIGHT_MOUSE, info.modifiers);
    m_otherButton = rmb? LEFT_MOUSE : RIGHT_MOUSE;
    m_settings.Set(ts_SwapColors, rmb);
  }

  const bool subPixel = is_object(info.layerType);
  m_settings.Set( ts_AntiAlias, subPixel );
  Point newPoint(info.pos);
  if ( first ){
    m_settings.Set( ts_SwapColors, mbtn(info.modifiers) == RIGHT_MOUSE );
    if ( fl(TOOLMODIFIER1, info.modifiers ) ) {
      newPoint = snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
    }
  }
  else {
    if ( fl(TOOLMODIFIER2, info.modifiers ) ) {
      if ( m_points.size() > 1 ){
	const Point& pt = m_points[m_points.size() - 2];
	newPoint = line_constrain( pt, info.pos, subPixel );
      }
    }
    else if ( fl(TOOLMODIFIER1, info.modifiers ) ){
      newPoint = snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
    }
  }

  if ( click ){
    if ( m_settings.Get(ts_PolyLine) && fl(m_otherButton, info.modifiers) ){
      m_command.Set(CreateCommand(info.layerType));
      return m_command.Valid() ? TOOL_COMMIT : TOOL_NONE;
    }
    m_points.push_back(newPoint);
    m_states.Did(newPoint);
    if ( first ){
      m_points.push_back(newPoint);
    }
  }
  else {
    m_points.back() = newPoint;
  }
  line_status_bar( *info.status, m_points[0], m_points[1], info.modifiers );
  return TOOL_DRAW;
}
