// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_PAINT_CANVAS_HH
#define FAINT_PAINT_CANVAS_HH
#include <memory>
#include "gui/canvas-state.hh"
#include "gui/transparency-style.hh"
#include "tools/tool-id.hh"
#include "util/common-fwd.hh"
#include "util/setting-id.hh"

class wxDC;
class ToolWrapper;

namespace faint{
  // Fixme: Document these
  bool should_draw_raster(const ToolWrapper&, Layer);
  bool should_draw_vector(const ToolWrapper&, Layer);

  // Get the layer used by the specified tool,
  // or the given defaultLayer if the tool supports both raster and vector.
  Layer get_tool_layer( ToolId, Layer defaultLayer );

  // Fixme: See if something can be done about this terrible parameter list
  void paint_canvas(wxDC&,
    const Image&,
    const CanvasState&,
    const IntRect& updateRegion,
    const std::weak_ptr<Copyable<Bitmap>>& bitmapChimera,
    const ColRGB& bgColor,
    const Point& mouseImagePos,
    const RasterSelection&,
    Tool&,
    TransparencyStyle,
    Layer);
}

#endif
