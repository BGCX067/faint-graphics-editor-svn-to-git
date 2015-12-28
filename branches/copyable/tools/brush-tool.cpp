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

#include <vector>
#include "bitmap/alpha-map.hh"
#include "bitmap/brush.hh"
#include "geo/adjust.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "geo/size.hh"
#include "rendering/faint-dc.hh"
#include "rendering/overlay.hh"
#include "text/formatting.hh"
#include "tools/standard-tool.hh"
#include "util/canvas.hh"
#include "util/color.hh"
#include "util/setting-util.hh"
#include "util/tool-util.hh"

namespace faint{

static bool eraser(const PosInfo& info){
  return info.tabletCursor == TABLET_CURSOR_ERASER;
}

static bool translucent(const Brush& b){
  const IntSize sz(b.GetSize());
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      uchar a = b.Get(x,y);
      if (a != 0 && a != 255){
        return true;
      }
    }
  }
  return false;
}

static std::vector<LineSegment> brush_edge(const Brush& b){
  IntSize sz(b.GetSize());
  std::vector<LineSegment> lines;
  for (int y = -1; y <= sz.h; y++){
    bool xTop = false;
    int xTopStart = 0;
    bool xBottom = false;
    int xBottomStart = 0;

    for (int x = 0; x <= sz.w; x++){
      // Pixel top side
      bool scanOut = (x != sz.w && y < sz.h - 1) &&
        (y == -1 || b.Get(x,y) == 0);
      if (scanOut){
        if (b.Get(x,y + 1) != 0){
          if (!xTop){
            xTopStart = x;
            xTop = true;
          }
        }
        else if (xTop){
          xTop = false;
          lines.push_back(floated(IntLineSegment({xTopStart,y + 1}, {x, y + 1})));
        }
      }
      else {
        if (xTop){
          xTop = false;
          lines.push_back(floated(IntLineSegment({xTopStart,y + 1}, {x, y + 1})));
        }
      }

      // Pixel bottom side
      scanOut = (x != sz.w && y > 0) &&
        (y == sz.h || b.Get(x,y) == 0);
      if (scanOut){
        if (b.Get(x,y - 1) != 0){
          if (!xBottom){
            xBottomStart = x;
            xBottom = true;
          }
        }
        else if (xBottom){
          xBottom = false;
          lines.push_back(floated(IntLineSegment({xBottomStart,y}, {x, y})));
        }

      }
      else {
        if (xBottom){
          xBottom = false;
          lines.push_back(floated(IntLineSegment({xBottomStart,y}, {x, y})));
        }
      }
    }
  }

  // Left/Right
  for (int x = -1; x <= sz.w; x++){
    bool yLeft = false;
    int yLeftStart = 0;

    bool yRight = false;
    int yRightStart = 0;

    for (int y = 0; y <= sz.h; y++){
      // Pixel left side
      bool scanOut = (x < sz.w - 1 && y != sz.h) &&
        (x == -1 || b.Get(x,y) == 0);
      if (scanOut){
        if (b.Get(x+1,y) != 0){
          if (!yLeft){
            yLeftStart = y;
            yLeft = true;
          }
        }
        else if (yLeft){
          yLeft = false;
          lines.push_back(floated(IntLineSegment({x + 1,yLeftStart}, {x + 1, y})));
        }
      }
      else {
        if (yLeft){
          yLeft = false;
          lines.push_back(floated(IntLineSegment({x + 1,yLeftStart}, {x + 1, y})));
        }
      }

      // Pixel right side
      scanOut = (x > 0 && y != sz.h) &&
        (x == sz.w || b.Get(x,y) == 0);
      if (scanOut){
        if (b.Get(x - 1,y) != 0){
          if (!yRight){
            yRightStart = y;
            yRight = true;
          }
        }
        else if (yRight){
          yRight = false;
          lines.push_back(floated(IntLineSegment({x,yRightStart}, {x, y})));
        }

      }
      else {
        if (yRight){
          yRight = false;
          lines.push_back(floated(IntLineSegment({x,yRightStart}, {x, y})));
        }
      }
    }
  }
  return lines;
}

class BrushCommand : public Command{
public:
  BrushCommand(const IntPoint& topLeft, const AlphaMap& alphaMap, const IntPoint& first, const Settings& settings)
    : Command(CommandType::RASTER),
      m_settings(settings),
      m_alphaMap(copy(alphaMap)),
      m_first(first),
      m_topLeft(topLeft)
  {
    finalize_swap_colors_erase_bg(m_settings);
  }

  utf8_string Name() const{
    return "Brush Stroke";
  }

  void Do(CommandContext& context){
    context.GetDC().Blend(m_alphaMap, m_topLeft, m_first, m_settings);
  }

private:
  Settings m_settings;
  AlphaMap m_alphaMap;
  IntPoint m_first;
  IntPoint m_topLeft;
};

static void init_brush_overlay(AlphaMap& m, const Brush& b){
  IntSize brushSz = b.GetSize();
  m.Reset(brushSz);
  for (int y = 0; y != brushSz.h; y++){
    for (int x = 0; x != brushSz.w; x++){
      m.Add(x, y, b.Get(x,y));
    }
  }
}

static Settings brush_settings(const Settings& allSettings){
  Settings s;
  s.Set(ts_BrushSize, 1);
  s.Set(ts_BrushShape, BrushShape::SQUARE);
  s.Set(ts_Fg, Paint(Color(0,0,0)));
  s.Set(ts_Bg, Paint(Color(0,0,0)));
  s.Set(ts_SwapColors, false);
  s.Set(ts_Filter, 0);
  s.Update(allSettings);
  return s;
}

