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
#include "objects/objrectangle.hh"
#include "rendering/faintdc.hh"
#include "tools/rectangle-tool.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

void adjust_status( StatusInterface& status, int modifiers ){
  const bool usingModifier = fl(TOOLMODIFIER1, modifiers) || fl(TOOLMODIFIER2, modifiers );
  status.SetMainText( usingModifier ? "": "Shift=Square, Ctrl=Snap" );
}

Point rect_snap_p1( const Point& p0, const Point& p1, const objects_t& objects, const Grid& grid ){
  const Point snapped = snap( p1, objects, grid );
  // Do not snap to points on the rectangle start point
  return snapped == p0 ? p1 : snapped;
}

Point rect_constrain( const Point& p0, const Point& p1, bool subPixel ){
  return subPixel? adjust_to(p0, p1, 90, 45 ):
    adjust_to( floated(floored(p0)), floated(floored(p1)), 90, 45 );
}

Point rect_adjust_p1( const Point& p0, const CursorPositionInfo& info, bool subPixel ){
  if ( fl(TOOLMODIFIER2, info.modifiers) ){
    return rect_constrain(p0, info.pos, subPixel);
  }
  else if ( fl(TOOLMODIFIER1, info.modifiers ) ){
    return rect_snap_p1(p0, info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() );
  }
  return info.pos;
}

RectangleTool::RectangleTool()
  : Tool(ToolId::RECTANGLE),
    m_active(false)
{
  m_settings = default_rectangle_settings();
}

void RectangleTool::Draw( FaintDC& dc, Overlays&, const Point& ){
  if ( m_active ){
    dc.Rectangle( tri_from_rect(Rect(m_p0, m_p1)), m_settings );
  }
}

bool RectangleTool::DrawBeforeZoom(Layer layer) const{
  return layer == Layer::RASTER;
}

Command* RectangleTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor RectangleTool::GetCursor(const CursorPositionInfo&) const{
  return Cursor::SQUARE_CROSS;
}

IntRect RectangleTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return floored(inflated(Rect( m_p0, m_p1 ), m_settings.Get( ts_LineWidth ) ) );
}

ToolResult RectangleTool::LeftDown( const CursorPositionInfo& info ){
  m_active = true;
  int mouseButton = mbtn( info.modifiers );
  m_settings.Set( ts_AntiAlias, info.layerType == Layer::OBJECT );
  m_settings.Set( ts_SwapColors, mouseButton == RIGHT_MOUSE );
  m_p0 = fl( TOOLMODIFIER1, info.modifiers ) ?
    snap( info.pos, info.canvas->GetObjects(), info.canvas->GetGrid() ) :
    info.pos;
  m_p1 = info.pos;
  return TOOL_NONE;
}

ToolResult RectangleTool::LeftUp( const CursorPositionInfo& info ){
  m_active = false;
  info.status->SetMainText("");
  bool subPixel = info.layerType == Layer::OBJECT;
  m_p1 = rect_adjust_p1( m_p0, info, subPixel );

  if ( info.layerType == Layer::OBJECT && m_p0 == m_p1 ){
    // Disallow 0-size object rectangles
    return TOOL_NONE;
  }

  m_command.Set(add_or_draw(new ObjRectangle(tri_from_rect(Rect(m_p0, m_p1)), m_settings),
      info.layerType));
  return TOOL_COMMIT;
}

ToolResult RectangleTool::Motion( const CursorPositionInfo& info ){
  if ( !m_active ){
    info.status->SetText( str( info.pos ) );
    return TOOL_NONE;
  }

  bool subPixel = info.layerType == Layer::OBJECT;
  m_p1 = rect_adjust_p1( m_p0, info, subPixel );
  m_settings.Set( ts_AntiAlias, subPixel );
  adjust_status( *info.status, info.modifiers );
  info.status->SetText( str_from_to( m_p0, m_p1 ), 0 );
  return TOOL_DRAW;
}

ToolResult RectangleTool::Preempt( const CursorPositionInfo& ){
  if ( m_active ){
    m_active = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}
