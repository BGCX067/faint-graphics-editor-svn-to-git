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

#include <algorithm>
#include "wx/bitmap.h"
#include "wx/dc.h"
#include "wx/dcmemory.h"
#include "wx/dcclient.h"
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/canvas-geo.hh"
#include "geo/geo-func.hh"
#include "geo/intpoint.hh"
#include "geo/intrect.hh"
#include "geo/point.hh"
#include "geo/primitive.hh"
#include "geo/rect.hh"
#include "geo/scale.hh"
#include "geo/size.hh"
#include "geo/tri.hh"
#include "objects/object.hh"
#include "rendering/faint-dc.hh"
#include "rendering/overlay.hh"
#include "rendering/overlay-dc-wx.hh"
#include "rendering/paint-canvas.hh"
#include "tools/tool.hh"
#include "tools/tool-wrapper.hh"
#include "util/canvas-handle.hh"
#include "util/convert-wx.hh"
#include "util/grid.hh"
#include "util/image.hh"
#include "util/iter.hh"
#include "util/mouse.hh"
#include "util/object-util.hh"

namespace faint{

bool should_draw_raster(const Tool& tool, Layer layer){
  return get_tool_layer(tool.GetId(), layer) == Layer::RASTER;
}

bool should_draw_raster(const ToolWrapper& toolWrap, Layer layer){
  ToolId tool = toolWrap.GetToolId();
  return get_tool_layer(tool, layer) == Layer::RASTER;
}

bool should_draw_vector(const Tool& tool, Layer layer){
  return !should_draw_raster(tool, layer);
}

bool should_draw_vector(const ToolWrapper& toolWrap, Layer layer){
  return !should_draw_raster(toolWrap, layer);
}

Layer get_tool_layer(ToolId tool, Layer defaultLayer){
  if (raster_layer_only(tool)){
    return Layer::RASTER;
  }
  else if (object_layer_only(tool)){
    return Layer::OBJECT;
  }
  return defaultLayer;
}

static wxBitmap& get_back_buffer(const IntSize& minSize){
  static wxBitmap backBuffer(minSize.w,minSize.h);
  wxSize oldSize = backBuffer.GetSize();
  if (oldSize.GetWidth() < minSize.w || oldSize.GetHeight() < minSize.h){
    backBuffer = wxBitmap(std::max(minSize.w, oldSize.GetWidth()),
      std::max(minSize.h, oldSize.GetHeight()));
  }
  return backBuffer;
}

static void paint_canvas_handles(wxDC& dc, const IntSize& canvasSize, const CanvasGeo& geo){
  dc.SetPen(wxPen(wxColour(128, 128, 128), 1));
  dc.SetBrush(wxBrush(wxColour(181, 165, 213)));

  for (HandlePos handlePos : iterable<HandlePos>()){
    CanvasResizeHandle handle(handlePos, canvasSize, geo);
    dc.DrawRectangle(to_wx(handle.GetRect()));
  }
}

static Rect view_to_image(const IntRect& r, const CanvasGeo& geo){
  coord zoom = geo.zoom.GetScaleFactor();
  Point pos = (r.TopLeft() + geo.pos - point_from_size(geo.border)) / zoom;
  Size size = (r.GetSize() + geo.border) / zoom;
  return Rect(pos, size);
}

static void paint_without_image(wxDC& dc, const IntRect& rView, const CanvasGeo& geo, const IntSize& imageSize, const ColRGB& canvasBg){
  dc.SetBrush(wxBrush(to_wx(canvasBg)));
  dc.SetPen(wxPen(to_wx(canvasBg)));
  dc.DrawRectangle(rView.x, rView.y, rView.w, rView.h);
  paint_canvas_handles(dc, imageSize, geo);
}

static void paint_checkered_background(wxDC& dc, const IntPoint& topLeft, const IntPoint& imageRegionTopLeft, const CanvasGeo& geo, int w, int h ){
  const int tileSz = 20;
  const wxColor c1(192,192,192);
  const wxColor c2(255,255,255);
  const coord zoom = geo.zoom.GetScaleFactor();
  int xOff = truncated(imageRegionTopLeft.x * zoom) % (tileSz * 2);
  int yOff = truncated(imageRegionTopLeft.y * zoom) % (tileSz * 2);
  const int row_max = h / tileSz + 2;
  const int col_max = w / (tileSz * 2) + 2;
  for (int row = 0; row <= row_max ; row++){
    dc.SetBrush(wxBrush(c1));
    dc.SetPen(wxPen(c1));
    int shift = (row % 2) * tileSz;
    for (int col = 0; col <= col_max; col++){
      dc.DrawRectangle(0 - xOff + col * 40 + shift + topLeft.x,
        0 - yOff + row * 20 + topLeft.y,
        tileSz,
        tileSz);
    }
    dc.SetBrush(wxBrush(c2));
    dc.SetPen(wxPen(c2));
    for (int col = 0; col <= col_max; col++){
      dc.DrawRectangle(0 - xOff + col * 40 - 20 + shift + topLeft.x, 0 - yOff + row * 20 + topLeft.y, tileSz, tileSz);
    }
  }
}

static void paint_fill_background(wxDC& dc, const IntPoint& topLeft, const IntSize& sz, const wxColour& bg){
  dc.SetBrush(wxBrush(bg));
  dc.SetPen(wxPen(bg));
  dc.DrawRectangle(topLeft.x, topLeft.y, sz.w, sz.h);
}

// Returns the intersection of the update region (in view coordinates)
// and the image as a rectangle in image coordinates.
static IntRect get_image_region(const IntRect& view, const IntSize& bmpSize, const CanvasGeo& g){
  const coord zoom = g.zoom.GetScaleFactor();
  Rect bmpRect(floated(rect_from_size(bmpSize)));

  Point pos(floated(view.TopLeft() + g.pos -point_from_size(g.border)) / zoom);
  Size size((view.GetSize() / zoom) + Size::Both(2));
  Rect rImage = intersection(Rect(pos, size), bmpRect);

  IntPoint dbgPos(max_coords(rounded_down(rImage.TopLeft()), IntPoint(0,0)));
  IntSize dbgSize = min_coords(truncated(rImage.GetSize() + Size::Both(5)),
    bmpSize - size_from_point(dbgPos));
  return IntRect(dbgPos, dbgSize);
}

static void paint_object_handles(const objects_t& objects, OverlayDC& dc){
  for (const Object* obj : objects){
    // Draw handles for objects, excluding currently adjusted objects
    // so the handles don't get in the way
    if (obj->Inactive() && resize_handles_enabled(obj)){
      if (object_aligned_resize(obj)){
        dc.Handles(obj->GetTri());
      }
      else{
        dc.Handles(bounding_rect(obj->GetTri()));
      }
    }
  }
}

static void paint_objects(FaintDC&& dc, const objects_t& objects, Tool& tool, Overlays& overlays, Point mouseImagePos, Layer layer){
  for (auto object : objects){
    object->Draw(dc);
  }

  if (!tool.DrawBeforeZoom(layer)){
    tool.Draw(dc, overlays, mouseImagePos);
  }
}

struct PaintInfo{
  IntSize bmpSize;
  Bitmap subBitmap;
  IntRect imageRegion;
  Rect imageCoordRect;
};

static void from_bitmap(PaintInfo& info, const Bitmap& bmp, const IntRect& viewRect, const CanvasGeo& geo){
  info.bmpSize = bmp.GetSize();
  info.imageRegion = get_image_region(viewRect, info.bmpSize, geo);
  info.imageCoordRect = view_to_image(viewRect, geo);
  if (empty(info.imageRegion)){
    return;
  }
  info.subBitmap = subbitmap(bmp, info.imageRegion);
}

static void from_color(PaintInfo& info, const Color& color, const IntSize& size, const IntRect rView, const CanvasGeo& geo){
  info.imageRegion = get_image_region(rView, size, geo);
  info.imageCoordRect = view_to_image(rView, geo);
  info.subBitmap = Bitmap(info.imageRegion.GetSize(), color);
  // Fixme: info.bmpSize?
}

static void set_origin(wxDC& dc, const IntPoint& p){
  dc.SetDeviceOrigin(p.x, p.y);
}

static void draw_bmp(wxDC& dc, const Bitmap& bmp, const IntPoint& p){
  dc.DrawBitmap(to_wx_bmp(bmp), p.x, p.y);
}

static void blit(wxDC& dst, wxDC& src, const IntRect& r){
  dst.Blit(r.x, r.y, r.w, r.h, &src, 0, 0);
}

void paint_overlays(wxDC& dc, coord zoom, const CanvasState& state, const PaintInfo& info, const IntRect& updateRegion, const Image& active, const Bitmap& scaled, bool shouldDrawVector, Overlays& overlays){
  const auto& geo = state.geo;
  // Offset the DC for the backbuffer so that the top left corner of
  // the image is the origin (0,0)
  set_origin(dc, point_from_size(geo.border) - geo.pos -
    updateRegion.TopLeft());

  OverlayDC_WX overlayDC(dc, zoom, info.imageCoordRect, info.bmpSize);
  if (state.grid.Visible()){
    overlayDC.GridLines(state.grid, info.imageRegion.TopLeft(), scaled.GetSize());
  }

  if (shouldDrawVector){
    paint_object_handles(active.GetObjectSelection(), overlayDC);
  }

  if (state.pointOverlay.IsSet()){
    overlays.Pixel(state.pointOverlay.Get());
  }
  overlays.Paint(overlayDC);
  set_origin(dc, IntPoint(0,0));
}

