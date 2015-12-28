// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_PY_COMMON_HH
#define FAINT_PY_COMMON_HH
#include "app/get-app-context.hh" // Fixme
#include "bitmap/color-counting.hh"
#include "bitmap/filter.hh"
#include "commands/blit-bitmap-cmd.hh"
#include "commands/draw-object-cmd.hh"
#include "commands/flip-rotate-cmd.hh"
#include "commands/function-cmd.hh"
#include "commands/rescale-cmd.hh"
#include "commands/resize-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "objects/objline.hh"
#include "python/py-function-error.hh"
#include "text/formatting.hh"
#include "util/clipboard.hh"
#include "util/setting-util.hh"

namespace faint{

struct Common_aa_line{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "aa_line((x0,y0,x1,y1),(r,g,b[,a]))\nExperimental!\nDraw an anti-aliased line from x0,y0 to x1,y1 with the specified color";
  }

  static const char* Name(){
    return "aa_line";
  }

  template<typename T>
  static void Func(T target, const LineSegment& line, const ColRGB& color){
    python_run_command(target,
      target_full_image(get_aa_line_command(floored(line.p0),
        floored(line.p1),
        color)));
  }
};


struct Common_auto_crop{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "auto_crop()\nAuto-crops the image.";
  }

  static const char* Name(){
    return "auto_crop";
  }

  template<typename T>
  static void Func(T target){
    Command* cmd = get_auto_crop_command(target.GetImage());
    if (cmd != nullptr){
      python_run_command(target, cmd);
    }
  }
};

struct Common_blit{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "blit((x,y), bmp)\nBlits the Bitmap-object at x,y.";
  }

  static const char* Name(){
    return "blit";
  }

  template<typename T>
  static void Func(T target, const IntPoint& pos, const Bitmap& bmp){
    python_run_command(target, get_blit_bitmap_command(pos, bmp));
  }
};

struct Common_blur{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "blur()\nBlurs the image.";
  }

  static const char* Name(){
    return "blur";
  }

  template<typename T>
  static void Func(T target){
    python_run_command(target,
      target_full_image(get_blur_command()));
  }
};

struct Common_boundary_fill{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "boundary_fill(x,y, src, boundary_color)\nBoundary fill from x,y with src, up to the boundary_color.";
  }

  static const char* Name(){
    return "boundary_fill";
  }

  template<typename T>
  static void Func(T target, const IntPoint& pos, const Paint& fill, const Color& boundary) {
    if (!fully_positive(pos)){
      throw ValueError("Fill origin position must be positive");
    }
    if (!contains_pos(target, pos)){
      throw ValueError("Fill origin position outside image");
    }
    python_run_command(target,
      target_full_image(get_boundary_fill_command(pos, fill, boundary)));
  }
};

struct Common_clear{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "clear(src)\nClear the image with the specified color, pattern or gradient";
  }

  static const char* Name(){
    return "clear";
  }

  template<typename T>
  static void Func(T target, const Paint& src){
    python_run_command(target,
      target_full_image(get_clear_command(src)));
  }
};

struct Common_count_colors{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "color_count() -> count\nReturns the number of distinct colors in the image";
  }

  static const char* Name(){
    return "color_count";
  }

  template<typename T>
  static int Func(T target) {
    const auto& bmp = target.GetBitmap();
    if (bmp.NotSet()){
      throw ValueError("Item has no bitmap."); // Fixme
    }
    return count_colors(bmp.Get());
  }
};

void do_copy_rect(const Optional<Bitmap>&, const IntRect&);

struct Common_copy_rect{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "copy_rect(x,y,w,h)\nCopies the specified rectangle to the clipboard";
  }

  static const char* Name(){
    return "copy_rect";
  }

  template<typename T>
  static void Func(T target, const IntRect& rect){
    do_copy_rect(target.GetBitmap(), rect);
  }
};

struct Common_desaturate{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "desaturate()\nDesaturate the image.";
  }

  static const char* Name(){
    return "desaturate";
  }

  template<typename T>
  static void Func(T target){
    python_run_command(target,
      target_full_image(get_desaturate_simple_command()));
  }
};


struct Common_desaturate_weighted{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "desaturate_weighted()\nDesaturate the image with weighted intensity.";
  }

  static const char* Name(){
    return "desaturate_weighted";
  }

  template<typename T>
  static void Func(T target){
    python_run_command(target,
      target_full_image(get_desaturate_weighted_command()));
  }
};

