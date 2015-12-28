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
#include "util/toolutil.hh"
#include "util/util.hh"

PickerTool::PickerTool()
  : Tool( T_PICKER )
{}

bool PickerTool::Draw( FaintDC&, Overlays&, const Point& ){
  return false;
}

bool PickerTool::DrawBeforeZoom(Layer::type) const{
  return false;
}

Command* PickerTool::GetCommand(){
  assert( false );
  return 0;
}

Cursor::type PickerTool::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::PICKER;
}

IntRect PickerTool::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

ToolResult PickerTool::LeftDown( const CursorPositionInfo& info ){
  if ( outside_canvas(info) ){
    return TOOL_NONE;
  }
  const ColorSetting colorId = mbtn( info.modifiers ) == LEFT_MOUSE ?
    ts_FgCol : ts_BgCol; // Fixme: Duplicates FillTool

  faint::Color color = get_hovered_color( info );
  GetAppContext().Set(colorId, color );
  return TOOL_NONE;
}

ToolResult PickerTool::LeftUp( const CursorPositionInfo& ){
  return TOOL_NONE;
}

ToolResult PickerTool::Motion( const CursorPositionInfo& info ){
  // Fixme: Duplicates FillTool
  if ( outside_canvas(info) ){
    info.status->SetMainText("");
    info.status->SetText("");
    return TOOL_NONE;
  }

  faint::Color col(get_hovered_color(info)); // Fixme: Ignored inside color of un-filled objects! (Different from fill!)
  info.status->SetText(space_sep(str_smart_rgba(col), bracketed(str(info.pos))));
  return TOOL_NONE;
}

ToolResult PickerTool::Preempt( const CursorPositionInfo& ){
  return TOOL_NONE;
}