 // Fixme: topLeft + bmpSize = scaledRect
void paint_background(wxMemoryDC& dc, const TransparencyStyle& trStyle, const IntRect& updateRegion, const IntPoint& topLeft, const IntSize& bmpSize, const PaintInfo& info, const CanvasGeo& geo){
  dc.SetClippingRegion(to_wx(IntRect(topLeft - updateRegion.TopLeft(), bmpSize)));
  if (trStyle.IsCheckered()){
    paint_checkered_background(dc, topLeft - updateRegion.TopLeft(),
      info.imageRegion.TopLeft(), geo, bmpSize.w, bmpSize.h); // Fixme: pass IntSize
  }
  else{
    paint_fill_background(dc, topLeft - updateRegion.TopLeft(),
      bmpSize, to_wx(trStyle.GetColor())); // Fixme: Pass ColRGB
  }
  dc.DestroyClippingRegion();
}

void paint_canvas(wxDC& paintDC, const Image& active, const CanvasState& state, const IntRect& updateRegion, const std::weak_ptr<Copyable<Bitmap>>& weakBitmapChimera, const ColRGB& canvasBg, const Point& mouseImagePos, const RasterSelection& rasterSelection, Tool& tool, TransparencyStyle trStyle, Layer layer){

  PaintInfo info;
  if (auto bitmapChimera = weakBitmapChimera.lock()){
    // Use the bitmap chimera as the raster background (this is for
    // feedback from some operation in a dialog, e.g.
    // brightness/contrast).
    from_bitmap(info, *bitmapChimera, updateRegion, state.geo);
  }
  else{
    active.GetBg().Visit(
      [&](const Bitmap& bg){
        // Use the image background bitmap.
        from_bitmap(info, bg, updateRegion, state.geo);
      },
      [&](const ColorSpan& bg){
        // No raster background - create on the fly.
        from_color(info, bg.color, bg.size, updateRegion, state.geo);
      });
  }

  if (empty(info.imageRegion)){
    // No part of of the image is visible in the update region.
    return paint_without_image(paintDC, updateRegion, state.geo,
      active.GetSize(), canvasBg);
  }

  // Collects overlay graphics (which are drawn rather later).
  Overlays overlays;

  // Draw raster tool and floating selection to the 1:1 bitmap
  bool toolBeforeZoom = tool.DrawBeforeZoom(layer);
  if (toolBeforeZoom || rasterSelection.Floating()){
    FaintDC dc (info.subBitmap,
      origin_t(-floated(info.imageRegion.TopLeft())));
    if (toolBeforeZoom){
      tool.Draw(dc, overlays, mouseImagePos);
    }
    if (rasterSelection.Floating()){
      rasterSelection.DrawFloating(dc);
    }
  }

  // Create a scaled bitmap for object graphics and overlays
  const coord zoom = state.geo.zoom.GetScaleFactor();
  Bitmap scaled(state.geo.zoom.At100() ? copy(info.subBitmap) :
    (zoom > 1.0 ?
      copy(scale_nearest(info.subBitmap, rounded(zoom))):
      copy(scale_bilinear(info.subBitmap, Scale(zoom)))));

  if (!bitmap_ok(scaled)){
    return paint_without_image(paintDC, updateRegion, state.geo,
      active.GetSize(), canvasBg);
  }

  // Paint objects onto the scaled bitmap
  paint_objects(
    FaintDC(scaled, origin_t(-info.imageRegion.TopLeft() * zoom), zoom),
    active.GetObjects(), tool, overlays, mouseImagePos, layer);

  if (should_draw_raster(tool, layer)){
    rasterSelection.DrawOutline(overlays);
  }

  {
    // Draw the image and any overlays to the back-buffer to avoid
    // flickering
    wxBitmap& backBuffer = get_back_buffer(updateRegion.GetSize());
    wxMemoryDC memDC(backBuffer);
    memDC.SetBackground(wxBrush(to_wx(canvasBg)));
    memDC.Clear();

    // Top left of image in view-coordinates
    IntPoint topLeft(mouse::image_to_view(info.imageRegion.TopLeft(), state.geo));

    paint_background(memDC, trStyle, updateRegion, topLeft, scaled.GetSize(),
      info, state.geo);

    // Draw the scaled part of the bitmap to the back buffer
    draw_bmp(memDC, scaled, topLeft - updateRegion.TopLeft());
    set_origin(memDC, -updateRegion.TopLeft());
    paint_canvas_handles(memDC, active.GetSize(), state.geo);
    paint_overlays(memDC, zoom, state, info, updateRegion, active, scaled, should_draw_vector(tool, layer), overlays);

    // Blit it all to the window
    blit(paintDC, memDC, updateRegion);
  }
}

} // namespace
