// -*- coding: us-ascii-unix -*-
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
#include "app/get-app-context.hh"
#include "bitmap/bitmap.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "text/formatting.hh"
#include "tools/standard-tool.hh"
#include "util/canvas.hh"
#include "util/pattern.hh"
#include "util/tool-util.hh"

namespace faint{

static bool should_pick_to_pattern(const PosInfo& info){
  return info.modifiers.Primary();
}

static bool should_anchor_topleft(const PosInfo& info){
  return info.modifiers.Secondary();
}

static Paint pattern_get_hovered_paint(const PosInfo& info){
  // Do not pick invisible object insides. Include the floating
  // selection if hovered
  return get_hovered_paint(info,
    include_hidden_fill(false),
    include_floating_selection(true));
}

class PickerTool : public StandardTool {
public:
  PickerTool()
    : StandardTool(ToolId::PICKER, Settings())
  {}

  void Draw(FaintDC&, Overlays&, const Point&) override{
  }

  bool DrawBeforeZoom(Layer) const override{
    return false;
  }

  Command* GetCommand() override{
    assert(false);
    return nullptr;
  }

  Cursor GetCursor(const PosInfo& info) const override{
    if (should_pick_to_pattern(info)){
      return should_anchor_topleft(info) ?
        Cursor::PICKER_PATTERN_TOPLEFT : Cursor::PICKER_PATTERN;
    }
    return Cursor::PICKER;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return IntRect::EmptyRect();
  }

  ToolResult MouseDown(const PosInfo& info) override{
    if (outside_canvas(info)){
      return TOOL_NONE;
    }
    const ColorSetting colorId = fg_or_bg(info);

    const auto& bg = info.canvas.GetBitmap();
    if (should_pick_to_pattern(info) && bg.IsSet()){
      bool anchorTopLeft = should_anchor_topleft(info);
      IntPoint anchor = anchorTopLeft ?
        IntPoint(0,0) : // Image top left
        floored(info.pos);
      Pattern pattern(bg.Get(),
        anchor, object_aligned_t(!anchorTopLeft));
      get_app_context().Set(colorId, Paint(pattern));
      return TOOL_NONE;
    }

    get_app_context().Set(colorId, pattern_get_hovered_paint(info));
    return TOOL_NONE;
  }

  ToolResult MouseUp(const PosInfo&) override{
    return TOOL_NONE;
  }

  ToolResult MouseMove(const PosInfo& info) override{
    if (outside_canvas(info)){
      info.status.SetMainText("");
      info.status.SetText("");
      return TOOL_NONE;
    }

    if (should_pick_to_pattern(info)){
      info.status.SetMainText("Click to use image as pattern");
      info.status.SetText(should_anchor_topleft(info) ?
        "Anchor: Top Left (0,0)" :
        space_sep("Anchor:", bracketed(str((info.pos)))));
      return TOOL_NONE;
    }

    Paint paint(pattern_get_hovered_paint(info));
    info.status.SetMainText("");
    info.status.SetText(space_sep(str(paint), bracketed(str(info.pos))));
    return TOOL_NONE;
  }

  ToolResult Preempt(const PosInfo&) override{
    return TOOL_NONE;
  }
};

Tool* picker_tool(){
  return new PickerTool();
}

} // namespace