struct Common_flood_fill{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "fill((x,y),src)\nFlood fill at x,y with src";
  }

  static const char* Name(){
    return "fill";
  }

  template<typename T>
  static void Func(T target, const IntPoint& pos, const Paint& fill) {
    if (!fully_positive(pos)){
      throw ValueError("Fill origin position must be positive");
    }
    if (!contains_pos(target, pos)){
      throw ValueError("Fill origin position outside image");
    }
    python_run_command(target,
      target_full_image(get_flood_fill_command(pos, fill)));
  }
};

struct Common_gaussian_blur{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "gaussian_blur()\nBlurs the image with a gaussian kernel.";
  }

  static const char* Name(){
    return "gaussian_blur";
  }

  template<typename T>
  static void Func(T target, const coord& sigma){
    if (sigma <= 0){
      throw ValueError("Sigma must be > 0");
    }

    python_run_command(target,
      target_full_image(get_function_command("Gaussian blur",
          [=](Bitmap& bmp){gaussian_blur(bmp, sigma);})));
  }
};

struct Common_invert{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "invert()\nInvert the colors of the image";
  }

  static const char* Name(){
    return "invert";
  }

  template<typename T>
  static void Func(T target){
    python_run_command(target,
      target_full_image(get_invert_command()));
  }
};

struct Common_line{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "line(x0,y0,x1,y1)\nDraw a line from x0,y0 to x1,y1";
  }

  static const char* Name(){
    return "line";
  }

  template<typename T>
  static void Func(T target, const LineSegment& line){
    Settings s(default_line_settings());
    s.Update(get_app_context().GetToolSettings());
    s.Set(ts_AntiAlias, false);
    Apply(target, its_yours(create_line_object(Points(std::vector<Point>({line.p0, line.p1})), s)));
  }

  template<typename T>
  static void Apply(T& target, its_yours_t<Object> line){
    // Separate function to allow specialization.
    python_run_command(target, draw_object_command(line));
  }
};

struct Common_replace_color{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "replace_color(old, new)\nReplaces all pixels matching the color old with the color new.\nThe colors are specified as r,g,b[,a]-tuples";
  }

  static const char* Name(){
    return "replace_color";
  }

  template<typename T>
  static void Func(T target, const Color& oldColor, const Paint& replacement){
    python_run_command(target,
      target_full_image(get_replace_color_command(Old(oldColor),
        replacement)));
  }
};

struct Common_rotate{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "rotate(a)\nRotate the image a-radians";
  }

  static const char* Name(){
    return "rotate";
  }

  template<typename T>
  static void Func(T target, const radian& angle, const Optional<Paint>& bg){
    python_run_command(target,
      rotate_image_command(Angle::Rad(angle),
        bg.Or(get_app_context().GetToolSettings().Get(ts_Bg))));
  }
};

struct Common_scale_bilinear{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "scale_bilinear(w,h)\nScale the image to the specified size with bilinear interpolation";
  }

  static const char* Name(){
    return "scale_bilinear";
  }

  template<typename T>
  static void Func(T target, const IntSize& size){
    python_run_command(target, rescale_command(size, ScaleQuality::BILINEAR));
  }
};


struct Common_scale_nearest{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "scale_nearest(w,h)\nScale the image to the specified size with nearest neighbour interpolation";
  }

  static const char* Name(){
    return "scale_nearest";
  }

  template<typename T>
  static void Func(T target, const IntSize& size){
    python_run_command(target, rescale_command(size, ScaleQuality::NEAREST));
  }
};

struct Common_sepia{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "sepia(intensity)\nApplies a horrendous sepia filter on the image.";
  }

  static const char* Name(){
    return "sepia";
  }

  template<typename T>
  static void Func(T target, const int& intensity){
    python_run_command(target,
      target_full_image(get_sepia_command(intensity)));
  }
};

struct Common_threshold{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    static const std::string s("set_threshold(low,high[,c1,c2])\nAssigns pixels with summed RGB components between low and high to c1, pixels outside "
      "to c2 (or the active foreground and background if omitted).\nThe range for min and max is " + str_range(as_closed_range<threshold_range_t>()).str() + ".");
    return s.c_str();
  }

  static const char* Name(){
    return "set_threshold";
  }

  template<typename T>
  static void Func(T target, const threshold_range_t& range, const Optional<Paint>& srcIn, const Optional<Paint>& srcOut){
    python_run_command(target,
      target_full_image(get_threshold_command(range,
        srcIn.Or(get_app_context().GetToolSettings().Get(ts_Fg)),
        srcOut.Or(get_app_context().GetToolSettings().Get(ts_Bg)))));
  }
};

