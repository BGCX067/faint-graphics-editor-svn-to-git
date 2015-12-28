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

#include "commands/change-setting-cmd.hh"
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
  if ( !info.object->GetSettings().Has(ts_BgCol) ){
    return "color";
  }
  else {
    return info.hitStatus == Hit::BOUNDARY ? "edge color" : "fill color";
  }
}

static faint::Color fill_tool_get_replace_color( const CursorPositionInfo& info ){
  const faint::Bitmap& bmp( info.canvas->GetBitmap() );
  faint::Color color(get_color(bmp, floored(info.pos) ));
  return color;
}

static bool fill_replace_flag( int modifiers ){
  return fl(TOOLMODIFIER1, modifiers) && !fl(TOOLMODIFIER2, modifiers);
}

static bool fill_replace_other_flag( int modifiers ){
  return fl(TOOLMODIFIER1, modifiers) && fl(TOOLMODIFIER2, modifiers);
}

FillTool::FillTool()
  : Tool(ToolId::FLOOD_FILL)
{
  m_settings.Set( ts_FgCol, faint::DrawSource(faint::Color(0,0,0)) );
  m_settings.Set( ts_BgCol, faint::DrawSource(faint::Color(0,0,0)) );
}

void FillTool::Draw( FaintDC&, Overlays&, const Point& ){
}

bool FillTool::DrawBeforeZoom(Layer) const{
  return false;
}

Command* FillTool::GetCommand(){
  return m_command.Retrieve();
}

Cursor FillTool::GetCursor( const CursorPositionInfo& info ) const{
  if ( fill_replace_flag(info.modifiers) ){
    return Cursor::BUCKET_REPLACE;
  }
  else if ( fill_replace_other_flag(info.modifiers) ){
    return Cursor::BUCKET_REPLACE_OTHER;
  }
  return Cursor::BUCKET;
}

IntRect FillTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

ToolResult FillTool::LeftDown( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    return TOOL_NONE;
  }

  const faint::DrawSource fill = m_settings.Get(fg_or_bg(info));
  if ( is_raster(info.layerType) || no_fillable_object_hit(info) ){
    // Raster-layer flood fill
    if ( fill_replace_flag(info.modifiers) ){
      faint::Color clickedColor(fill_tool_get_replace_color(info));
      if ( fill == clickedColor ){
	// No reason to replace with same color.
        return TOOL_NONE;
      }
      else {
        m_command.Set( get_replace_color_command(Old(clickedColor), fill) );
      }
    }
    else if ( fill_replace_other_flag(info.modifiers ) ){
      faint::Color clickedColor(fill_tool_get_replace_color(info));
      if ( fill == clickedColor ){
        // This would clear the image, and would be a strange way to clear
        // an image, so do nothing.
        return TOOL_NONE;
      }
      else {
        m_command.Set( get_erase_but_color_command( clickedColor, fill ) );
      }
    }
    else {
      m_command.Set( get_flood_fill_command( floored(info.pos), fill ) );
    }
  }
  else if ( is_text_object(info.object) ){
    // Text objects don't quite support a background color, and
    // especially not a fill style, so handle them separately
    m_command.Set( get_fill_boundary_command(info.object, fill) );
  }
  else if ( info.hitStatus == Hit::BOUNDARY ){
    m_command.Set( get_fill_boundary_command(info.object, fill) );
  }
  else if ( info.hitStatus == Hit::INSIDE ){
    // Note: DWIM would possible here if the object's fillstyle is BORDER
    // (transparent inside) but that could mean any object below
    // should be filled, or the raster background, so it's tricky...
    m_command.Set( get_fill_inside_command(info.object, fill ) );
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
    if ( fill_replace_flag( info.modifiers ) ){
      info.status->SetMainText("Click to replace color");
    }
    else if ( fill_replace_other_flag( info.modifiers ) ){
      info.status->SetMainText("Click to replace all other colors");
    }
    else{
      info.status->SetMainText("");
    }
  }
  info.status->SetText(space_sep(str(get_hovered_draw_source(info, hidden_fill(true)) ), bracketed(str(info.pos))));
  return TOOL_NONE;
}

ToolResult FillTool::Preempt( const CursorPositionInfo& ){
  return TOOL_NONE;
}
