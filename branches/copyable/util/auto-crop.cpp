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

#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/intrect.hh"
#include "util/auto-crop.hh"
#include "util/color.hh"
#include "util/optional.hh"

namespace faint{

static IntRect do_get_auto_crop_rect(const Bitmap& bmp, const Color& bgCol){
  const int width = bmp.m_w;
  const int height = bmp.m_h;
  int y0 = 0;
  int y1 = 0;
  bool upDone = false;
  bool dnDone = false;

  for (int y = 0; y != height; y++){
    for (int x = 0; x!= width; x++){
      if (!upDone){
        if (get_color_raw(bmp, x, y) != bgCol){
          y0 = y;
          upDone = true;
          if (dnDone){
            goto yDone;
          }
        }
      }
      if (!dnDone){
        if (get_color_raw(bmp, x, height - y - 1) != bgCol){
          y1 = height - y;
          dnDone = true;
          if (upDone){
            goto yDone;
          }
        }
      }
    }
  }

  // Post-y computation-label
 yDone:
  int x0 = 0;
  int x1 = 0;
  bool leftDone = false;
  bool rightDone = false;

  for (int x = 0; x!= width; x++){
    for (int y = 0; y!= height; y++){
      if (!leftDone){
        if (get_color_raw(bmp, x, y) != bgCol){
          x0 = x;
          leftDone = true;
          if (rightDone){
            goto xDone;
          }
        }
      }
      if (!rightDone){
        if (get_color_raw(bmp, width - x - 1, y) != bgCol){
          x1 = width - x;
          rightDone = true;
          if (leftDone){
            goto xDone;
          }
        }
      }
    }
  }

  // Done
 xDone:
  return IntRect(IntPoint(x0, y0), IntSize(x1 - x0, y1 - y0));
}

static bool decide_color(const Bitmap& bmp, Color& result){
  const Color leftColor = get_color_raw(bmp, 0, 0);
  const Color upColor = leftColor;
  const int width = bmp.m_w;
  const int height = bmp.m_h;

  for (int x = 0; x < width; x++) {
    if (get_color_raw(bmp, x, 0) != upColor){
      goto not_top_row;
    }
  }
  result = upColor;
  return true;
 not_top_row:
  for (int y = 0; y != height; y++){
    if (get_color_raw(bmp, 0, y) != leftColor){
      goto not_left;
    }
  }
  result = leftColor;
  return true;
 not_left:

  const Color rightColor = get_color_raw(bmp, bmp.m_w - 1, bmp.m_h - 1);
  const Color bottomColor = rightColor;

  // and then to top right
  for (int y = 0; y < height; y++){
    if (get_color_raw(bmp, width - 1, height - y - 1) != rightColor) {
      goto not_right;
    }
  }
  result = rightColor;
  return true;

 not_right:
  for (int x = 0; x != width; x++){
    if (get_color_raw(bmp, width - 1 - x, height - 1) != bottomColor){
      goto not_bottom;
    }
  }
  result = bottomColor;
  return true;
 not_bottom:
  return false;
}

bool get_auto_crop_rect(const Bitmap& bmp, IntRect& result){
  // First determine background color. Background color must have
  // atleast one complete same-colored edge.
  Color bgColor(0,0,0,0);
  bool gotColor = decide_color(bmp, bgColor);
  if (!gotColor){
    return false;
  }
  result = do_get_auto_crop_rect(bmp, bgColor);
  return !empty(result);
}

Optional<Color> get_edge_color(const Bitmap& bmp, const IntRect& r){
  if (r.x < 0 || r.x + r.w > bmp.m_w || r.y < 0 || r.y + r.h > bmp.m_h){
    // Invalid rectangle
    return Optional<Color>();
  }

  Color color = get_color_raw(bmp, r.x, r.y);
  for (int x = r.x; x != r.x + r.w; x++){
    if (get_color_raw(bmp, x, r.y) != color || get_color_raw(bmp, x, r.y + r.h - 1) != color) {
      return Optional<Color>(); // Failed
    }
  }
  for (int y = r.y; y != r.y + r.h; y++){
    if (get_color_raw(bmp, r.x + r.w - 1, y) != color ||
      get_color_raw(bmp, r.x, y) != color){
      return Optional<Color>(); // Failed
    }
  }
  // Color determined
  return Optional<Color>(color);
}

static bool get_horizontal_scanline_color(const Bitmap& bmp, int y, Color& result){
  result = get_color_raw(bmp, 0, y);
  for (int x = 1; x != bmp.m_w; x++){
    if (get_color_raw(bmp, x, y) != result){
      return false;
    }
  }
  return true;
}

static bool get_vertical_scanline_color(const Bitmap& bmp, int x, Color& result){
  result = get_color_raw(bmp, x, 0);
  for (int y = 1; y != bmp.m_h; y++){
    if (get_color_raw(bmp, x, y) != result){
      return false;
    }
  }
  return true;
}

bool get_bottom_edge_color(const Bitmap& bmp, Color& result){
  return get_horizontal_scanline_color(bmp, bmp.m_h - 1, result);
}

bool get_left_edge_color(const Bitmap& bmp, Color& result){
  return get_vertical_scanline_color(bmp, 0, result);
}

bool get_right_edge_color(const Bitmap& bmp, Color& result){
  return get_vertical_scanline_color(bmp, bmp.m_w - 1, result);
}

bool get_top_edge_color(const Bitmap& bmp, Color& result){
  return get_horizontal_scanline_color(bmp, 0, result);
}

std::vector<IntRect> get_auto_crop_rectangles(const Bitmap& bmp){
  Color color1;
  if (get_top_edge_color(bmp, color1)){
    std::vector<IntRect> v = { do_get_auto_crop_rect(bmp, color1) };
    if (empty(v[0])){
      // Blank image, the empty rectangle would cause everything to be
      // cropped away, return nothing instead
      return {};
    }
    Color color2;
    if (get_bottom_edge_color(bmp, color2) && color2 != color1){
      v.push_back(do_get_auto_crop_rect(bmp, color2));
    }
    return v;
  }
  else if (get_left_edge_color(bmp, color1)){
    std::vector<IntRect> v = { do_get_auto_crop_rect(bmp, color1) };
    Color color2;
    if (get_right_edge_color(bmp, color2) && color2 != color1){
      v.push_back(do_get_auto_crop_rect(bmp, color2));
    }
    return v;
  }
  else if (get_bottom_edge_color(bmp, color1)){
    return { do_get_auto_crop_rect(bmp, color1) };
  }
  else if (get_right_edge_color(bmp, color1)){
    return {do_get_auto_crop_rect(bmp, color1)};
  }
  return {};
}

std::vector<IntRect> get_auto_crop_rectangles(const Optional<Bitmap>& bmp){
  if (bmp.IsSet()){
    return get_auto_crop_rectangles(bmp.Get());
  }
  return {};
}

}