class BrushTool : public StandardTool {
public:
  BrushTool(const Settings& allSettings)
    : StandardTool(ToolId::BRUSH, brush_settings(allSettings)),
      m_alphaMap(IntSize(1, 1)),
      m_brush(circle_brush(1)),
      m_brushOverlay(IntSize(1,1)),
      m_constrainDir(ConstrainDir::NONE),
      m_translucent(false)
  {
    m_active = false;
    // Prevent cursor being drawn at 0,0 before motion.
    m_drawCursor = false;
  }

  void Draw(FaintDC& dc, Overlays& overlays, const Point& p) override{
    if (m_active){
      dc.Blend(m_alphaMap, IntPoint(0,0), m_first, GetSettings());
      if (m_translucent){
        overlays.Lines(m_brushEdge, floored(p) - point_from_size(m_brush.GetSize() / 2));
      }
    }
    else if (m_drawCursor){
      dc.Blend(m_brushOverlay, floored(p - point_from_size(floated(m_brush.GetSize())) / 2 + Point(0.5, 0.5)), floored(p), GetSettings());
      if (m_translucent){
        overlays.Lines(m_brushEdge, floored(p) - point_from_size(m_brush.GetSize() / 2));
      }
    }
  }

  bool DrawBeforeZoom(Layer) const override{
    return true;
  }

  Command* GetCommand() override{
    return m_command.Retrieve();
  }

  Cursor GetCursor(const PosInfo& info) const override{
    if (outside_canvas_by(info, GetSettings().Get(ts_BrushSize) / 2)){
      return Cursor::BRUSH_OUT;
    }
    return Cursor::BRUSH;
  }

  IntRect GetRefreshRect(const IntRect& rView, const Point& mousePos) const override{
    if (!m_active){
      const IntPoint p(floored(mousePos));
      return padded(IntRect(p, p), get_padding(GetSettings()));
    }
    return rView;
  }

  ToolResult MouseDown(const PosInfo& info) override{
    Reset(info);
    m_prev = floored(info.pos);
    m_covered = floored(Rect(info.pos, info.pos));
    m_origin = floored(info.pos);
    if (info.modifiers.RightMouse() || eraser(info)){
      // Test: Color swap on digitizer eraser end
      SetSwapColors(true);
    }
    else{
      SetSwapColors(false);
    }
    stroke(m_alphaMap, m_prev, m_prev, m_brush);

    return TOOL_DRAW;
  }

  ToolResult MouseUp(const PosInfo&) override{
    if (!m_active){
      return TOOL_NONE;
    }
    return Commit();
  }

  ToolResult MouseMove(const PosInfo& info) override{
    info.status.SetMainText("");
    info.status.SetText(str(info.pos), 0);

    Point pos(info.pos);
    if (!m_drawCursor){
      InitBrush();
      m_drawCursor = true;
    }

    if (m_active) {
      const bool constrainHeld(info.modifiers.Secondary());
      if (constrainHeld) {
        if (m_constrainDir == ConstrainDir::NONE){
          if (distance(floored(pos), m_origin) > 5){
            // Lock the constrain direction after a certain distance
            m_constrainDir = constrain_pos(pos, floated(m_origin));
          }
          else{
            // Don't draw anything until the constrain direction is
            // ascertained, to avoid "bumps".
            return TOOL_NONE;
          }
        }
        else{
          constrain_pos(pos, floated(m_origin), m_constrainDir);
        }
      }
      else {
        m_origin = floored(pos);
        m_constrainDir = ConstrainDir::NONE;
      }

      stroke(m_alphaMap, m_prev, floored(pos), m_brush);
      m_covered = bounding_rect(m_covered.TopLeft(), m_covered.BottomRight(), floored(pos));
      m_prev = floored(pos);
      return TOOL_DRAW;
    }
    else{
      // Test: Swap brush cursor color on digitizer eraser end
      SetSwapColors(info.tabletCursor == TABLET_CURSOR_ERASER);
    }
    return TOOL_DRAW;
  }

  ToolResult Preempt(const PosInfo&) override{
    if (!m_active){
      return TOOL_NONE;
    }
    return TOOL_NONE;
  }

  bool RefreshOnMouseOut() override{
    // Prevent leaving a brush cursor dropping when mouse leaves the
    // window
    m_drawCursor = false;
    return true;
  }

private:
  void InitBrush(){
    m_brush = get_brush(GetSettings());
    m_brushEdge = brush_edge(m_brush);
    init_brush_overlay(m_brushOverlay, m_brush);
    m_translucent = translucent(m_brush);
  }

  ToolResult Commit(){
    assert(m_active);
    m_active = false;

    IntRect r(intersection(padded(m_covered, get_padding(GetSettings())),
      IntRect(IntPoint(0,0), m_alphaMap.GetSize())));
    if (empty(r)){
      return TOOL_NONE;
    }
    m_command.Set(new BrushCommand(r.TopLeft(),
      m_alphaMap.SubCopy(r),
        m_first, GetSettings()));
    return TOOL_COMMIT;
  }

  void Reset(const PosInfo& info){
    m_active = true;
    m_alphaMap.Reset(info.canvas.GetSize());
    m_constrainDir = ConstrainDir::NONE;
    InitBrush();
    m_first = floored(info.pos);
  }
  bool m_active;
  AlphaMap m_alphaMap;
  Brush m_brush;
  std::vector<LineSegment> m_brushEdge;
  AlphaMap m_brushOverlay;
  PendingCommand m_command;
  ConstrainDir m_constrainDir;
  bool m_drawCursor;
  IntPoint m_first;
  IntPoint m_origin;
  IntPoint m_prev;
  IntRect m_covered;
  bool m_translucent;
};

Tool* brush_tool(const Settings& s){
  return new BrushTool(s);
}

} // namespace
