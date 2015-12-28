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
#include "geo/geotypes.hh"
#include "autocrop.hh"

IntRect do_get_auto_crop_rect( const faint::Bitmap& bmp, const faint::Color& bgCol ){
  const unsigned int width = bmp.m_w;
  const unsigned int height = bmp.m_h;
  int y0 = 0;
  int y1 = 0;
  bool upDone = false;
  bool dnDone = false;

  for ( unsigned int y = 0; y != height; y++ ){
    for ( unsigned int x = 0; x!= width; x++ ){
      if ( !upDone ){
        if ( get_color_raw( bmp, x, y ) != bgCol ){
          y0 = y;
          upDone = true;
          if ( dnDone ){
            goto yDone;
          }
        }
      }
      if ( !dnDone ){
        if ( get_color_raw( bmp, x, height - y - 1 ) != bgCol ){
          y1 = height - y;
          dnDone = true;
          if ( upDone ){
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

  for ( unsigned int x = 0; x!= width; x++ ){
    for ( unsigned int y = 0; y!= height; y++ ){
      if ( !leftDone ){
        if ( get_color_raw( bmp, x, y ) != bgCol ){
          x0 = x;
          leftDone = true;
          if ( rightDone ){
            goto xDone;
          }
        }
      }
      if ( !rightDone ){
        if ( get_color_raw( bmp, width - x - 1, y ) != bgCol ){
          x1 = width - x;
          rightDone = true;
          if ( leftDone ){
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

bool decide_color( const faint::Bitmap& bmp, faint::Color& result ){
  const faint::Color leftColor = get_color_raw( bmp, 0, 0 );
  const faint::Color upColor = leftColor;
  const unsigned int width = bmp.m_w;
  const unsigned int height = bmp.m_h;

  for ( unsigned int x = 0; x < width; x++ ) {
    if ( get_color_raw( bmp, x, 0 ) != upColor ){
      goto not_top_row;
    }
  }
  result = upColor;
  return true;
 not_top_row:
  for ( unsigned int y = 0; y != height; y++ ){
    if ( get_color_raw( bmp, 0, y ) != leftColor ){
      goto not_left;
    }
  }
  result = leftColor;
  return true;
 not_left:

  const faint::Color rightColor = get_color_raw( bmp, bmp.m_w - 1, bmp.m_h - 1);
  const faint::Color bottomColor = rightColor;

  // and then to top right
  for ( unsigned int y = 0; y < height; y++ ){
    if ( get_color_raw( bmp, width - 1, height - y - 1 ) != rightColor ) {
      goto not_right;
    }
  }
  result = rightColor;
  return true;

 not_right:
  for ( unsigned int x = 0; x != width; x++ ){
    if ( get_color_raw( bmp, width - 1 - x, height - 1 ) != bottomColor ){
      goto not_bottom;
    }
  }
  result = bottomColor;
  return true;
 not_bottom:
  return false;
}

bool get_auto_crop_rect( const faint::Bitmap& bmp, IntRect& result ){
  // First determine background color. Background color must have
  // atleast one complete same-colored edge.
  faint::Color bgColor(0,0,0,0);
  bool gotColor = decide_color( bmp, bgColor );
  if ( !gotColor ){
    return false;
  }
  result = do_get_auto_crop_rect( bmp, bgColor );
  return !empty( result );
}

Optional<faint::Color> get_edge_color( const faint::Bitmap& bmp, const IntRect& r ){
  if ( r.x < 0 || r.x + r.w > static_cast<int>(bmp.m_w) || r.y < 0 || r.y + r.h > static_cast<int>(bmp.m_h) ){
    // Invalid rectangle
    return Optional<faint::Color>();
  }

  faint::Color color = get_color_raw( bmp, r.x, r.y );
  for ( int x = r.x; x != r.x + r.w; x++ ){
    if ( get_color_raw( bmp, x, r.y ) != color || get_color_raw( bmp, x, r.y + r.h - 1 ) != color ) {
      return Optional<faint::Color>(); // Failed
    }
  }
  for ( int y = r.y; y != r.y + r.h; y++ ){
    if ( get_color_raw( bmp, r.x + r.w - 1, y ) != color ||
      get_color_raw( bmp, r.x, y ) != color ){
      return Optional<faint::Color>(); // Failed
    }
  }
  // Color determined
  return Optional<faint::Color>(color);
}

bool get_horizontal_scanline_color( const faint::Bitmap& bmp, int y, faint::Color& result ){
  result = get_color_raw( bmp, 0, y );
  const int bmpW = static_cast<int>(bmp.m_w);
  for ( int x = 1; x != bmpW; x++ ){
    if ( get_color_raw( bmp, x, y ) != result ){
      return false;
    }
  }
  return true;
}

bool get_vertical_scanline_color( const faint::Bitmap& bmp, int x, faint::Color& result ){
  result = get_color_raw( bmp, x, 0 );
  const int bmpH = static_cast<int>(bmp.m_h);
  for ( int y = 1; y != bmpH; y++ ){
    if ( get_color_raw(bmp, x, y ) != result ){
      return false;
    }
  }
  return true;
}

bool get_bottom_edge_color( const faint::Bitmap& bmp, faint::Color& result ){
  return get_horizontal_scanline_color( bmp, bmp.m_h - 1, result );
}

bool get_left_edge_color( const faint::Bitmap& bmp, faint::Color& result ){
  return get_vertical_scanline_color( bmp, 0, result );
}

bool get_right_edge_color( const faint::Bitmap& bmp, faint::Color& result ){
  return get_vertical_scanline_color( bmp, bmp.m_w - 1, result );
}

bool get_top_edge_color( const faint::Bitmap& bmp, faint::Color& result ){
  return get_horizontal_scanline_color( bmp, 0, result );
}

std::vector<IntRect> get_auto_crop_rectangles( const faint::Bitmap& bmp ){
  faint::Color color1;
  if ( get_top_edge_color(bmp, color1) ){
    std::vector<IntRect> v;
    v.push_back(do_get_auto_crop_rect( bmp, color1 ));
    if ( empty(v[0]) ){
      // Empty image, everything would be cropped away
      return std::vector<IntRect>();
    }
    faint::Color color2;
    if ( get_bottom_edge_color(bmp, color2) && color2 != color1 ){
      v.push_back(do_get_auto_crop_rect( bmp, color2) );
    }
    return v;
  }
  else if ( get_left_edge_color(bmp, color1) ){
    std::vector<IntRect> v;
    v.push_back(do_get_auto_crop_rect(bmp, color1));
    faint::Color color2;
    if ( get_right_edge_color(bmp, color2) && color2 != color1 ){
      v.push_back(do_get_auto_crop_rect( bmp, color2 ) );
    }
    return v;
  }
  else if ( get_bottom_edge_color(bmp, color1) ){
    std::vector<IntRect> v;
    v.push_back(do_get_auto_crop_rect( bmp, color1 ));
    return v;
  }
  else if ( get_right_edge_color(bmp, color1) ){
    std::vector<IntRect> v;
    v.push_back(do_get_auto_crop_rect(bmp, color1));
    return v;
  }
  return std::vector<IntRect>();
}
