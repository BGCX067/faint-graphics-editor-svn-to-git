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

#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "objects/objrectangle.hh"
#include "rendering/faint-dc.hh"
#include "text/formatting.hh"
#include "tools/standard-tool.hh"
#include "util/canvas.hh"
#include "util/command-util.hh"
#include "util/convenience.hh"
#include "util/grid.hh"
#include "util/object-util.hh"
#include "util/setting-util.hh"
#include "util/tool-util.hh"

namespace faint{

static Point snap_p1(const Point& p0, const Point& p1, const objects_t& objects, const Grid& grid){
  const Point snapped = snap(p1, objects, grid);
  // Do not snap to points on the rectangle start point
  return snapped == p0 ? p1 : snapped;
}

static Point adjust_p1(const Point& p0, const PosInfo& info, bool subPixel){
  const auto p1 = info.pos;
  if (info.modifiers.Secondary()){
    return constrain_to_square(p0, p1, subPixel);
  }
  else if (info.modifiers.Primary()){
    return snap_p1(p0, p1, info.canvas.GetObjects(), info.canvas.GetGrid());
  }
  return info.pos;
}

static Settings rectangle_tool_settings(const Settings& allSettings){
  Settings s(default_rectangle_settings());
  s.Update(allSettings);
  return s;
}

class RectangleTool : public StandardTool {
public:
  RectangleTool(const Settings& allSettings)
    : StandardTool(ToolId::RECTANGLE, rectangle_tool_settings(allSettings)),
      m_active(false)
  {}

  void Draw(FaintDC& dc, Overlays&, const Point&) override{
    if (m_active){
      dc.Rectangle(tri_from_rect(Rect(m_p0, m_p1)), GetSettings());
    }
  }

  bool DrawBeforeZoom(Layer layer) const override{
    return layer == Layer::RASTER;
  }

  Command* GetCommand() override{
    return m_command.Retrieve();
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::SQUARE_CROSS;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return padded(floored(Rect(m_p0, m_p1)), get_padding(GetSettings()));
  }

  ToolResult MouseDown(const PosInfo& info) override{
    m_active = true;
    SetAntiAlias(info);
    SetSwapColors(info.modifiers.RightMouse());
    m_p0 = info.modifiers.Primary() ?
      snap(info.pos, info.canvas.GetObjects(), info.canvas.GetGrid()) :
      info.pos;
    m_p1 = info.pos;
    return TOOL_NONE;
  }

  ToolResult MouseUp(const PosInfo& info) override{
    m_active = false;
    info.status.SetMainText("");
    bool subPixel = info.layerType == Layer::OBJECT;
    m_p1 = adjust_p1(m_p0, info, subPixel);

    if (info.layerType == Layer::OBJECT && m_p0 == m_p1){
      // Disallow 0-size object rectangles
      return TOOL_NONE;
    }

    const auto tri = tri_from_rect({m_p0, m_p1});
    m_command.Set(add_or_draw(create_rectangle_object(tri, GetSettings()),
      info.layerType));
    return TOOL_COMMIT;
  }

  ToolResult MouseMove(const PosInfo& info) override{
    if (!m_active){
      info.status.SetText(str(info.pos));
      return TOOL_NONE;
    }

    bool subPixel = info.layerType == Layer::OBJECT;
    m_p1 = adjust_p1(m_p0, info, subPixel);
    SetAntiAlias(info);
    info.status.SetMainText(info.modifiers.Either() ?
      "": "Shift=Square, Ctrl=Snap");
    info.status.SetText(str_from_to(m_p0, m_p1), 0);
    return TOOL_DRAW;
  }

  ToolResult Preempt(const PosInfo&){
    if (then_false(m_active)){
      return TOOL_CANCEL;
    }
    return TOOL_NONE;
  }

private:
  bool m_active;
  Point m_p0;
  Point m_p1;
  PendingCommand m_command;
};

Tool* rectangle_tool(const Settings& allSettings){
  return new RectangleTool(allSettings);
}

} // namespace
