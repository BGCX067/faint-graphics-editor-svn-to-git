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

#include "commands/cmdchangesetting.hh"
#include "rendering/faintdc.hh"
#include "tools/fill-tool.hh"
#include "util/commandutil.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

bool no_fillable_object_hit(const CursorPositionInfo& info ){
  return !object_color_region_hit(info) || is_raster_object(info.object);
}

std::string hit_description( const CursorPositionInfo& info ){
  if ( !info.object->GetSettings().Has(ts_BgCol ) ){
    return "color";
  }
  else {
    return info.hitStatus == HIT_BOUNDARY ? "edge color" : "fill color";
  }
}

FillTool::FillTool()
  : Tool(T_FLOODFILL)
{
  m_settings.Set( ts_FgCol, faint::Color(0,0,0) );
  m_settings.Set( ts_BgCol, faint::Color(0,0,0) );
}

bool FillTool::Draw( FaintDC&, Overlays&, const Point& ){
  return false;
}

bool FillTool::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* FillTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor::type FillTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::FILL;
}

IntRect FillTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

ToolResult FillTool::LeftDown( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    return TOOL_NONE;
  }

  const faint::Color fillColor = m_settings.Get(LEFT_MOUSE == mbtn(info.modifiers) ?
    ts_FgCol : ts_BgCol );

  if ( is_raster(info.layerType) || no_fillable_object_hit(info) ){
    // Raster-layer flood fill
    m_command.Set( get_flood_fill_command( truncated(info.pos), fillColor ) );
  }
  else if ( is_text_object(info.object) ){
    // Text objects don't quite support a background color, and especially not a fill
    // style, so handle them separately
    m_command.Set( get_fill_boundary_command(info.object, fillColor) );
  }
  else if ( info.hitStatus == HIT_BOUNDARY ){
    m_command.Set( get_fill_boundary_command(info.object, fillColor) );
  }
  else if ( info.hitStatus == HIT_INSIDE ){
    // Note: DWIM would possible here if the object's fillstyle is BORDER
    // (transparent inside) but that could mean any object below
    // should be filled, or the raster background, so it's tricky...
    m_command.Set( get_fill_inside_command(info.object, fillColor ) );
  }
  else {
    assert( false );
  }
  return TOOL_COMMIT;
}

ToolResult FillTool::LeftUp( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult FillTool::Motion( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    info.status->SetMainText("");
    info.status->SetText(bracketed(str(info.pos)));
    return TOOL_NONE;
  }

  if ( is_object(info.layerType) && object_color_region_hit(info) ){
    info.status->SetMainText(space_sep("Click to set the", info.object->GetType(), hit_description(info) ) );
  }
  else{
    info.status->SetMainText("");
  }
  info.status->SetText(space_sep(str_smart_rgba( get_hovered_color(info) ), bracketed(str(info.pos))));
  return TOOL_NONE;
}

ToolResult FillTool::Preempt( const CursorPositionInfo& ){
  return TOOL_NONE;
}
