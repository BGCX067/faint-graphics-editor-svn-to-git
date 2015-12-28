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

#include "rendering/faintdc.hh"
#include "tools/brush-tool.hh"
#include "util/formatting.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

static void draw_brush_overlay( FaintDC& dc, const Point& pos, const Settings& settings ){
  std::vector<IntPoint> v;
  v.push_back(floored(pos));
  dc.Stroke(v, settings);
}

class BrushCommand : public Command{
public:
  BrushCommand( const std::vector<IntPoint>& points, const Settings& settings )
    : Command( CommandType::RASTER ),
    m_settings( settings ),
    m_points( points )
  {
    finalize_swap_colors_erase_bg(m_settings);
  }

  std::string Name() const{
    return "Brush Stroke";
  }

  void Do( CommandContext& context ){
    context.GetDC().Stroke( m_points, m_settings );
  }

private:
  Settings m_settings;
  std::vector<IntPoint> m_points;
};

BrushTool::BrushTool()
  : Tool( ToolId::BRUSH )
{
  m_constrain = false;
  m_settings.Set( ts_BrushSize, 1 );
  m_settings.Set( ts_BrushShape, BrushShape::SQUARE );
  m_settings.Set( ts_FgCol, faint::DrawSource(faint::Color(0,0,0)) );
  m_settings.Set( ts_BgCol, faint::DrawSource(faint::Color(0,0,0)) );
  m_settings.Set( ts_SwapColors, false );
  m_active = false;

  // Prevent cursor being drawn at 0,0 before motion.
  m_drawCursor = false;
}

ToolResult BrushTool::Commit(){
  m_command.Set( new BrushCommand( m_points, m_settings ) );
  m_active = false;
  m_settings.Set( ts_SwapColors, false );
  m_points.clear();
  m_constrain = false;
  return TOOL_COMMIT;
}

void BrushTool::Draw( FaintDC& dc, Overlays&, const Point& p ){
  if ( m_active ){
    dc.Stroke( m_points, m_settings );
  }
  else if ( m_drawCursor ){
    draw_brush_overlay(dc, p, m_settings);
  }
}

bool BrushTool::DrawBeforeZoom(Layer) const{
  return true;
}

Command* BrushTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor BrushTool::GetCursor(const CursorPositionInfo& info) const{
  if ( outside_canvas_by(info, m_settings.Get(ts_BrushSize) / 2) ){
    return Cursor::BRUSH_OUT;
  }
  return Cursor::BRUSH;
}

IntRect BrushTool::GetRefreshRect( const IntRect&, const Point& mousePos) const{
  const int size = m_settings.Get( ts_BrushSize );
  if ( !m_active ){
    const IntPoint p( floored(mousePos) );
    return IntRect( p - size, p + size );
  }

  if ( m_points.size() == 1 ) {
    IntPoint p = m_points.back();
    return IntRect( p - size, p + size );
  }
  IntPoint p0 = m_points[m_points.size() - 1];
  IntPoint p1 = m_points[m_points.size() - 2];
  return inflated( IntRect( p0, p1 ), size );
}

ToolResult BrushTool::LeftDown( const CursorPositionInfo& info ){
  m_active = true;
  if ( RIGHT_MOUSE == mbtn(info.modifiers) ){
    m_settings.Set( ts_SwapColors, true );
  }
  else{
    m_settings.Set( ts_SwapColors, false );
  }
  m_points.clear();
  m_points.push_back( floored( info.pos ) );
  return TOOL_DRAW;
}

ToolResult BrushTool::LeftUp( const CursorPositionInfo& ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  return Commit();
}

ToolResult BrushTool::Motion( const CursorPositionInfo& info ){
  info.status->SetMainText("");
  info.status->SetText( str( info.pos ), 0 );

  IntPoint pos( floored(info.pos ) );
  m_drawCursor = true;
  if ( m_active ) {
    const bool constrainHeld( fl(TOOLMODIFIER2, info.modifiers) );
    if ( constrainHeld ) {
      if ( m_constrain ){
        constrain_pos( pos, m_origin );
      }
      else {
        m_origin = pos;
      }
    }
    m_constrain = constrainHeld;

    if ( m_points.back() != pos ){
      m_points.push_back(pos);
    }
    else {
      return TOOL_NONE;
    }
  }
  return TOOL_DRAW;
}

bool BrushTool::RefreshOnMouseOut(){
  // Prevent leaving a brush cursor dropping when mouse leaves the
  // window
  m_drawCursor = false;
  return true;
}

ToolResult BrushTool::Preempt( const CursorPositionInfo& ){
  if ( !m_active ){
    return TOOL_NONE;
  }
  return Commit();
}
