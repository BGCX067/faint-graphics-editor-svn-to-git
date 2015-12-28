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

#include <cassert>
#include <string>
#include "app/getappcontext.hh"
#include "bitmap/bitmap.hh"
#include "objects/object.hh"
#include "tools/picker-tool.hh"
#include "util/formatting.hh"
#include "util/gradient.hh"
#include "util/pattern.hh"
#include "util/toolutil.hh"
#include "util/util.hh"

static bool should_pick_to_pattern( const CursorPositionInfo& info ){
  return fl(TOOLMODIFIER1, info.modifiers);
}

static bool should_anchor_topleft( const CursorPositionInfo& info ){
  return fl(TOOLMODIFIER2, info.modifiers);
}

PickerTool::PickerTool()
  : Tool( ToolId::PICKER )
{}

void PickerTool::Draw( FaintDC&, Overlays&, const Point& ){
}

bool PickerTool::DrawBeforeZoom(Layer) const{
  return false;
}

Command* PickerTool::GetCommand(){
  assert( false );
  return nullptr;
}

Cursor PickerTool::GetCursor( const CursorPositionInfo& info ) const{
  if ( should_pick_to_pattern(info) ){
    return should_anchor_topleft(info) ?
      Cursor::PICKER_PATTERN_TOPLEFT : Cursor::PICKER_PATTERN;
  }
  return Cursor::PICKER;
}

IntRect PickerTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

ToolResult PickerTool::LeftDown( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    return TOOL_NONE;
  }
  const ColorSetting colorId = fg_or_bg(info);

  if ( should_pick_to_pattern(info) ){
    bool anchorTopLeft = should_anchor_topleft(info);
    IntPoint anchor = anchorTopLeft ?
      IntPoint(0,0) : // Image top left
      floored(info.pos);
    faint::Pattern pattern(info.canvas->GetBitmap(),
      anchor, object_aligned_t(!anchorTopLeft));
    GetAppContext().Set(colorId, faint::DrawSource(pattern));
    return TOOL_NONE;
  }

  faint::DrawSource src(get_hovered_draw_source(info, hidden_fill(false)));
  GetAppContext().Set(colorId, src);
  return TOOL_NONE;
}

ToolResult PickerTool::LeftUp( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult PickerTool::Motion( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    info.status->SetMainText("");
    info.status->SetText("");
    return TOOL_NONE;
  }

  if ( should_pick_to_pattern(info) ){
    info.status->SetMainText("Click to use image as pattern");
    info.status->SetText( should_anchor_topleft(info) ?
      "Anchor: Top Left (0,0)" :
      space_sep("Anchor:", bracketed(str((info.pos)))));
    return TOOL_NONE;
  }

  faint::DrawSource src(get_hovered_draw_source(info, hidden_fill(false)));
  info.status->SetMainText("");
  info.status->SetText(space_sep(str(src), bracketed(str(info.pos))));
  return TOOL_NONE;
}

ToolResult PickerTool::Preempt( const CursorPositionInfo& ){
  return TOOL_NONE;
}
