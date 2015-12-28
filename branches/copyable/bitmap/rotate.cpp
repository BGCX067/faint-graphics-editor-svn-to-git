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

#include <cmath>
#include "bitmap/bitmap.hh"
#include "bitmap/rotate-util.hh"
#include "geo/angle.hh"
#include "util/color.hh"

// The rotation code is adapted from this example for Windows GDI by
// Yves Maurer: http://www.codeguru.com/cpp/g-m/gdi/article.php/c3693
namespace faint {
  static double min4(double a, double b, double c, double d){
    if (a < b) {
      if (c < a) {
        return (d < c) ? d : c;
      }
      else {
        return ( d < a ) ? d : a;
      }
    }
    else if (c < b) {
      return ( d < c ) ? d : c;
    }
    else {
      return ( d < b ) ? d : b;
    }
  }

  static double max4(double a, double b, double c, double d) {
    if (a > b) {
      if (c > a) {
        return (d > c) ? d : c;
      }
      else {
        return ( d > a ) ? d : a;
      }
    }
    else if (c > b) {
      return ( d > c ) ? d : c;
    }
    else {
      return ( d > b ) ? d : b;
    }
  }

  RotationAdjustment::RotationAdjustment( const IntPoint& in_offset, const IntSize& in_size )
    : offset(in_offset),
      size(in_size)
  {}

  RotationAdjustment get_rotation_adjustment(const faint::Angle& angle, const IntSize& size){
    // Use the upper-left corner as pivot
    const faint::coord CtX = 0;
    const faint::coord CtY = 0;

    // Find the corners to initialize the destination width and height
    const faint::coord cA = cos(angle);
    const faint::coord sA = sin(angle);

    const faint::coord x1 = CtX + (-CtX) * cA - (-CtY) * sA;
    const faint::coord x2 = CtX + (size.w - CtX) * cA - (-CtY) * sA;
    const faint::coord x3 = CtX + (size.w - CtX) * cA - (size.h - CtY) * sA;
    const faint::coord x4 = CtX + (-CtX) * cA - (size.h - CtY) * sA;

    const faint::coord y1 = CtY + (-CtY) * cA + (-CtX) * sA;
    const faint::coord y2 = CtY + (size.h - CtY) * cA + (-CtX) * sA;
    const faint::coord y3 = CtY + (size.h - CtY) * cA + (size.w - CtX) * sA;
    const faint::coord y4 = CtY + (-CtY) * cA + (size.w - CtX) * sA;


    IntPoint offset(floored(min4(x1, x2, x3, x4)),
      floored(min4(y1, y2, y3, y4)));
    IntSize newSize(ceiled(max4(x1, x2, x3, x4)) - offset.x,
      ceiled(max4(y1,y2,y3,y4)) - offset.y);

    return RotationAdjustment(offset, newSize);
  }


  faint::Bitmap rotate_nearest( const faint::Bitmap& bmp, const faint::Angle& angle, const faint::Color& bgColor ) {
    // Use the upper-left corner as pivot
    double CtX = 0;
    double CtY = 0;

    // Find the corners to initialize the destination width and height

    RotationAdjustment adj = get_rotation_adjustment(angle,
      bmp.GetSize());

    faint::Bitmap bmpDst( adj.size, bgColor );
    faint::uchar* dstRow = bmpDst.m_data;

    const faint::uchar* src = bmp.m_data;
    faint::coord ca = cos(angle);
    faint::coord sa = sin(angle);
    faint::coord divisor = ca*ca + sa*sa;

    // Iterate over the destination bitmap
    for ( int stepY = 0; stepY != bmpDst.m_h; stepY++ ) {
      for ( int stepX = 0; stepX != bmpDst.m_w; stepX++ ) {
        // Calculate the source coordinate
        int orgX = static_cast<int>((ca * (((double) stepX + adj.offset.x) + CtX * (ca - 1)) + sa * (((double) stepY + adj.offset.y) + CtY * (sa - 1))) / divisor );
        int orgY = static_cast<int>( CtY + (CtX - ((double) stepX + adj.offset.x)) * sa + ca *(((double) stepY + adj.offset.y ) - CtY + (CtY - CtX) * sa) );
        if ( orgX >= 0 && orgY >= 0 && orgX < bmp.m_w && orgY < bmp.m_h ) {
          // Inside source - copy the bits
          dstRow[ stepX * 4 + faint::iR] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iR];
          dstRow[ stepX * 4 + faint::iG] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iG];
          dstRow[ stepX * 4 + faint::iB] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iB];
          dstRow[ stepX * 4 + faint::iA] = src[orgX * 4 + orgY * bmp.m_row_stride + faint::iA];
        }
        else {
          // Outside source - set the color to bg color
          dstRow[ stepX * 4 + faint::iR] = bgColor.r;
          dstRow[ stepX * 4 + faint::iG] = bgColor.g;
          dstRow[ stepX * 4 + faint::iB] = bgColor.b;
          dstRow[ stepX * 4 + faint::iA] = bgColor.a;
        }
      }
      dstRow += bmpDst.m_row_stride;
    }

    return bmpDst;
  }

} // namespace
