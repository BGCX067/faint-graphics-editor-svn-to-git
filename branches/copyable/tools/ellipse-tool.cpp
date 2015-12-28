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

#include <algorithm>
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "geo/radii.hh"
#include "geo/size.hh"
#include "objects/objellipse.hh"
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

static Settings ellipse_settings(const Settings& allSettings){
  Settings s(default_ellipse_settings());
  s.Update(allSettings);
  return s;
}

class EllipseTool : public StandardTool {
public:
  EllipseTool(const Settings& allSettings)
    : StandardTool(ToolId::ELLIPSE, ellipse_settings(allSettings)),
      m_p1(0,0),
      m_p2(0,0),
      m_origP1(0,0)
  {
    m_active = false;
  }

  void Draw(FaintDC& dc, Overlays&, const Point&) override{
    if (m_active){
      dc.Ellipse(tri_from_rect(Rect(m_p1, m_p2)), GetSettings());
    }
  }

  bool DrawBeforeZoom(Layer layer) const override{
    return layer == Layer::RASTER;
  }

  Command* GetCommand() override{
    return m_command.Retrieve();
  }

  Cursor GetCursor(const PosInfo&) const override{
    return Cursor::CROSSHAIR;
  }

  IntRect GetRefreshRect(const IntRect&, const Point&) const override{
    return padded(floored(Rect(m_p1, m_p2)), get_padding(GetSettings()));
  }

  ToolResult MouseDown(const PosInfo& info) override{
    SetAntiAlias(info);
    SetSwapColors(info.modifiers.RightMouse());
    m_active = true;

    if (info.modifiers.Primary()){
      m_p1 = m_origP1 = snap(info.pos, info.canvas.GetObjects(),
        info.canvas.GetGrid());
    }
    else {
      m_p1 = m_origP1 = info.pos;
    }
    m_p2 = m_p1;
    return TOOL_DRAW;
  }

  ToolResult MouseUp(const PosInfo& info) override{
    if (!m_active){
      return TOOL_NONE;
    }
    m_active = false;

    coord x0 = std::min(m_p1.x, m_p2.x);
    coord x1 = std::max(m_p1.x, m_p2.x);
    coord y0 = std::min(m_p1.y, m_p2.y) ;
    coord y1 = std::max(m_p1.y, m_p2.y);
    coord w = x1 - x0 + 1;
    coord h = y1 - y0 + 1;

    if (w == 0 || h == 0){
      m_active = false;
      return TOOL_NONE;
    }
    const auto tri = tri_from_rect(Rect(Point(x0, y0), Size(w,h)));
    m_command.Set(add_or_draw(create_ellipse_object(tri, GetSettings()),
      info.layerType));
    info.status.SetMainText("");
    return TOOL_COMMIT;
  }

  ToolResult MouseMove(const PosInfo& info) override{
    if (!m_active){
      info.status.SetText(str(info.pos));
      return TOOL_NONE;
    }
    bool subPixel = SetAntiAlias(info);

    m_p2 = info.pos;

    const bool centerPoint = info.modifiers.Primary();
    const bool constrainToCircle = info.modifiers.Secondary();

    if (!centerPoint){
      // Non-centered ellipses should always originate from the initial
      // click position, m_p1 may have been modified by a
      // (since-released) center-point constraining.
      m_p1 = m_origP1;
    }

    if (centerPoint && constrainToCircle) {
      m_p2 = constrain_to_square(m_origP1, info.pos, subPixel);
      Angle angle = line_angle({m_origP1, m_p2});
      coord radius = distance(m_origP1, m_p2);
      coord rx = cos(angle) * radius;
      m_p1 = m_origP1 - Point::Both(rx);
      m_p2 = m_origP1 + Point::Both(rx);
      info.status.SetMainText("");
      info.status.SetText(str_center_radius(m_origP1, rx));
    }
    else if (centerPoint){
      Point delta = abs(m_origP1 - m_p2);
      m_p1 = m_origP1 - delta;
      m_p2 = m_origP1 + delta;
      info.status.SetMainText("Shift=Circle");
      info.status.SetText(str_center_radius(m_origP1,
        radii_from_point(delta)) , 0);
    }
    else if (constrainToCircle){
      info.status.SetMainText("Ctrl=From center");
      m_p2 = constrain_to_square(m_p1, info.pos, subPixel);
      info.status.SetText(str_from_to(m_p1, m_p2), 0);
    }
    else {
      info.status.SetMainText("Shift=Circle Ctrl=From center");
      info.status.SetText(str_from_to(m_p1, m_p2), 0);
    }
    return TOOL_DRAW;
  }

  ToolResult Preempt(const PosInfo&) override{
    if (then_false(m_active)){
      return TOOL_CANCEL;
    }
    return TOOL_NONE;
  }

private:
  bool m_active;
  PendingCommand m_command;
  Point m_p1;
  Point m_p2;
  Point m_origP1;
};

Tool* ellipse_tool(const Settings& s){
  return new EllipseTool(s);
}

} // namespace
