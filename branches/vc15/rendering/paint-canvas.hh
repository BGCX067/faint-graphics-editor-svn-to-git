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
#include "util/common-fwd.hh"

class wxDC;
class ToolWrapper;

namespace faint{

// True if the tool targets the raster layer. If so, any raster
// selection outline should be drawn, and raster-selection
// related menu options should be enabled
bool should_draw_raster(const ToolWrapper&, Layer);

// True if the tool targets the vector/object layer. If so, selected
// object handles will be drawn, and object-selection related menu
// options should be enabled.
bool should_draw_vector(const ToolWrapper&, Layer);

// Draws the canvas onto the passed in DC
void paint_canvas(wxDC&,
  const Image&,
  const CanvasState&,
  const IntRect& updateRegion,
  const std::weak_ptr<Bitmap>& bitmapChimera,
  const ColRGB& bgColor,
  const PosInfo&,
  const RasterSelection&,
  Tool&,
  const TransparencyStyle&,
  Layer,
  int objectHandleWidth);

} // namespace

#endif
