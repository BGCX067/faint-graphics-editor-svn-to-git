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

#include "geo/point.hh"
#include "rendering/faintdc.hh"
#include "tools/pen-tool.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"

class PenCommand : public Command {
public:
  PenCommand( const std::vector<IntPoint>& points, const Settings& settings )
    : Command( CommandType::RASTER ),
      m_settings( settings ),
      m_points( points )
  {
    finalize_swap_colors_erase_bg(m_settings);
  }

  void Do( CommandContext& context ){
    context.GetDC().Stroke( m_points, m_settings );
  }

  std::string Name() const{
    return "Pen Stroke";
  }
private:
  Settings m_settings;
  std::vector<IntPoint> m_points;
};

static bool pen_constrain_held( int modifiers ){
  return (modifiers & TOOLMODIFIER2 ) == TOOLMODIFIER2;
}

PenTool::PenTool()
  : Tool(ToolId::PEN)
{
  m_active = false;
  m_constrainDir = constrain_dir::NONE;
  m_settings.Set( ts_FgCol, faint::DrawSource(faint::Color( 0, 0, 0 ) ));
    m_settings.Set( ts_BgCol, faint::DrawSource(faint::Color( 0, 0, 0 ) ));
  m_settings.Set( ts_SwapColors, false );
}

ToolResult PenTool::Commit(){
  m_command.Set( new PenCommand( m_points, m_settings ) );
  m_active = false;
  m_settings.Set( ts_SwapColors, false );
  m_points.clear();
  m_constrainDir = constrain_dir::NONE;
  return TOOL_COMMIT;
}

void PenTool::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active ){
    dc.Stroke( m_points, m_settings );
  }
}

bool PenTool::DrawBeforeZoom(Layer) const{
  return true;
}

Command* PenTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor PenTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::PEN;
}

IntRect PenTool::GetRefreshRect( const IntRect&, const Point& ) const{
  if ( m_points.empty() ){
    return IntRect( IntPoint(0,0), IntSize(0,0) );
  }
  else if ( m_points.size() == 1 ){
    return IntRect( m_points[0], IntSize(1,1));
  }
  else {
    IntPoint p0 = m_points[ m_points.size() -1 ];
    IntPoint p1 = m_points[ m_points.size() -2 ];
    return IntRect( p0, p1 );
  }
}

ToolResult PenTool::LeftDown( const CursorPositionInfo& info ){
  m_active = true;
  m_origin = floored(info.pos);
  m_constrainDir = constrain_dir::NONE;
  m_settings.Set( ts_SwapColors, mbtn(info.modifiers) == RIGHT_MOUSE );
  m_points.clear();
  m_points.push_back( m_origin );
  return TOOL_DRAW;
}

ToolResult PenTool::LeftUp( const CursorPositionInfo& ){
  if ( !m_active ){
    return TOOL_NONE;
  }

  return Commit();
}

ToolResult PenTool::Motion( const CursorPositionInfo& info ){
  Point pos = info.pos;
  info.status->SetText( str( pos ), 0 );

  if ( !m_active ){
    m_constrainDir = constrain_dir::NONE;
    return TOOL_NONE;
  }

  if ( pen_constrain_held( info.modifiers ) ) {
    if ( m_constrainDir == constrain_dir::NONE ){
      if ( distance( floored(pos), m_origin ) > 5 ){
        // Lock the constrain direction after a certain distance
        // to avoid weird switches
        m_constrainDir = constrain_pos( pos, floated(m_origin) );
      }
      else {
        constrain_pos( pos, floated(m_origin) );
      }
    }
    else {
      constrain_pos( pos, floated(m_origin), m_constrainDir );
    }
  }
  else {
    m_origin = floored(pos);
    m_constrainDir = constrain_dir::NONE;
  }

  IntPoint iPos( floored(pos) );

  if ( m_points[ m_points.size() - 1 ] != iPos ){
    m_points.push_back( iPos );
  }
  return TOOL_DRAW;
}

ToolResult PenTool::Preempt( const CursorPositionInfo& ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  return Commit();
}