struct Common_paste{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "paste(x,y)\nPastes a bitmap from the clipboard to (x,y)";
  }

  static const char* Name(){
    return "paste";
  }

  template<typename T>
  static void Func(T target, const IntPoint& pos){
    Clipboard clipboard;
    if (!clipboard.Good()){
      throw ValueError("Failed opening clipboard");
    }

    auto bmp(std::move(clipboard.GetBitmap()));
    if (bmp){
      python_run_command(target,
        get_paste_raster_bitmap_command(bmp.Take(), pos,
          target.GetRasterSelection(),
          get_app_context().GetToolSettings()));
    }
    else{
      throw ValueError("No bitmap in clipboard");
    }
  }
};

struct Common_quantize{
  static decltype(METH_VARARGS) ArgType(){
    return METH_NOARGS;
  }

  static const char* Doc(){
    return "quantize()\nReduce the colors in the image to at most 256.";
  }

  static const char* Name(){
    return "quantize";
  }

  template<typename T>
  static void Func(T target){
    python_run_command(target,
      target_full_image(get_quantize_command()));
  }
};


struct Common_insert_bitmap{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "insert_bitmap(x,y, bmp)\nInserts bmp floating at x,y";
  }

  static const char* Name(){
    return "insert_bitmap";
  }

  template<typename T>
  static void Func(T target, const IntPoint& pos, const Bitmap& bmp){
    // Fixme: Doesn't properly stamp old floating?
    // Fixme: Name is Paste rather than insert
    python_run_command(target,
      get_paste_raster_bitmap_command(bmp, pos,
        target.GetRasterSelection(),
        get_app_context().GetToolSettings()));
  }
};

struct Common_pixelize{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "pixelize(width)\nPixelize the image with the given width";
  }

  static const char* Name(){
    return "pixelize";
  }

  template<typename T>
  static void Func(T target, const int& width){
    if (width < 1){
      throw ValueError("Width must be >= 1");
    }
    python_run_command(target,
      target_full_image(get_pixelize_command(width)));
  }
};

struct Common_set_rect{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "set_rect(x,y,w,h)\nSets the image size to w,h extending from x,y.\nx and y may be negative";
  }

  static const char* Name(){
    return "set_rect";
  }

  template<typename T>
  static void Func(T target, const IntRect& rect, const Optional<Paint>& src){
    if (empty(rect)){
      throw ValueError("Empty rectangle specified.");
    }
    Paint bg(get_app_context().GetToolSettings().Get(ts_Bg));
    python_run_command(target,
      resize_command(rect, src.Or(get_app_context().GetToolSettings().Get(ts_Bg))));
  }
};

struct Common_erase_but_color{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "erase_but_color(keptColor[,eraseColor])\nReplaces all colors, except keptColor, with eraseColor. Uses the current secondary color if keptColor is omitted.";
  }

  static const char* Name(){
    return "erase_but_color";
  }

  template<typename T>
  static void Func(T target, const Color& keep, const Optional<Paint>& eraser){
    if (!eraser.IsSet()){
      Paint bg = get_app_context().GetToolSettings().Get(ts_Bg);
      if (bg == keep){
        // Return without error when retrieved bg is same
        return;
      }
      python_run_command(target,
        target_full_image(get_erase_but_color_command(keep, bg)));
    }
    else{
      if (keep == eraser.Get()){
        // Consider explicitly replacing everyting but color with color
        // an error
          throw ValueError("Same erase color as the kept color");
      }
      python_run_command(target,
        target_full_image(get_erase_but_color_command(keep, eraser.Get())));
    }
  }
};

struct Common_replace_alpha{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "replace_alpha(r,g,b)\nBlends alpha towards the specified color";
  }

  static const char* Name(){
    return "replace_alpha";
  }

  template<typename T>
  static void Func(T target, const ColRGB& color){
    python_run_command(target,
      target_full_image(get_blend_alpha_command(color)));
  }
};

struct Common_set_alpha{
  typedef StaticBoundedInt<0,255> color_value_t;
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "set_alpha(a)\nSets the alpha component of all pixels to a";
  }

  static const char* Name(){
    return "set_alpha";
  }

  template<typename T>
  static void Func(T target, const color_value_t& alpha){
    python_run_command(target,
      target_full_image(get_set_alpha_command(static_cast<uchar>(alpha.GetValue()))));
  }
};

struct Common_color_balance{
  static decltype(METH_VARARGS) ArgType(){
    return METH_VARARGS;
  }

  static const char* Doc(){
    return "color_balance((r0,r1),(g0,g1),(b0,b1))\nStretches the specified color intervals to [0,255]";
  }

  static const char* Name(){
    return "color_balance";
  }

  template<typename T>
  static void Func(T target, const color_range_t& r, const color_range_t& g, const color_range_t& b){
    python_run_command(target,
      target_full_image(get_function_command("Color balance",
          [=](Bitmap& bmp){color_balance(bmp, r, g, b);})));
  }
};

}

#endif
